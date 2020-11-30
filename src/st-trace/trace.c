#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>

#include <stlink.h>
#include <logging.h>


#define DEFAULT_LOGGING_LEVEL 50
#define DEBUG_LOGGING_LEVEL 100

#define APP_RESULT_SUCCESS                      0
#define APP_RESULT_INVALID_PARAMS               1
#define APP_RESULT_STLINK_NOT_FOUND             2
#define APP_RESULT_STLINK_MISSING_DEVICE        3
#define APP_RESULT_STLINK_UNSUPPORTED_DEVICE    4
#define APP_RESULT_STLINK_UNSUPPORTED_LINK      5
#define APP_RESULT_STLINK_STATE_ERROR           6

// See D4.2 of https://developer.arm.com/documentation/ddi0403/ed/
#define TRACE_OP_IS_OVERFLOW(c)         ((c) == 0x70)
#define TRACE_OP_IS_SYNC(c)             ((c) == 0x00)
#define TRACE_OP_IS_LOCAL_TIME(c)       (((c) & 0x0f) == 0x00 && ((c) & 0x70) != 0x00)
#define TRACE_OP_IS_EXTENSION(c)        (((c) & 0x0b) == 0x08)
#define TRACE_OP_IS_GLOBAL_TIME(c)      (((c) & 0xdf) == 0x94)
#define TRACE_OP_IS_SOURCE(c)           (((c) & 0x03) != 0x00)
#define TRACE_OP_IS_SW_SOURCE(c)        (((c) & 0x03) != 0x00 && ((c) & 0x04) == 0x00)
#define TRACE_OP_IS_HW_SOURCE(c)        (((c) & 0x03) != 0x00 && ((c) & 0x04) == 0x04)
#define TRACE_OP_IS_TARGET_SOURCE(c)    ((c) == 0x01)
#define TRACE_OP_GET_CONTINUATION(c)    ((c) & 0x80)
#define TRACE_OP_GET_SOURCE_SIZE(c)     ((c) & 0x03)
#define TRACE_OP_GET_SW_SOURCE_ADDR(c)  ((c) >> 3)


// We use a global flag to allow communicating to the main thread from the signal handler.
static bool g_done = false;


struct _st_settings_t {
    bool  show_help;
    bool  show_version;
    int   logging_level;
    int   core_frequency_mhz;
    bool  reset_board;
    bool  force;
    char* serial_number;
};
typedef struct _st_settings_t st_settings_t;


// We use a simple state machine to parse the trace data.
enum _trace_state {
    TRACE_STATE_IDLE,
    TRACE_STATE_TARGET_SOURCE,
    TRACE_STATE_SKIP_FRAME,
    TRACE_STATE_SKIP_4,
    TRACE_STATE_SKIP_3,
    TRACE_STATE_SKIP_2,
    TRACE_STATE_SKIP_1,
};
typedef enum _trace_state trace_state;

struct _st_trace_t {
    trace_state state;
    bool        overflow;
    bool        error;
};
typedef struct _st_trace_t st_trace_t;


static void usage(void) {
    puts("st-trace - usage:");
    puts("  -h, --help            Print this help");
    puts("  -V, --version         Print this version");
    puts("  -vXX, --verbose=XX    Specify a specific verbosity level (0..99)");
    puts("  -v, --verbose         Specify a generally verbose logging");
    puts("  -cXX, --clock=XX      Specify the core frequency in MHz");
    puts("  -n, --no-reset        Do not reset board on connection");
    puts("  -sXX, --serial=XX     Use a specific serial number");
    puts("  -f, --force           Ignore some errors");
}

static void cleanup() {
    g_done = true;
}

