#include <ctype.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <logging.h>
#include <reg.h>
#include <stlink.h>

#define DEFAULT_LOGGING_LEVEL 50
#define DEBUG_LOGGING_LEVEL 100

#define APP_RESULT_SUCCESS 0
#define APP_RESULT_INVALID_PARAMS 1
#define APP_RESULT_STLINK_NOT_FOUND 2
#define APP_RESULT_STLINK_MISSING_DEVICE 3
#define APP_RESULT_STLINK_UNSUPPORTED_DEVICE 4
#define APP_RESULT_STLINK_UNSUPPORTED_LINK 5
#define APP_RESULT_UNSUPPORTED_TRACE_FREQUENCY 6
#define APP_RESULT_STLINK_STATE_ERROR 7

// See D4.2 of https://developer.arm.com/documentation/ddi0403/ed/
#define TRACE_OP_IS_OVERFLOW(c) ((c) == 0x70)
#define TRACE_OP_IS_LOCAL_TIME(c) (((c)&0x0f) == 0x00 && ((c)&0x70) != 0x00)
#define TRACE_OP_IS_EXTENSION(c) (((c)&0x0b) == 0x08)
#define TRACE_OP_IS_GLOBAL_TIME(c) (((c)&0xdf) == 0x94)
#define TRACE_OP_IS_SOURCE(c) (((c)&0x03) != 0x00)
#define TRACE_OP_IS_SW_SOURCE(c) (((c)&0x03) != 0x00 && ((c)&0x04) == 0x00)
#define TRACE_OP_IS_HW_SOURCE(c) (((c)&0x03) != 0x00 && ((c)&0x04) == 0x04)
#define TRACE_OP_IS_TARGET_SOURCE(c) ((c) == 0x01)
#define TRACE_OP_GET_CONTINUATION(c) ((c)&0x80)
#define TRACE_OP_GET_SOURCE_SIZE(c) ((c)&0x03)
#define TRACE_OP_GET_SW_SOURCE_ADDR(c) ((c) >> 3)

typedef struct {
  bool show_help;
  bool show_version;
  int logging_level;
  uint32_t core_frequency;
  uint32_t trace_frequency;
  bool reset_board;
  bool force;
  char *serial_number;
} st_settings_t;

// We use a simple state machine to parse the trace data.
typedef enum {
  TRACE_STATE_UNKNOWN,
  TRACE_STATE_IDLE,
  TRACE_STATE_TARGET_SOURCE,
  TRACE_STATE_SKIP_FRAME,
  TRACE_STATE_SKIP_4,
  TRACE_STATE_SKIP_3,
  TRACE_STATE_SKIP_2,
  TRACE_STATE_SKIP_1,
} trace_state;

typedef struct {
  time_t start_time;
  bool configuration_checked;

  trace_state state;

  uint32_t count_raw_bytes;
  uint32_t count_target_data;
  uint32_t count_time_packets;
  uint32_t count_hw_overflow;
  uint32_t count_sw_overflow;
  uint32_t count_error;

  uint8_t unknown_opcodes[256 / 8];
  uint32_t unknown_sources;
} st_trace_t;

// We use a global flag to allow communicating to the main thread from the
// signal handler.
static bool g_abort_trace = false;

static void abort_trace() { g_abort_trace = true; }

#if defined(_WIN32)
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
  (void)fdwCtrlType;
  abort_trace();
  return FALSE;
}
#endif

static void usage(void) {
  puts("st-trace - usage:");
  puts("  -h, --help            Print this help");
  puts("  -V, --version         Print this version");
  puts("  -vXX, --verbose=XX    Specify a specific verbosity level (0..99)");
  puts("  -v, --verbose         Specify a generally verbose logging");
  puts("  -cXX, --clock=XX      Specify the core frequency in MHz");
  puts("  -tXX, --trace=XX      Specify the trace frequency in Hz");
  puts("  -n, --no-reset        Do not reset board on connection");
  puts("  -sXX, --serial=XX     Use a specific serial number");
  puts("  -f, --force           Ignore most initialization errors");
}

