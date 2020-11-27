#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <getopt.h>

#include <stlink.h>
#include <logging.h>


#define DEFAULT_LOGGING_LEVEL 50
#define DEBUG_LOGGING_LEVEL 100

#define APP_RESULT_SUCCESS                   0
#define APP_RESULT_INVALID_PARAMS            1
#define APP_RESULT_STLINK_NOT_FOUND          2
#define APP_RESULT_STLINK_MISSING_DEVICE     3
#define APP_RESULT_STLINK_UNSUPPORTED_DEVICE 4
#define APP_RESULT_STLINK_STATE_ERROR        5


struct _st_settings_t
{
	bool  show_help;
	bool  show_version;
	int   logging_level;
	int   core_frequency_mhz;
	bool  reset_board;
	char* serial_number;
	bool  wait_sync;
};
typedef struct _st_settings_t st_settings_t;


static void usage(void) {
	puts("st-trace - usage:");
	puts("  -h, --help            Print this help");
	puts("  -V, --version         Print this version");
	puts("  -vXX, --verbose=XX    Specify a specific verbosity level (0..99)");
	puts("  -v, --verbose         Specify a generally verbose logging");
	puts("  -cXX, --clock=XX      Specify the core frequency in MHz");
	puts("  -n, --no-reset        Do not reset board on connection");
	puts("  -sXX, --serial=XX     Use a specific serial number");
	puts("  -a, --no-sync         Do not wait for a sync packet");
}

bool parse_options(int argc, char** argv, st_settings_t *settings) {

	static struct option long_options[] = {
		{"help",     no_argument,       NULL, 'h'},
		{"version",  no_argument,       NULL, 'V'},
		{"verbose",  optional_argument, NULL, 'v'},
		{"clock",    required_argument, NULL, 'c'},
		{"no-reset", no_argument,       NULL, 'n'},
		{"serial",   required_argument, NULL, 's'},
		{"no-sync",  no_argument,       NULL, 'a'},
		{0, 0, 0, 0},
	};
	int option_index = 0;
	int c;

	settings->show_help = false;
	settings->show_version = false;
	settings->logging_level = DEFAULT_LOGGING_LEVEL;
	settings->core_frequency_mhz = 0;
	settings->reset_board = true;
	settings->serial_number = NULL;
	settings->wait_sync = true;
	ugly_init(settings->logging_level);

	while ((c = getopt_long(argc, argv, "hVv::c:ns:a", long_options, &option_index)) != -1) {
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
		case 's':
			settings->serial_number = optarg;
			break;
		case 'a':
			settings->wait_sync = false;
			break;
		case '?':
			return false;
			break;
		default:
			ELOG("Unknown command line option: '%c' (0x%02x)\n", c, c);
			return false;
		}
	}

	if (optind < argc) {
		while (optind < argc) {
			ELOG("Unknown command line argument: '%s'\n", argv[optind++]);
		}
		return false;
	}

	return true;
}

static stlink_t* StLinkConnect(const st_settings_t* settings) {
	if (settings->serial_number) {
		char serial_number[STLINK_SERIAL_MAX_SIZE] = { 0 };
		size_t length = 0;
		for (uint32_t n = 0; n < strlen(settings->serial_number) && length < STLINK_SERIAL_MAX_SIZE; n += 2) {
			char buffer[3] = { 0 };
			memcpy(buffer, settings->serial_number + n, 2);
			serial_number[length++] = (uint8_t)strtol(buffer, NULL, 16);
		}
		return stlink_open_usb(settings->logging_level, false, serial_number, 0);
	} else {
		return stlink_open_usb(settings->logging_level, false, NULL, 0);
	}
}

// TODO: Move this to device table.
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