bool parse_options(int argc, char** argv, st_settings_t *settings) {

    static struct option long_options[] = {
        {"help",     no_argument,       NULL, 'h'},
        {"version",  no_argument,       NULL, 'V'},
        {"verbose",  optional_argument, NULL, 'v'},
        {"clock",    required_argument, NULL, 'c'},
        {"no-reset", no_argument,       NULL, 'n'},
        {"serial",   required_argument, NULL, 's'},
        {"force",    no_argument,       NULL, 'f'},
        {0, 0, 0, 0},
    };
    int option_index = 0;
    int c;
    bool error = false;

    settings->show_help = false;
    settings->show_version = false;
    settings->logging_level = DEFAULT_LOGGING_LEVEL;
    settings->core_frequency_mhz = 0;
    settings->reset_board = true;
    settings->force = false;
    settings->serial_number = NULL;
    ugly_init(settings->logging_level);

    while ((c = getopt_long(argc, argv, "hVv::c:ns:f", long_options, &option_index)) != -1) {
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
            settings->core_frequency_mhz = atoi(optarg);
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

static stlink_t* StLinkConnect(const st_settings_t* settings) {
    if (settings->serial_number) {
        // Convert our serial number string to a binary value.
        char serial_number[STLINK_SERIAL_MAX_SIZE] = { 0 };
        size_t length = 0;
        for (uint32_t n = 0; n < strlen(settings->serial_number) && length < STLINK_SERIAL_MAX_SIZE; n += 2) {
            char buffer[3] = { 0 };
            memcpy(buffer, settings->serial_number + n, 2);
            serial_number[length++] = (uint8_t)strtol(buffer, NULL, 16);
        }

        // Open this specific stlink.
        return stlink_open_usb(settings->logging_level, false, serial_number, 0);
    } else {
        // Otherwise, open any stlink.
        return stlink_open_usb(settings->logging_level, false, NULL, 0);
    }
}

// TODO: Consider moving this to the device table.
static bool IsDeviceTraceSupported(int chip_id) {
    switch (chip_id) {
    case STLINK_CHIPID_STM32_F4:
    case STLINK_CHIPID_STM32_F4_DSI:
    case STLINK_CHIPID_STM32_F4_HD:
    case STLINK_CHIPID_STM32_F4_LP:
    case STLINK_CHIPID_STM32_F411RE:
    case STLINK_CHIPID_STM32_F4_DE:
    case STLINK_CHIPID_STM32_F446:
    case STLINK_CHIPID_STM32_F410:
    case STLINK_CHIPID_STM32_F3:
    case STLINK_CHIPID_STM32_F37x:
    case STLINK_CHIPID_STM32_F412:
    case STLINK_CHIPID_STM32_F413:
    case STLINK_CHIPID_STM32_F3_SMALL:
    case STLINK_CHIPID_STM32_F334:
    case STLINK_CHIPID_STM32_F303_HIGH:
        return true;
    default:
        return false;
    }
}

static void Write32(stlink_t* stlink, uint32_t address, uint32_t data) {
    write_uint32(stlink->q_buf, data);
    if (stlink_write_mem32(stlink, address, 4)) {
        ELOG("Unable to set address 0x%08x to 0x%08x\n", address, data);
    }
}

static uint32_t Read32(stlink_t* stlink, uint32_t address) {
    if (stlink_read_mem32(stlink, address, 4)) {
        ELOG("Unable to read from address 0x%08x\n", address);
    }
    return read_uint32(stlink->q_buf, 0);
}

static bool EnableTrace(stlink_t* stlink, const st_settings_t* settings) {
    struct stlink_reg regp = {};

    if (stlink_force_debug(stlink)) {
        ELOG("Unable to debug device\n");
        if (!settings->force)
            return false;
    }

    if (settings->reset_board && stlink_reset(stlink)) {
        ELOG("Unable to reset device\n");
        if (!settings->force)
            return false;
    }

    // The order and values to set were taken from https://github.com/avrhack/stlink-trace/blob/master/stlink-trace.c#L386

    Write32(stlink, 0xE000EDF0, 0xA05F0003); // Set DHCSR to C_HALT and C_DEBUGEN
    Write32(stlink, 0xE000EDFC, 0x01000000); // Set TRCENA flag to enable global DWT and ITM
    Write32(stlink, 0xE0002000, 0x00000002); // Set FP_CTRL to enable write
    Write32(stlink, 0xE0001028, 0x00000000); // Set DWT_FUNCTION0 to DWT_FUNCTION3 to disable sampling
    Write32(stlink, 0xE0001038, 0x00000000);
    Write32(stlink, 0xE0001048, 0x00000000);
    Write32(stlink, 0xE0001058, 0x00000000);
    Write32(stlink, 0xE0001000, 0x00000000); // Clear DWT_CTRL and other registers
    Write32(stlink, 0xE0001004, 0x00000000);
    Write32(stlink, 0xE0001008, 0x00000000);
    Write32(stlink, 0xE000100C, 0x00000000);
    Write32(stlink, 0xE0001010, 0x00000000);
    Write32(stlink, 0xE0001014, 0x00000000);
    Write32(stlink, 0xE0001018, 0x00000000);

    stlink_read_reg(stlink, 15, &regp); // Read program counter
    stlink_read_reg(stlink, 16, &regp); // Read xpsr register

    Write32(stlink, 0xE0042004, 0x00000027); // Set DBGMCU_CR to enable asynchronous transmission

    // Actually start tracing.
    if (stlink_trace_enable(stlink)) {
        ELOG("Unable to trace device\n");
        if (!settings->force)
            return false;
    }

    Write32(stlink, 0xE0040004, 0x00000001); // Set TPIU_CSPSR to enable trace port width of 2

    if (settings->core_frequency_mhz) {
        uint32_t prescaler = settings->core_frequency_mhz * 1000000 / STLINK_TRACE_FREQUENCY - 1;
        Write32(stlink, 0xE0040010, prescaler); // Set TPIU_ACPR clock divisor
    }
    uint32_t prescaler = Read32(stlink, 0xE0040010);
    if (prescaler) {
        uint32_t system_clock_speed = (prescaler + 1) * STLINK_TRACE_FREQUENCY;
        ILOG("Trace Port Interface configured to expect a %d MHz system clock.\n", (system_clock_speed + 500000) / 1000000);
    } else {
        WLOG("Trace Port Interface not configured.  Specify the system clock with a --clock=XX command\n");
        WLOG("line option or set it in your device's clock initialization routine, such as with:\n");
        WLOG("  TPI->ACPR = HAL_RCC_GetHCLKFreq() / 2000000 - 1;\n");
    }

    Write32(stlink, 0xE00400F0, 0x00000002); // Set TPIU_SPPR to Asynchronous SWO (NRZ)
    Write32(stlink, 0xE0040304, 0x00000100); // Set TPIU_FFCR continuous formatting)
    Write32(stlink, 0xE0000FB0, 0xC5ACCE55); // Unlock the ITM registers for write
    Write32(stlink, 0xE0000E90, 0x00000400); // Set sync counter
    Write32(stlink, 0xE0000E80, 0x00010003); // Set ITM_TCR flags : ITMENA,TSENA ATB=0
    Write32(stlink, 0xE0000E00, 0xFFFFFFFF); // Enable all trace ports in ITM_TER
    Write32(stlink, 0xE0000E40, 0x0000000F); // Enable unprivileged access to trace ports 31:0 in ITM_TPR
    Write32(stlink, 0xE0000E40, 0x400003FE); // Set DWT_CTRL flags)
    Write32(stlink, 0xE000EDFC, 0x01000000); // Enable tracing (DEMCR - TRCENA bit)

    return true;
}

static void UpdateTrace(st_trace_t* trace, uint8_t c) {
    // Parse the input using a state machine.
    switch (trace->state)
    {
    case TRACE_STATE_IDLE:
        if (TRACE_OP_IS_TARGET_SOURCE(c)) {
            trace->state = TRACE_STATE_TARGET_SOURCE;
        } else if (TRACE_OP_IS_SOURCE(c)) {
            const trace_state kSourceSkip[] = { TRACE_STATE_IDLE, TRACE_STATE_SKIP_1, TRACE_STATE_SKIP_2, TRACE_STATE_SKIP_4 };
            const char* type = TRACE_OP_IS_SW_SOURCE(c) ? "sw" : "hw";
            uint8_t addr = TRACE_OP_GET_SW_SOURCE_ADDR(c);
            uint8_t size = TRACE_OP_GET_SOURCE_SIZE(c);
            if (TRACE_OP_IS_SW_SOURCE(c))
                WLOG("Unsupported %s source 0x%x size %d\n", type, addr, size);
            trace->state = kSourceSkip[size];
        } else if (TRACE_OP_IS_LOCAL_TIME(c) || TRACE_OP_IS_GLOBAL_TIME(c)) {
            if (TRACE_OP_GET_CONTINUATION(c))
                trace->state = TRACE_STATE_SKIP_FRAME;
        } else if (TRACE_OP_IS_EXTENSION(c)) {
            if (TRACE_OP_GET_CONTINUATION(c))
                trace->state = TRACE_STATE_SKIP_FRAME;
        } else if (TRACE_OP_IS_SYNC(c)) {
        } else if (TRACE_OP_IS_OVERFLOW(c)) {
            trace->overflow = true;
        } else {
            WLOG("Unknown opcode 0x%02x\n", c);
            trace->error = true;
            if (TRACE_OP_GET_CONTINUATION(c))
                trace->state = TRACE_STATE_SKIP_FRAME;
        }
        break;

    case TRACE_STATE_TARGET_SOURCE:
        putchar(c);
        trace->state = TRACE_STATE_IDLE;
        break;

    case TRACE_STATE_SKIP_FRAME:
        if (TRACE_OP_GET_CONTINUATION(c) == 0)
            trace->state = TRACE_STATE_IDLE;
        break;

    case TRACE_STATE_SKIP_4:
        trace->state = TRACE_STATE_SKIP_3;
        break;

    case TRACE_STATE_SKIP_3:
        trace->state = TRACE_STATE_SKIP_2;
        break;

    case TRACE_STATE_SKIP_2:
        trace->state = TRACE_STATE_SKIP_1;
        break;

    case TRACE_STATE_SKIP_1:
        trace->state = TRACE_STATE_IDLE;
        break;

    default:
        ELOG("Invalid state %d.  This should never happen\n", trace->state);
        trace->state = TRACE_STATE_IDLE;
    }
}

static bool ReadTrace(stlink_t* stlink, st_trace_t* trace) {
    uint8_t buffer[STLINK_TRACE_BUF_LEN];
    int length = stlink_trace_read(stlink, buffer, sizeof(buffer));

    if (length < 0) {
        ELOG("Error reading trace (%d)\n", length);
        return false;
    }

    if (length == 0) {
        usleep(1000);
        return true;
    }

    DLOG("Trace Length: %d\n", length);
    for (int i = 0; i < length; i++) {
        UpdateTrace(trace, buffer[i]);
    }

    return true;
}

int main(int argc, char** argv)
{
    st_settings_t settings;
    stlink_t* stlink = NULL;

    signal(SIGINT, &cleanup);
    signal(SIGTERM, &cleanup);
    signal(SIGSEGV, &cleanup);

    if (!parse_options(argc, argv, &settings)) {
        usage();
        return APP_RESULT_INVALID_PARAMS;
    }

    DLOG("show_help = %s\n", settings.show_help ? "true" : "false");
    DLOG("show_version = %s\n", settings.show_version ? "true" : "false");
    DLOG("logging_level = %d\n", settings.logging_level);
    DLOG("core_frequency = %d MHz\n", settings.core_frequency_mhz);
    DLOG("reset_board = %s\n", settings.reset_board ? "true" : "false");
    DLOG("force = %s\n", settings.force ? "true" : "false");
    DLOG("serial_number = %s\n", settings.serial_number ? settings.serial_number : "any");

    if (settings.show_help) {
        usage();
        return APP_RESULT_SUCCESS;
    }

    if (settings.show_version) {
        printf("v%s\n", STLINK_VERSION);
        return APP_RESULT_SUCCESS;
    }

    stlink = StLinkConnect(&settings);
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

    if (!IsDeviceTraceSupported(stlink->chip_id)) {
        const struct stlink_chipid_params *params = stlink_chipid_get_params(stlink->chip_id);
        ELOG("We do not support SWO output for device '%s'\n", params->description);
        if (!settings.force)
            return APP_RESULT_STLINK_UNSUPPORTED_DEVICE;
    }

    if (!EnableTrace(stlink, &settings)) {
        ELOG("Unable to enable trace mode\n");
        if (!settings.force)
            return APP_RESULT_STLINK_STATE_ERROR;
    }

    if (stlink_run(stlink)) {
        ELOG("Unable to run device\n");
        if (!settings.force)
            return APP_RESULT_STLINK_STATE_ERROR;
    }

    ILOG("Reading Trace\n");
    st_trace_t trace = {};
    while (!g_done && ReadTrace(stlink, &trace)) {
        // TODO: Check that things are roughly working.
    }

    stlink_trace_disable(stlink);
    stlink_close(stlink);

    return APP_RESULT_SUCCESS;
}