bool parse_options(int argc, char **argv, st_settings_t *settings) {

  static struct option long_options[] = {
      {"help", no_argument, NULL, 'h'},
      {"version", no_argument, NULL, 'V'},
      {"verbose", optional_argument, NULL, 'v'},
      {"clock", required_argument, NULL, 'c'},
      {"trace", required_argument, NULL, 't'},
      {"no-reset", no_argument, NULL, 'n'},
      {"serial", required_argument, NULL, 's'},
      {"force", no_argument, NULL, 'f'},
      {0, 0, 0, 0},
  };
  int option_index = 0;
  int c;
  bool error = false;

  settings->show_help = false;
  settings->show_version = false;
  settings->logging_level = DEFAULT_LOGGING_LEVEL;
  settings->core_frequency = 0;
  settings->trace_frequency = 0;
  settings->reset_board = true;
  settings->force = false;
  settings->serial_number = NULL;
  ugly_init(settings->logging_level);

  while ((c = getopt_long(argc, argv, "hVv::c:ns:f", long_options,
                          &option_index)) != -1) {
    switch (c) {
    case 'h':
      settings->show_help = true;
      break;
    case 'V':
      settings->show_version = true;
      break;
    case 'v':
      if (optarg) {
        settings->logging_level = atoi(optarg);
      } else {
        settings->logging_level = DEBUG_LOGGING_LEVEL;
      }
      ugly_init(settings->logging_level);
      break;
    case 'c':
      settings->core_frequency = atoi(optarg) * 1000000;
      break;
    case 't':
      settings->trace_frequency = atoi(optarg);
      break;
    case 'n':
      settings->reset_board = false;
      break;
    case 'f':
      settings->force = true;
      break;
    case 's':
      settings->serial_number = optarg;
      break;
    case '?':
      error = true;
      break;
    default:
      ELOG("Unknown command line option: '%c' (0x%02x)\n", c, c);
      error = true;
      break;
    }
  }

  if (optind < argc) {
    while (optind < argc) {
      ELOG("Unknown command line argument: '%s'\n", argv[optind++]);
    }
    error = true;
  }

  if (error && !settings->force)
    return false;

  return true;
}

static stlink_t *stlink_connect(const st_settings_t *settings) {
  return stlink_open_usb(settings->logging_level, false,
                         settings->serial_number, 0);
}