static bool EnableTrace(stlink_t* stlink, int core_frequency_mhz, bool reset_board) {

	if (stlink_force_debug(stlink)) {
		return false;
	}

	if (reset_board && stlink_reset(stlink)) {
		return false;
	}

	// The remainder of the items in this function were taken directly from https://github.com/avrhack/stlink-trace/blob/master/stlink-trace.c#L386

	// Set DHCSR to C_HALT and C_DEBUGEN
	stlink_write_debug32(stlink, 0xE000EDF0, 0xA05F0003);

	// Set TRCENA flag to enable global DWT and ITM
	stlink_write_debug32(stlink, 0xE000EDFC, 0x01000000);

	// Set FP_CTRL to enable write
	stlink_write_debug32(stlink, 0xE0002000, 0x00000002);

	// Set DWT_FUNCTION0 to DWT_FUNCTION3 to disable sampling
	stlink_write_debug32(stlink, 0xE0001028, 0x00000000);
	stlink_write_debug32(stlink, 0xE0001038, 0x00000000);
	stlink_write_debug32(stlink, 0xE0001048, 0x00000000);
	stlink_write_debug32(stlink, 0xE0001058, 0x00000000);

	// Clear DWT_CTRL and other registers
	stlink_write_debug32(stlink, 0xE0001000, 0x00000000);
	stlink_write_debug32(stlink, 0xE0001004, 0x00000000);
	stlink_write_debug32(stlink, 0xE0001008, 0x00000000);
	stlink_write_debug32(stlink, 0xE000100C, 0x00000000);
	stlink_write_debug32(stlink, 0xE0001010, 0x00000000);
	stlink_write_debug32(stlink, 0xE0001014, 0x00000000);
	stlink_write_debug32(stlink, 0xE0001018, 0x00000000);

	// We should be checking these regisers.
	struct stlink_reg regp = {};
	stlink_read_reg(stlink, 15, &regp); // Read program counter
	stlink_read_reg(stlink, 16, &regp); // Read xpsr register

	// Set DBGMCU_CR to enable asynchronous transmission
	stlink_write_debug32(stlink, 0xE0042004, 0x00000027);

	// Actually start tracing.
	stlink_trace_enable(stlink);

	// Set TPIU_CSPSR to enable trace port width of 2
	stlink_write_debug32(stlink, 0xE0040004, 0x00000001);

	if (core_frequency_mhz)
	// Set TPIU_ACPR clock divisor
	stlink_write_debug32(stlink, 0xE0040010, core_frequency_mhz / 2 - 1);

	// Set TPIU_SPPR to Asynchronous SWO (NRZ)
	stlink_write_debug32(stlink, 0xE00400F0, 0x00000002);

	// Set TPIU_FFCR continuous formatting)
	stlink_write_debug32(stlink, 0xE0040304, 0x00000100);

	// Unlock the ITM registers for write
	stlink_write_debug32(stlink, 0xE0000FB0, 0xC5ACCE55);

	// Set sync counter
	stlink_write_debug32(stlink, 0xE0000E90, 0x00000400);

	// Set ITM_TCR flags : ITMENA,TSENA ATB=0
	stlink_write_debug32(stlink, 0xE0000E80, 0x00010003);

	// Enable all trace ports in ITM_TER
	stlink_write_debug32(stlink, 0xE0000E00, 0xFFFFFFFF);

	// Enable unprivileged access to trace ports 31:0 in ITM_TPR
	stlink_write_debug32(stlink, 0xE0000E40, 0x0000000F);

	// Set DWT_CTRL flags)
	stlink_write_debug32(stlink, 0xE0000E40, 0x400003FE);		// Keil one

	// Enable tracing (DEMCR - TRCENA bit)
	stlink_write_debug32(stlink, 0xE000EDFC, 0x01000000);

	return true;
}

int main(int argc, char** argv)
{
	st_settings_t settings;
	stlink_t* stlink = NULL;

	if (!parse_options(argc, argv, &settings)) {
		usage();
		return APP_RESULT_INVALID_PARAMS;
	}

	DLOG("show_help = %s\n", settings.show_help ? "true" : "false");
	DLOG("show_version = %s\n", settings.show_version ? "true" : "false");
	DLOG("logging_level = %d\n", settings.logging_level);
	DLOG("core_frequency = %d MHz\n", settings.core_frequency_mhz);
	DLOG("reset_board = %s\n", settings.reset_board ? "true" : "false");
	DLOG("serial_number = %s\n", settings.serial_number ? settings.serial_number : "any");
	DLOG("wait_sync = %s\n", settings.wait_sync ? "true" : "false");

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
		ELOG("Unable to locate st-link\n");
		return APP_RESULT_STLINK_NOT_FOUND;
	}

	if (stlink->chip_id == STLINK_CHIPID_UNKNOWN) {
		ELOG("st-link not connected\n");
		return APP_RESULT_STLINK_MISSING_DEVICE;
	}

	// TODO: Check if trace is supported by stlink.

	if (!IsDeviceTraceSupported(stlink->chip_id)) {
		const struct stlink_chipid_params *params = stlink_chipid_get_params(stlink->chip_id);
		ELOG("We do not support SWO output for device '%s'", params->description);
		return APP_RESULT_STLINK_UNSUPPORTED_DEVICE;
	}

	if (!EnableTrace(stlink, settings.core_frequency_mhz, settings.reset_board)) {
		ELOG("Unable to enable trace mode\n");
		return APP_RESULT_STLINK_STATE_ERROR;
	}
 	
	if (stlink_run(stlink)) {
		ELOG("Unable to run device\n");
		return APP_RESULT_STLINK_STATE_ERROR;
	}


	// Read SWO output
	
	
	stlink_close(stlink);

	return APP_RESULT_SUCCESS;
}

