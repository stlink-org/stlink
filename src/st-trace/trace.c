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

#define APP_RESULT_SUCCESS          0
#define APP_RESULT_INVALID_PARAMS   1
#define APP_RESULT_STLINK_NOT_FOUND 2


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

int main(int argc, char** argv)
{
	st_settings_t settings;
	stlink_t* link = NULL;

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

	link = StLinkConnect(&settings);
	if (!link) {
		ELOG("Unable to locate st-link\n");
		return APP_RESULT_STLINK_NOT_FOUND;
	}
		
	return APP_RESULT_SUCCESS;
}