static bool enable_trace(stlink_t *stlink, const st_settings_t *settings,
                         uint32_t trace_frequency) {

  if (stlink_force_debug(stlink)) {
    ELOG("Unable to debug device\n");
    if (!settings->force)
      return false;
  }

  if (settings->reset_board && stlink_reset(stlink, RESET_AUTO)) {
    ELOG("Unable to reset device\n");
    if (!settings->force)
      return false;
  }

  stlink_write_debug32(stlink, STLINK_REG_DHCSR,
                       STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_DEBUGEN |
                           STLINK_REG_DHCSR_C_HALT);
  stlink_write_debug32(stlink, STLINK_REG_DEMCR, STLINK_REG_DEMCR_TRCENA);
  stlink_write_debug32(stlink, STLINK_REG_CM3_FP_CTRL,
                       STLINK_REG_CM3_FP_CTRL_KEY);
  stlink_write_debug32(stlink, STLINK_REG_DWT_FUNCTION0, 0);
  stlink_write_debug32(stlink, STLINK_REG_DWT_FUNCTION1, 0);
  stlink_write_debug32(stlink, STLINK_REG_DWT_FUNCTION2, 0);
  stlink_write_debug32(stlink, STLINK_REG_DWT_FUNCTION3, 0);
  stlink_write_debug32(stlink, STLINK_REG_DWT_CTRL, 0);
  stlink_write_debug32(
      stlink, STLINK_REG_DBGMCU_CR,
      STLINK_REG_DBGMCU_CR_DBG_SLEEP | STLINK_REG_DBGMCU_CR_DBG_STOP |
          STLINK_REG_DBGMCU_CR_DBG_STANDBY | STLINK_REG_DBGMCU_CR_TRACE_IOEN |
          STLINK_REG_DBGMCU_CR_TRACE_MODE_ASYNC);

  if (stlink_trace_enable(stlink, trace_frequency)) {
    ELOG("Unable to turn on tracing in stlink\n");
    if (!settings->force)
      return false;
  }

  stlink_write_debug32(stlink, STLINK_REG_TPI_CSPSR,
                       STLINK_REG_TPI_CSPSR_PORT_SIZE_1);

  if (settings->core_frequency) {
    uint32_t prescaler = settings->core_frequency / trace_frequency - 1;
    if (prescaler > STLINK_REG_TPI_ACPR_MAX) {
      ELOG("Trace frequency prescaler %d out of range.  Try setting a faster "
           "trace frequency.\n",
           prescaler);
      if (!settings->force)
        return false;
    }
    stlink_write_debug32(stlink, STLINK_REG_TPI_ACPR,
                         prescaler); // Set TPIU_ACPR clock divisor
  }
  stlink_write_debug32(stlink, STLINK_REG_TPI_FFCR,
                       STLINK_REG_TPI_FFCR_TRIG_IN);
  stlink_write_debug32(stlink, STLINK_REG_TPI_SPPR,
                       STLINK_REG_TPI_SPPR_SWO_NRZ);
  stlink_write_debug32(stlink, STLINK_REG_ITM_LAR, STLINK_REG_ITM_LAR_KEY);
  stlink_write_debug32(stlink, STLINK_REG_ITM_TCC,
                       0x00000400); // Set sync counter
  stlink_write_debug32(stlink, STLINK_REG_ITM_TCR,
                       STLINK_REG_ITM_TCR_TRACE_BUS_ID_1 |
                           STLINK_REG_ITM_TCR_TS_ENA |
                           STLINK_REG_ITM_TCR_ITM_ENA);
  stlink_write_debug32(stlink, STLINK_REG_ITM_TER,
                       STLINK_REG_ITM_TER_PORTS_ALL);
  stlink_write_debug32(stlink, STLINK_REG_ITM_TPR,
                       STLINK_REG_ITM_TPR_PORTS_ALL);
  stlink_write_debug32(stlink, STLINK_REG_DWT_CTRL,
                       4 * STLINK_REG_DWT_CTRL_NUM_COMP |
                           STLINK_REG_DWT_CTRL_CYC_TAP |
                           0xF * STLINK_REG_DWT_CTRL_POST_INIT |
                           0xF * STLINK_REG_DWT_CTRL_POST_PRESET |
                           STLINK_REG_DWT_CTRL_CYCCNT_ENA);
  stlink_write_debug32(stlink, STLINK_REG_DEMCR, STLINK_REG_DEMCR_TRCENA);

  uint32_t prescaler;
  stlink_read_debug32(stlink, STLINK_REG_TPI_ACPR_MAX, &prescaler);
  if (prescaler) {
    uint32_t system_clock_speed = (prescaler + 1) * trace_frequency;
    uint32_t system_clock_speed_mhz = (system_clock_speed + 500000) / 1000000;
    ILOG("Trace Port Interface configured to expect a %d MHz system clock.\n",
         system_clock_speed_mhz);
  } else {
    WLOG("Trace Port Interface not configured.  Specify the system clock with "
         "a --clock=XX command\n");
    WLOG("line option or set it in your device's clock initialization routine, "
         "such as with:\n");
    WLOG("  TPI->ACPR = HAL_RCC_GetHCLKFreq() / %d - 1;\n", trace_frequency);
  }
  ILOG("Trace frequency set to %d Hz.\n", trace_frequency);

  return true;
}

static trace_state update_trace_idle(st_trace_t *trace, uint8_t c) {
  // Handle a trace byte when we are in the idle state.

  if (TRACE_OP_IS_TARGET_SOURCE(c)) {
    return TRACE_STATE_TARGET_SOURCE;
  }

  if (TRACE_OP_IS_SOURCE(c)) {
    uint8_t size = TRACE_OP_GET_SOURCE_SIZE(c);
    if (TRACE_OP_IS_SW_SOURCE(c)) {
      uint8_t addr = TRACE_OP_GET_SW_SOURCE_ADDR(c);
      if (!(trace->unknown_sources & (1 << addr)))
        WLOG("Unsupported source 0x%x size %d\n", addr, size);
      trace->unknown_sources |= (1 << addr);
    }
    if (size == 1)
      return TRACE_STATE_SKIP_1;
    if (size == 2)
      return TRACE_STATE_SKIP_2;
    if (size == 3)
      return TRACE_STATE_SKIP_4;
  }

  if (TRACE_OP_IS_LOCAL_TIME(c) || TRACE_OP_IS_GLOBAL_TIME(c)) {
    trace->count_time_packets++;
    return TRACE_OP_GET_CONTINUATION(c) ? TRACE_STATE_SKIP_FRAME
                                        : TRACE_STATE_IDLE;
  }

  if (TRACE_OP_IS_EXTENSION(c)) {
    return TRACE_OP_GET_CONTINUATION(c) ? TRACE_STATE_SKIP_FRAME
                                        : TRACE_STATE_IDLE;
  }

  if (TRACE_OP_IS_OVERFLOW(c)) {
    trace->count_hw_overflow++;
  }

  if (!(trace->unknown_opcodes[c / 8] & (1 << c % 8)))
    WLOG("Unknown opcode 0x%02x\n", c);
  trace->unknown_opcodes[c / 8] |= (1 << c % 8);

  trace->count_error++;
  return TRACE_OP_GET_CONTINUATION(c) ? TRACE_STATE_SKIP_FRAME
                                      : TRACE_STATE_IDLE;
}

static trace_state update_trace(st_trace_t *trace, uint8_t c) {
  trace->count_raw_bytes++;

  // Parse the input using a state machine.

  if (trace->state == TRACE_STATE_UNKNOWN) {
    if (TRACE_OP_IS_TARGET_SOURCE(c) || TRACE_OP_IS_LOCAL_TIME(c) ||
        TRACE_OP_IS_GLOBAL_TIME(c))
      trace->state = TRACE_STATE_IDLE;
  }

  switch (trace->state) {
  case TRACE_STATE_IDLE:
    return update_trace_idle(trace, c);

  case TRACE_STATE_TARGET_SOURCE:
    putchar(c);
    if (c == '\n')
      fflush(stdout);
    trace->count_target_data++;
    return TRACE_STATE_IDLE;

  case TRACE_STATE_SKIP_FRAME:
    return TRACE_OP_GET_CONTINUATION(c) ? TRACE_STATE_SKIP_FRAME
                                        : TRACE_STATE_IDLE;

  case TRACE_STATE_SKIP_4:
    return TRACE_STATE_SKIP_3;

  case TRACE_STATE_SKIP_3:
    return TRACE_STATE_SKIP_2;

  case TRACE_STATE_SKIP_2:
    return TRACE_STATE_SKIP_1;

  case TRACE_STATE_SKIP_1:
    return TRACE_STATE_IDLE;

  case TRACE_STATE_UNKNOWN:
    return TRACE_STATE_UNKNOWN;

  default:
    ELOG("Invalid state %d.  This should never happen\n", trace->state);
    return TRACE_STATE_IDLE;
  }
}

static bool read_trace(stlink_t *stlink, st_trace_t *trace) {
  uint8_t buffer[STLINK_TRACE_BUF_LEN];
  int length = stlink_trace_read(stlink, buffer, sizeof(buffer));

  if (length < 0) {
    ELOG("Error reading trace (%d)\n", length);
    return false;
  }

  if (length == 0) {
    usleep(100);
    return true;
  }

  if (length == sizeof(buffer)) {
    if (trace->count_sw_overflow++)
      DLOG("Buffer overflow.\n");
    else
      WLOG("Buffer overflow.  Try using a slower trace frequency.\n");
    trace->state = TRACE_STATE_UNKNOWN;
  }

  for (int i = 0; i < length; i++) {
    trace->state = update_trace(trace, buffer[i]);
  }

  return true;
}

static void check_for_configuration_error(stlink_t *stlink, st_trace_t *trace,
                                          uint32_t trace_frequency) {
  // Only check configuration one time after the first 10 seconds of running.
  time_t elapsed_time_s = time(NULL) - trace->start_time;
  if (trace->configuration_checked || elapsed_time_s < 10) {
    return;
  }
  trace->configuration_checked = true;

  // Simple huristic to determine if we are configured poorly.
  bool error_no_data = (trace->count_raw_bytes < 100);
  bool error_low_data =
      (trace->count_time_packets < 10 && trace->count_target_data < 1000);
  bool error_bad_data = (trace->count_error > 1 || trace->unknown_sources > 0);
  bool error_dropped_data = (trace->count_sw_overflow > 0);

  if (!error_no_data && !error_low_data && !error_bad_data &&
      !error_dropped_data)
    return;

  WLOG("****\n");
  WLOG("We do not appear to be retrieving data from the stlink correctly.\n");

  if (error_dropped_data) {
    WLOG("Try setting a slower trace frequency with the --trace=%d command "
         "line option.\n",
         trace_frequency / 2);
  }

  if (error_no_data || error_low_data || error_bad_data) {
    uint32_t prescaler;
    stlink_read_debug32(stlink, STLINK_REG_TPI_ACPR, &prescaler);
    if (prescaler) {
      uint32_t system_clock_speed = (prescaler + 1) * trace_frequency;
      uint32_t system_clock_speed_mhz = (system_clock_speed + 500000) / 1000000;
      WLOG("Verify the system clock is running at %d MHz.\n",
           system_clock_speed_mhz);
    }
    WLOG("Try specifying the system clock with the --clock=XX command line "
         "option.\n");
    WLOG("Try setting the trace speed in your device's clock initialization "
         "routine:\n");
    WLOG("  TPI->ACPR = HAL_RCC_GetHCLKFreq() / %d - 1;\n", trace_frequency);
  }

  WLOG("Diagnostic Information:\n");
  WLOG("Raw Bytes: %d\n", trace->count_raw_bytes);
  WLOG("Target Data: %d\n", trace->count_target_data);
  WLOG("Time Packets: %d\n", trace->count_time_packets);
  WLOG("Hardware Overflow Count: %d\n", trace->count_hw_overflow);
  WLOG("Software Overflow Count: %d\n", trace->count_sw_overflow);
  WLOG("Errors: %d\n", trace->count_error);

  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  uint32_t offset = 0;
  for (uint32_t i = 0; i <= 0xFF; i++)
    if (trace->unknown_opcodes[i / 8] & (1 << i % 8)) {
      uint32_t n =
          snprintf(buffer + offset, sizeof(buffer) - offset, "%02x, ", i);
      if (n >= sizeof(buffer) - offset) {
        break;
      }
      offset += n;
    }
  WLOG("Unknown Opcodes: %s\n", buffer);

  memset(buffer, 0, sizeof(buffer));
  offset = 0;
  for (uint32_t i = 0; i < 32; i++)
    if (trace->unknown_sources & (1 << i)) {
      uint32_t n =
          snprintf(buffer + offset, sizeof(buffer) - offset, "%d, ", i);
      if (n >= sizeof(buffer) - offset) {
        break;
      }
      offset += n;
    }
  WLOG("Unknown Sources: %s\n", buffer);

  WLOG("Chip ID: 0x%04x\n", stlink->chip_id);
  WLOG("****\n");
}

int main(int argc, char **argv) {
#if defined(_WIN32)
  SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
#else
  signal(SIGINT, &abort_trace);
  signal(SIGTERM, &abort_trace);
  signal(SIGSEGV, &abort_trace);
  signal(SIGPIPE, &abort_trace);
#endif

  st_settings_t settings;
  if (!parse_options(argc, argv, &settings)) {
    usage();
    return APP_RESULT_INVALID_PARAMS;
  }
  init_chipids (ETC_STLINK_DIR);


  DLOG("show_help = %s\n", settings.show_help ? "true" : "false");
  DLOG("show_version = %s\n", settings.show_version ? "true" : "false");
  DLOG("logging_level = %d\n", settings.logging_level);
  DLOG("core_frequency = %d Hz\n", settings.core_frequency);
  DLOG("trace_frequency = %d Hz\n", settings.trace_frequency);
  DLOG("reset_board = %s\n", settings.reset_board ? "true" : "false");
  DLOG("force = %s\n", settings.force ? "true" : "false");
  DLOG("serial_number = %s\n",
       settings.serial_number ? settings.serial_number : "any");

  if (settings.show_help) {
    usage();
    return APP_RESULT_SUCCESS;
  }

  if (settings.show_version) {
    printf("v%s\n", STLINK_VERSION);
    return APP_RESULT_SUCCESS;
  }

  stlink_t *stlink = stlink_connect(&settings);
  if (!stlink) {
    ELOG("Unable to locate an stlink\n");
    return APP_RESULT_STLINK_NOT_FOUND;
  }

  stlink->verbose = settings.logging_level;

  if (stlink->chip_id == STLINK_CHIPID_UNKNOWN) {
    ELOG("Your stlink is not connected to a device\n");
    if (!settings.force)
      return APP_RESULT_STLINK_MISSING_DEVICE;
  }

  if (!(stlink->version.flags & STLINK_F_HAS_TRACE)) {
    ELOG("Your stlink does not support tracing\n");
    if (!settings.force)
      return APP_RESULT_STLINK_UNSUPPORTED_LINK;
  }

  if (!(stlink->chip_flags & CHIP_F_HAS_SWO_TRACING)) {
    const struct stlink_chipid_params *params =
        stlink_chipid_get_params(stlink->chip_id);
    ELOG("We do not support SWO output for device '%s'\n", params->description);
    if (!settings.force)
      return APP_RESULT_STLINK_UNSUPPORTED_DEVICE;
  }

  uint32_t trace_frequency = settings.trace_frequency;
  if (!trace_frequency)
    trace_frequency = STLINK_DEFAULT_TRACE_FREQUENCY;
  if (trace_frequency > stlink->max_trace_freq) {
    ELOG("Invalid trace frequency %d (max %d)\n", trace_frequency,
         stlink->max_trace_freq);
    if (!settings.force)
      return APP_RESULT_UNSUPPORTED_TRACE_FREQUENCY;
  }

  if (!enable_trace(stlink, &settings, trace_frequency)) {
    ELOG("Unable to enable trace mode\n");
    if (!settings.force)
      return APP_RESULT_STLINK_STATE_ERROR;
  }

  ILOG("Reading Trace\n");
  st_trace_t trace;
  memset(&trace, 0, sizeof(trace));
  trace.start_time = time(NULL);

  if (stlink_run(stlink, RUN_NORMAL)) {
    ELOG("Unable to run device\n");
    if (!settings.force)
      return APP_RESULT_STLINK_STATE_ERROR;
  }

  while (!g_abort_trace && read_trace(stlink, &trace)) {
    check_for_configuration_error(stlink, &trace, trace_frequency);
  }

  stlink_trace_disable(stlink);
  stlink_close(stlink);

  return APP_RESULT_SUCCESS;
}
