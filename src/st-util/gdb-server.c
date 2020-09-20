/*
 * Copyright (c) 2011 Peter Zotov <whitequark@whitequark.org>
 * Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
 */

#include <ctype.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#if defined(_MSC_VER)
#include <stdbool.h>
#define __attribute__(x)
#endif

#if defined(_WIN32)
#include <win32_socket.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <stlink.h>
#include <logging.h>
#include "gdb-remote.h"
#include "gdb-server.h"
#include "semihosting.h"

#define FLASH_BASE 0x08000000

// Semihosting doesn't have a short option, we define a value to identify it
#define SEMIHOSTING_OPTION 128
#define SERIAL_OPTION 127

// always update the FLASH_PAGE before each use, by calling stlink_calculate_pagesize
#define FLASH_PAGE (sl->flash_pgsz)

static stlink_t *connected_stlink = NULL;
static bool semihosting = false;
static bool serial_specified = false;
static char serialnumber[28] = {0};

#if defined(_WIN32)
#define close_socket win32_close_socket
#define IS_SOCK_VALID(__sock) ((__sock) != INVALID_SOCKET)
#else
#define close_socket close
#define SOCKET int
#define IS_SOCK_VALID(__sock) ((__sock) > 0)
#endif

static const char hex[] = "0123456789abcdef";

static const char* current_memory_map = NULL;

typedef struct _st_state_t {
    // things from command line, bleh
    int logging_level;
    int listen_port;
    int persistent;
    int reset;
} st_state_t;


int serve(stlink_t *sl, st_state_t *st);
char* make_memory_map(stlink_t *sl);
static void init_cache(stlink_t *sl);

static void _cleanup() {
    if (connected_stlink) {
        // Switch back to mass storage mode before closing
        stlink_run(connected_stlink);
        stlink_exit_debug_mode(connected_stlink);
        stlink_close(connected_stlink);
    }
}

static void cleanup(int signum) {
    printf("Receive signal %i. Exiting...\n", signum);
    _cleanup();
    exit(1);
    (void)signum;
}

#if defined(_WIN32)
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    printf("Receive signal %i. Exiting...\r\n", (int)fdwCtrlType);
    _cleanup();
    return FALSE;
}
#endif


static stlink_t* do_connect(st_state_t *st) {
    stlink_t *sl = NULL;

    if (serial_specified) {
        sl = stlink_open_usb(st->logging_level, st->reset, serialnumber, 0);
    } else {
        sl = stlink_open_usb(st->logging_level, st->reset, NULL, 0);
    }

    return(sl);
}


int parse_options(int argc, char** argv, st_state_t *st) {
    static struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"verbose", optional_argument, NULL, 'v'},
        {"listen_port", required_argument, NULL, 'p'},
        {"multi", optional_argument, NULL, 'm'},
        {"no-reset", optional_argument, NULL, 'n'},
        {"version", no_argument, NULL, 'V'},
        {"semihosting", no_argument, NULL, SEMIHOSTING_OPTION},
        {"serial", required_argument, NULL, SERIAL_OPTION},
        {0, 0, 0, 0},
    };
    const char * help_str = "%s - usage:\n\n"
                            "  -h, --help\t\tPrint this help\n"
                            "  -V, --version\t\tPrint the version\n"
                            "  -vXX, --verbose=XX\tSpecify a specific verbosity level (0..99)\n"
                            "  -v, --verbose\t\tSpecify generally verbose logging\n"
                            "\t\t\tChoose what version of stlink to use, (defaults to 2)\n"
                            "  -1, --stlinkv1\tForce stlink version 1\n"
                            "  -p 4242, --listen_port=1234\n"
                            "\t\t\tSet the gdb server listen port. "
                            "(default port: " STRINGIFY(DEFAULT_GDB_LISTEN_PORT) ")\n"
                            "  -m, --multi\n"
                            "\t\t\tSet gdb server to extended mode.\n"
                            "\t\t\tst-util will continue listening for connections after disconnect.\n"
                            "  -n, --no-reset\n"
                            "\t\t\tDo not reset board on connection.\n"
                            "  --semihosting\n"
                            "\t\t\tEnable semihosting support.\n"
                            "  --serial <serial>\n"
                            "\t\t\tUse a specific serial number.\n"
                            "\n"
                            "The STLINK device to use can be specified in the environment\n"
                            "variable STLINK_DEVICE on the format <USB_BUS>:<USB_ADDR>.\n"
                            "\n"
    ;


    int option_index = 0;
    int c;
    int q;

    while ((c = getopt_long(argc, argv, "hv::p:mn", long_options, &option_index)) != -1)
        switch (c) {
        case 0:
            break;
        case 'h':
            printf(help_str, argv[0]);
            exit(EXIT_SUCCESS);
            break;
        case 'v':

            if (optarg) {
                st->logging_level = atoi(optarg);
            } else {
                st->logging_level = DEBUG_LOGGING_LEVEL;
            }

            break;
        case 'p':
            sscanf(optarg, "%i", &q);

            if (q < 0) {
                fprintf(stderr, "Can't use a negative port to listen on: %d\n", q);
                exit(EXIT_FAILURE);
            }

            st->listen_port = q;
            break;
        case 'm':
            st->persistent = 1;
            break;
        case 'n':
            st->reset = 0;
            break;
        case 'V':
            printf("v%s\n", STLINK_VERSION);
            exit(EXIT_SUCCESS);
        case SEMIHOSTING_OPTION:
            semihosting = true;
            break;
        case SERIAL_OPTION:
            printf("use serial %s\n", optarg);
            /* TODO: This is not really portable, as strlen really returns size_t,
             * we need to obey and not cast it to a signed type.
             */
            int j = (int)strlen(optarg);
            int length = j / 2;      // the length of the destination-array

            if (j % 2 != 0) { return(-1); }

            for (size_t k = 0; j >= 0 && k < sizeof(serialnumber); ++k, j -= 2) {
                char buffer[3] = {0};
                memcpy(buffer, optarg + j, 2);
                serialnumber[length - k] = (uint8_t)strtol(buffer, NULL, 16);
            }

            serial_specified = true;
            break;
        }


    if (optind < argc) {
        printf("non-option ARGV-elements: ");

        while (optind < argc) { printf("%s ", argv[optind++]); }

        printf("\n");
    }

    return(0);
}

int main(int argc, char** argv) {
    stlink_t *sl = NULL;
    st_state_t state;
    memset(&state, 0, sizeof(state));

    // set defaults ...
    state.logging_level = DEFAULT_LOGGING_LEVEL;
    state.listen_port = DEFAULT_GDB_LISTEN_PORT;
    state.reset = 1; // by default, reset board
    parse_options(argc, argv, &state);

    printf("st-util\n");

    sl = do_connect(&state);

    if (sl == NULL) { return(1); }

    if (sl->chip_id == STLINK_CHIPID_UNKNOWN) {
        ELOG("Unsupported Target (Chip ID is %#010x, Core ID is %#010x).\n", sl->chip_id, sl->core_id);
        return(1);
    }

    connected_stlink = sl;
#if defined(_WIN32)
    SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlHandler, TRUE);
#else
    signal(SIGINT, &cleanup);
    signal(SIGTERM, &cleanup);
    signal(SIGSEGV, &cleanup);
#endif

    if (state.reset) { stlink_reset(sl); }

    DLOG("Chip ID is %#010x, Core ID is %#08x.\n", sl->chip_id, sl->core_id);

    sl->verbose = 0;
    current_memory_map = make_memory_map(sl);

#if defined(_WIN32)
    WSADATA wsadata;

    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) { goto winsock_error; }

#endif

    init_cache(sl);

    do {                       // don't go beserk if serve() returns with error
        if (serve(sl, &state)) { usleep (1 * 1000); }

        sl = connected_stlink; // in case serve() changed the connection
        stlink_run(sl);        // continue
    } while (state.persistent);

#if defined(_WIN32)
winsock_error:
    WSACleanup();
#endif

    // switch back to mass storage mode before closing
    stlink_exit_debug_mode(sl);
    stlink_close(sl);

    return(0);
}

static const char* const target_description_F4 =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
    "<target version=\"1.0\">"
    "   <architecture>arm</architecture>"
    "   <feature name=\"org.gnu.gdb.arm.m-profile\">"
    "       <reg name=\"r0\" bitsize=\"32\"/>"
    "       <reg name=\"r1\" bitsize=\"32\"/>"
    "       <reg name=\"r2\" bitsize=\"32\"/>"
    "       <reg name=\"r3\" bitsize=\"32\"/>"
    "       <reg name=\"r4\" bitsize=\"32\"/>"
    "       <reg name=\"r5\" bitsize=\"32\"/>"
    "       <reg name=\"r6\" bitsize=\"32\"/>"
    "       <reg name=\"r7\" bitsize=\"32\"/>"
    "       <reg name=\"r8\" bitsize=\"32\"/>"
    "       <reg name=\"r9\" bitsize=\"32\"/>"
    "       <reg name=\"r10\" bitsize=\"32\"/>"
    "       <reg name=\"r11\" bitsize=\"32\"/>"
    "       <reg name=\"r12\" bitsize=\"32\"/>"
    "       <reg name=\"sp\" bitsize=\"32\" type=\"data_ptr\"/>"
    "       <reg name=\"lr\" bitsize=\"32\"/>"
    "       <reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/>"
    "       <reg name=\"xpsr\" bitsize=\"32\" regnum=\"25\"/>"
    "       <reg name=\"msp\" bitsize=\"32\" regnum=\"26\" type=\"data_ptr\" group=\"general\" />"
    "       <reg name=\"psp\" bitsize=\"32\" regnum=\"27\" type=\"data_ptr\" group=\"general\" />"
    "       <reg name=\"control\" bitsize=\"8\" regnum=\"28\" type=\"int\" group=\"general\" />"
    "       <reg name=\"faultmask\" bitsize=\"8\" regnum=\"29\" type=\"int\" group=\"general\" />"
    "       <reg name=\"basepri\" bitsize=\"8\" regnum=\"30\" type=\"int\" group=\"general\" />"
    "       <reg name=\"primask\" bitsize=\"8\" regnum=\"31\" type=\"int\" group=\"general\" />"
    "       <reg name=\"s0\" bitsize=\"32\" regnum=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s1\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s2\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s3\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s4\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s5\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s6\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s7\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s8\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s9\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s10\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s11\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s12\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s13\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s14\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s15\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s16\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s17\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s18\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s19\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s20\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s21\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s22\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s23\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s24\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s25\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s26\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s27\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s28\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s29\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s30\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"s31\" bitsize=\"32\" type=\"float\" group=\"float\" />"
    "       <reg name=\"fpscr\" bitsize=\"32\" type=\"int\" group=\"float\" />"
    "   </feature>"
    "</target>";

static const char* const memory_map_template_F4 =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x100000\"/>"     // code = sram, bootrom or flash; flash is bigger
    "  <memory type=\"ram\" start=\"0x10000000\" length=\"0x10000\"/>"      // ccm ram
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x20000\"/>"      // sram
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x10000\">"     // Sectors 0..3
    "    <property name=\"blocksize\">0x4000</property>"                    // 16kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08010000\" length=\"0x10000\">"     // Sector 4
    "    <property name=\"blocksize\">0x10000</property>"                   // 64kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08020000\" length=\"0xE0000\">"     // Sectors 5..11
    "    <property name=\"blocksize\">0x20000</property>"                   // 128kB
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0x60000000\" length=\"0x7fffffff\"/>"   // AHB3 Peripherals
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x1fff0000\" length=\"0x7800\"/>"       // bootrom
    "  <memory type=\"rom\" start=\"0x1fffc000\" length=\"0x10\"/>"         // option byte area
    "</memory-map>";

static const char* const memory_map_template_F4_HD =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x100000\"/>"     // code = sram, bootrom or flash; flash is bigger
    "  <memory type=\"ram\" start=\"0x10000000\" length=\"0x10000\"/>"      // ccm ram
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x40000\"/>"      // sram
    "  <memory type=\"ram\" start=\"0x60000000\" length=\"0x10000000\"/>"   // fmc bank 1 (nor/psram/sram)
    "  <memory type=\"ram\" start=\"0x70000000\" length=\"0x20000000\"/>"   // fmc bank 2 & 3 (nand flash)
    "  <memory type=\"ram\" start=\"0x90000000\" length=\"0x10000000\"/>"   // fmc bank 4 (pc card)
    "  <memory type=\"ram\" start=\"0xC0000000\" length=\"0x20000000\"/>"   // fmc sdram bank 1 & 2
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x10000\">"     // Sectors 0..3
    "    <property name=\"blocksize\">0x4000</property>"                    // 16kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08010000\" length=\"0x10000\">"     // Sector 4
    "    <property name=\"blocksize\">0x10000</property>"                   // 64kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08020000\" length=\"0xE0000\">"     // Sectors 5..11
    "    <property name=\"blocksize\">0x20000</property>"                   // 128kB
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x1fff0000\" length=\"0x7800\"/>"       // bootrom
    "  <memory type=\"rom\" start=\"0x1fffc000\" length=\"0x10\"/>"         // option byte area
    "</memory-map>";

static const char* const memory_map_template_F2 =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x%x\"/>"         // code = sram, bootrom or flash; flash is bigger
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x%x\"/>"         // sram
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x10000\">"     // Sectors 0..3
    "    <property name=\"blocksize\">0x4000</property>"                    // 16kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08010000\" length=\"0x10000\">"     // Sector 4
    "    <property name=\"blocksize\">0x10000</property>"                   // 64kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08020000\" length=\"0x%x\">"        // Sectors 5..
    "    <property name=\"blocksize\">0x20000</property>"                   // 128kB
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x%08x\" length=\"0x%x\"/>"             // bootrom
    "  <memory type=\"rom\" start=\"0x1fffc000\" length=\"0x10\"/>"         // option byte area
    "</memory-map>";

static const char* const memory_map_template_L4 =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x%x\"/>"         // code = sram, bootrom or flash; flash is bigger
    "  <memory type=\"ram\" start=\"0x10000000\" length=\"0x8000\"/>"       // SRAM2 (32kB)
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x18000\"/>"      // SRAM1 (96kB)
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x%x\">"
    "    <property name=\"blocksize\">0x800</property>"
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0x60000000\" length=\"0x7fffffff\"/>"   // AHB3 Peripherals
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x1fff0000\" length=\"0x7000\"/>"       // bootrom
    "  <memory type=\"rom\" start=\"0x1fff7800\" length=\"0x10\"/>"         // option byte area
    "  <memory type=\"rom\" start=\"0x1ffff800\" length=\"0x10\"/>"         // option byte area
    "</memory-map>";

static const char* const memory_map_template_L496 =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x%x\"/>"         // code = sram, bootrom or flash; flash is bigger
    "  <memory type=\"ram\" start=\"0x10000000\" length=\"0x10000\"/>"      // SRAM2 (64kB)
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x50000\"/>"      // SRAM1 + aliased SRAM2 (256 + 64 = 320kB)
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x%x\">"
    "    <property name=\"blocksize\">0x800</property>"
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0x60000000\" length=\"0x7fffffff\"/>"   // AHB3 Peripherals
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x1fff0000\" length=\"0x7000\"/>"       // bootrom
    "  <memory type=\"rom\" start=\"0x1fff7800\" length=\"0x10\"/>"         // option byte area
    "  <memory type=\"rom\" start=\"0x1ffff800\" length=\"0x10\"/>"         // option byte area
    "</memory-map>";

static const char* const memory_map_template =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x%x\"/>"         // code = sram, bootrom or flash; flash is bigger
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x%x\"/>"         // sram 8kB
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x%x\">"
    "    <property name=\"blocksize\">0x%x</property>"
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x%08x\" length=\"0x%x\"/>"             // bootrom
    "  <memory type=\"rom\" start=\"0x1ffff800\" length=\"0x10\"/>"         // option byte area
    "</memory-map>";

static const char* const memory_map_template_F7 =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"ram\" start=\"0x00000000\" length=\"0x4000\"/>"       // ITCM ram 16kB
    "  <memory type=\"rom\" start=\"0x00200000\" length=\"0x100000\"/>"     // ITCM flash
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x%x\"/>"         // sram
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x20000\">"     // Sectors 0..3
    "    <property name=\"blocksize\">0x8000</property>"                    // 32kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08020000\" length=\"0x20000\">"     // Sector 4
    "    <property name=\"blocksize\">0x20000</property>"                   // 128kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08040000\" length=\"0xC0000\">"     // Sectors 5..7
    "    <property name=\"blocksize\">0x40000</property>"                   // 128kB
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0x60000000\" length=\"0x7fffffff\"/>"   // AHB3 Peripherals
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x00100000\" length=\"0xEDC0\"/>"       // bootrom
    "  <memory type=\"rom\" start=\"0x1fff0000\" length=\"0x20\"/>"         // option byte area
    "</memory-map>";


static const char* const memory_map_template_F4_DE =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x80000\"/>"      // code = sram, bootrom or flash; flash is bigger
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x18000\"/>"      // sram
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x10000\">"     // Sectors 0..3
    "    <property name=\"blocksize\">0x4000</property>"                    // 16kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08010000\" length=\"0x10000\">"     // Sector 4
    "    <property name=\"blocksize\">0x10000</property>"                   // 64kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08020000\" length=\"0x60000\">"     // Sectors 5..7
    "    <property name=\"blocksize\">0x20000</property>"                   // 128kB
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x1fff0000\" length=\"0x7800\"/>"       // bootrom
    "  <memory type=\"rom\" start=\"0x1fff7800\" length=\"0x210\"/>"        // otp
    "  <memory type=\"rom\" start=\"0x1fffc000\" length=\"0x10\"/>"         // option byte area
    "</memory-map>";

char* make_memory_map(stlink_t *sl) {
    // this will be freed in serve()
    const size_t sz = 4096;
    char* map = malloc(sz);
    map[0] = '\0';

    if (sl->chip_id == STLINK_CHIPID_STM32_F4 ||
        sl->chip_id == STLINK_CHIPID_STM32_F446 ||
        sl->chip_id == STLINK_CHIPID_STM32_F411RE) {
            strcpy(map, memory_map_template_F4);
    } else if (sl->chip_id == STLINK_CHIPID_STM32_F4_DE) {
        strcpy(map, memory_map_template_F4_DE);
    } else if (sl->core_id == STM32F7_CORE_ID) {
        snprintf(map, sz, memory_map_template_F7,
                 (unsigned int)sl->sram_size);
    } else if (sl->chip_id == STLINK_CHIPID_STM32_F4_HD) {
        strcpy(map, memory_map_template_F4_HD);
    } else if (sl->chip_id == STLINK_CHIPID_STM32_F2) {
        snprintf(map, sz, memory_map_template_F2,
                 (unsigned int)sl->flash_size,
                 (unsigned int)sl->sram_size,
                 (unsigned int)sl->flash_size - 0x20000,
                 (unsigned int)sl->sys_base,
                 (unsigned int)sl->sys_size);
    } else if ((sl->chip_id == STLINK_CHIPID_STM32_L4) ||
               (sl->chip_id == STLINK_CHIPID_STM32_L43X) ||
               (sl->chip_id == STLINK_CHIPID_STM32_L46X)) {
        snprintf(map, sz, memory_map_template_L4,
                 (unsigned int)sl->flash_size,
                 (unsigned int)sl->flash_size);
    } else if (sl->chip_id == STLINK_CHIPID_STM32_L496X) {
        snprintf(map, sz, memory_map_template_L496,
                 (unsigned int)sl->flash_size,
                 (unsigned int)sl->flash_size);
    } else {
        snprintf(map, sz, memory_map_template,
                 (unsigned int)sl->flash_size,
                 (unsigned int)sl->sram_size,
                 (unsigned int)sl->flash_size,
                 (unsigned int)sl->flash_pgsz,
                 (unsigned int)sl->sys_base,
                 (unsigned int)sl->sys_size);
    }

    return(map);
}

/*
 * DWT_COMP0     0xE0001020
 * DWT_MASK0     0xE0001024
 * DWT_FUNCTION0 0xE0001028
 * DWT_COMP1     0xE0001030
 * DWT_MASK1     0xE0001034
 * DWT_FUNCTION1 0xE0001038
 * DWT_COMP2     0xE0001040
 * DWT_MASK2     0xE0001044
 * DWT_FUNCTION2 0xE0001048
 * DWT_COMP3     0xE0001050
 * DWT_MASK3     0xE0001054
 * DWT_FUNCTION3 0xE0001058
 */

#define DATA_WATCH_NUM 4

enum watchfun { WATCHDISABLED = 0, WATCHREAD = 5, WATCHWRITE = 6, WATCHACCESS = 7 };

struct code_hw_watchpoint {
    stm32_addr_t addr;
    uint8_t mask;
    enum watchfun fun;
};

static struct code_hw_watchpoint data_watches[DATA_WATCH_NUM];

static void init_data_watchpoints(stlink_t *sl) {
    uint32_t data;
    DLOG("init watchpoints\n");

    stlink_read_debug32(sl, 0xE000EDFC, &data);
    data |= 1 << 24;
    // set trcena in debug command to turn on dwt unit
    stlink_write_debug32(sl, 0xE000EDFC, data);

    // make sure all watchpoints are cleared
    for (int i = 0; i < DATA_WATCH_NUM; i++) {
        data_watches[i].fun = WATCHDISABLED;
        stlink_write_debug32(sl, 0xe0001028 + i * 16, 0);
    }
}

static int add_data_watchpoint(stlink_t *sl, enum watchfun wf, stm32_addr_t addr, unsigned int len) {
    int i = 0;
    uint32_t mask, dummy;

    // computer mask
    // find a free watchpoint
    // configure

    mask = -1;
    i = len;

    while (i) {
        i >>= 1;
        mask++;
    }

    if ((mask != (uint32_t)-1) && (mask < 16)) {
        for (i = 0; i < DATA_WATCH_NUM; i++)
            // is this an empty slot ?
            if (data_watches[i].fun == WATCHDISABLED) {
                DLOG("insert watchpoint %d addr %x wf %u mask %u len %d\n", i, addr, wf, mask, len);

                data_watches[i].fun = wf;
                data_watches[i].addr = addr;
                data_watches[i].mask = mask;

                // insert comparator address
                stlink_write_debug32(sl, 0xE0001020 + i * 16, addr);

                // insert mask
                stlink_write_debug32(sl, 0xE0001024 + i * 16, mask);

                // insert function
                stlink_write_debug32(sl, 0xE0001028 + i * 16, wf);

                // just to make sure the matched bit is clear !
                stlink_read_debug32(sl,  0xE0001028 + i * 16, &dummy);
                return(0);
            }
    }

    DLOG("failure: add watchpoints addr %x wf %u len %u\n", addr, wf, len);
    return(-1);
}

static int delete_data_watchpoint(stlink_t *sl, stm32_addr_t addr) {
    int i;

    for (i = 0; i < DATA_WATCH_NUM; i++) {
        if ((data_watches[i].addr == addr) && (data_watches[i].fun != WATCHDISABLED)) {
            DLOG("delete watchpoint %d addr %x\n", i, addr);

            data_watches[i].fun = WATCHDISABLED;
            stlink_write_debug32(sl, 0xe0001028 + i * 16, 0);

            return(0);
        }
    }

    DLOG("failure: delete watchpoint addr %x\n", addr);

    return(-1);
}

static int code_break_num;
static int code_lit_num;
#define CODE_BREAK_NUM_MAX 15
#define CODE_BREAK_LOW     0x01
#define CODE_BREAK_HIGH    0x02

struct code_hw_breakpoint {
    stm32_addr_t addr;
    int type;
};

static struct code_hw_breakpoint code_breaks[CODE_BREAK_NUM_MAX];

static void init_code_breakpoints(stlink_t *sl) {
    unsigned int val;
    memset(sl->q_buf, 0, 4);
    stlink_write_debug32(sl, STLINK_REG_CM3_FP_CTRL, 0x03 /* KEY | ENABLE4 */);
    stlink_read_debug32(sl, STLINK_REG_CM3_FP_CTRL, &val);
    code_break_num = ((val >> 4) & 0xf);
    code_lit_num = ((val >> 8) & 0xf);

    ILOG("Found %i hw breakpoint registers\n", code_break_num);

    for (int i = 0; i < code_break_num; i++) {
        code_breaks[i].type = 0;
        stlink_write_debug32(sl, STLINK_REG_CM3_FP_COMP0 + i * 4, 0);
    }
}

static int has_breakpoint(stm32_addr_t addr) {
    for (int i = 0; i < code_break_num; i++)
        if (code_breaks[i].addr == addr) { return(1); }

    return(0);
}

static int update_code_breakpoint(stlink_t *sl, stm32_addr_t addr, int set) {
    stm32_addr_t fpb_addr;
    uint32_t mask;
    int type = (addr & 0x2) ? CODE_BREAK_HIGH : CODE_BREAK_LOW;

    if (addr & 1) {
        ELOG("update_code_breakpoint: unaligned address %08x\n", addr);
        return(-1);
    }

    if (sl->core_id == STM32F7_CORE_ID) {
        fpb_addr = addr;
    } else {
        fpb_addr = addr & ~0x3;
    }

    int id = -1;

    for (int i = 0; i < code_break_num; i++)
        if (fpb_addr == code_breaks[i].addr || (set && code_breaks[i].type == 0)) {
            id = i;
            break;
        }


    if (id == -1) {
        if (set) {
            return(-1);
        } // free slot not found
        else {
            return(0);
        } // breakpoint is already removed

    }

    struct code_hw_breakpoint* bp = &code_breaks[id];

    bp->addr = fpb_addr;

    if (sl->core_id == STM32F7_CORE_ID) {
        if (set) {
            bp->type = type;
        } else {
            bp->type = 0;
        }

        mask = (bp->addr) | 1;
    } else {
        if (set) {
            bp->type |= type;
        } else {
            bp->type &= ~type;
        }

        mask = (bp->addr) | 1 | (bp->type << 30);
    }

    if (bp->type == 0) {
        DLOG("clearing hw break %d\n", id);
        stlink_write_debug32(sl, 0xe0002008 + id * 4, 0);
    } else {
        DLOG("setting hw break %d at %08x (%d)\n", id, bp->addr, bp->type);
        DLOG("reg %08x \n", mask);
        stlink_write_debug32(sl, 0xe0002008 + id * 4, mask);
    }

    return(0);
}


struct flash_block {
    stm32_addr_t addr;
    unsigned length;
    uint8_t*     data;

    struct flash_block* next;
};

static struct flash_block* flash_root;

static int flash_add_block(stm32_addr_t addr, unsigned length, stlink_t *sl) {

    if (addr < FLASH_BASE || addr + length > FLASH_BASE + sl->flash_size) {
        ELOG("flash_add_block: incorrect bounds\n");
        return(-1);
    }

    stlink_calculate_pagesize(sl, addr);

    if (addr % FLASH_PAGE != 0 || length % FLASH_PAGE != 0) {
        ELOG("flash_add_block: unaligned block\n");
        return(-1);
    }

    struct flash_block* new = malloc(sizeof(struct flash_block));
    new->next = flash_root;
    new->addr   = addr;
    new->length = length;
    new->data   = calloc(length, 1);

    flash_root = new;
    return(0);
}

static int flash_populate(stm32_addr_t addr, uint8_t* data, unsigned length) {
    unsigned int fit_blocks = 0, fit_length = 0;

    for (struct flash_block* fb = flash_root; fb; fb = fb->next) {
        /*
         * Block: ------X------Y--------
         * Data:            a-----b
         *                a--b
         *            a-----------b
         * Block intersects with data, if:
         *  a < Y && b > x
         */

        unsigned X = fb->addr, Y = fb->addr + fb->length;
        unsigned a = addr, b = addr + length;

        if (a < Y && b > X) {
            // from start of the block
            unsigned start = (a > X ? a : X) - X;
            unsigned end   = (b > Y ? Y : b) - X;

            memcpy(fb->data + start, data, end - start);

            fit_blocks++;
            fit_length += end - start;
        }
    }

    if (fit_blocks == 0) {
        ELOG("Unfit data block %08x -> %04x\n", addr, length);
        return(-1);
    }

    if (fit_length != length) {
        WLOG("data block %08x -> %04x truncated to %04x\n", addr, length, fit_length);
        WLOG("(this is not an error, just a GDB glitch)\n");
    }

    return(0);
}

static int flash_go(stlink_t *sl) {
    int error = -1;

    // some kinds of clock settings do not allow writing to flash.
    stlink_reset(sl);
    stlink_force_debug(sl);

    for (struct flash_block* fb = flash_root; fb; fb = fb->next) {
        DLOG("flash_do: block %08x -> %04x\n", fb->addr, fb->length);

        for (stm32_addr_t page = fb->addr; page < fb->addr + fb->length; page += (uint32_t)FLASH_PAGE) {
            unsigned length = fb->length - (page - fb->addr);

            // update FLASH_PAGE
            stlink_calculate_pagesize(sl, page);

            DLOG("flash_do: page %08x\n", page);
            unsigned len = (length > FLASH_PAGE) ? (unsigned int)FLASH_PAGE : length;
            int ret = stlink_write_flash(sl, page, fb->data + (page - fb->addr), len, 0);

            if (ret < 0) { goto error; }
        }
    }

    stlink_reset(sl);
    error = 0;

error:

    for (struct flash_block* fb = flash_root, *next; fb; fb = next) {
        next = fb->next;
        free(fb->data);
        free(fb);
    }

    flash_root = NULL;
    return(error);
}

#define CLIDR   0xE000ED78
#define CTR     0xE000ED7C
#define CCSIDR  0xE000ED80
#define CSSELR  0xE000ED84
#define CCR     0xE000ED14
#define CCR_DC  (1 << 16)
#define CCR_IC  (1 << 17)
#define DCCSW   0xE000EF6C
#define ICIALLU 0xE000EF50

struct cache_level_desc {
    unsigned int nsets;
    unsigned int nways;
    unsigned int log2_nways;
    unsigned int width;
};

struct cache_desc_t {
    // minimal line size in bytes
    unsigned int dminline;
    unsigned int iminline;

    // last level of unification (uniprocessor)
    unsigned int louu;

    struct cache_level_desc icache[7];
    struct cache_level_desc dcache[7];
};

static struct cache_desc_t cache_desc;

// return the smallest R so that V <= (1 << R); not performance critical
static unsigned ceil_log2(unsigned v) {
    unsigned res;

    for (res = 0; (1U << res) < v; res++);

    return(res);
}

static void read_cache_level_desc(stlink_t *sl, struct cache_level_desc *desc) {
    unsigned int ccsidr;
    unsigned int log2_nsets;

    stlink_read_debug32(sl, CCSIDR, &ccsidr);
    desc->nsets = ((ccsidr >> 13) & 0x3fff) + 1;
    desc->nways = ((ccsidr >> 3) & 0x1ff) + 1;
    desc->log2_nways = ceil_log2 (desc->nways);
    log2_nsets = ceil_log2 (desc->nsets);
    desc->width = 4 + (ccsidr & 7) + log2_nsets;
    ILOG("%08x LineSize: %u, ways: %u, sets: %u (width: %u)\n",
         ccsidr, 4 << (ccsidr & 7), desc->nways, desc->nsets, desc->width);
}

static void init_cache (stlink_t *sl) {
    unsigned int clidr;
    unsigned int ccr;
    unsigned int ctr;
    int i;

    // assume only F7 has a cache
    if (sl->core_id != STM32F7_CORE_ID) { return; }

    stlink_read_debug32(sl, CLIDR, &clidr);
    stlink_read_debug32(sl, CCR, &ccr);
    stlink_read_debug32(sl, CTR, &ctr);
    cache_desc.dminline = 4 << ((ctr >> 16) & 0x0f);
    cache_desc.iminline = 4 << (ctr & 0x0f);
    cache_desc.louu = (clidr >> 27) & 7;

    ILOG("Chip clidr: %08x, I-Cache: %s, D-Cache: %s\n",
         clidr, ccr & CCR_IC ? "on" : "off", ccr & CCR_DC ? "on" : "off");
    ILOG(" cache: LoUU: %u, LoC: %u, LoUIS: %u\n",
         (clidr >> 27) & 7, (clidr >> 24) & 7, (clidr >> 21) & 7);
    ILOG(" cache: ctr: %08x, DminLine: %u bytes, IminLine: %u bytes\n", ctr,
         cache_desc.dminline, cache_desc.iminline);

    for (i = 0; i < 7; i++) {
        unsigned int ct = (clidr >> (3 * i)) & 0x07;
        cache_desc.dcache[i].width = 0;
        cache_desc.icache[i].width = 0;

        if (ct == 2 || ct == 3 || ct == 4) { // data
            stlink_write_debug32(sl, CSSELR, i << 1);
            ILOG("D-Cache L%d: ", i);
            read_cache_level_desc(sl, &cache_desc.dcache[i]);
        }

        if (ct == 1 || ct == 3) { // instruction
            stlink_write_debug32(sl, CSSELR, (i << 1) | 1);
            ILOG("I-Cache L%d: ", i);
            read_cache_level_desc(sl, &cache_desc.icache[i]);
        }
    }
}

static void cache_flush(stlink_t *sl, unsigned ccr) {
    int level;

    if (ccr & CCR_DC) {
        for (level = cache_desc.louu - 1; level >= 0; level--) {
            struct cache_level_desc *desc = &cache_desc.dcache[level];
            unsigned addr;
            unsigned max_addr = 1 << desc->width;
            unsigned way_sh = 32 - desc->log2_nways;

            // D-cache clean by set-ways.
            for (addr = (level << 1); addr < max_addr; addr += cache_desc.dminline) {
                unsigned int way;

                for (way = 0; way < desc->nways; way++) {
                    stlink_write_debug32(sl, DCCSW, addr | (way << way_sh));
                }
            }
        }
    }

    // invalidate all I-cache to oPU
    if (ccr & CCR_IC) {
        stlink_write_debug32(sl, ICIALLU, 0);
    }
}

static int cache_modified;

static void cache_change(stm32_addr_t start, unsigned count) {
    if (count == 0) { return; }

    (void)start;
    cache_modified = 1;
}

static void cache_sync(stlink_t *sl) {
    unsigned ccr;

    if (sl->core_id != STM32F7_CORE_ID) { return; }

    if (!cache_modified) { return; }

    cache_modified = 0;
    stlink_read_debug32(sl, CCR, &ccr);

    if (ccr & (CCR_IC | CCR_DC)) { cache_flush(sl, ccr); }
}

static size_t unhexify(const char *in, char *out, size_t out_count) {
    size_t i;
    unsigned int c;

    for (i = 0; i < out_count; i++) {
        if (sscanf(in + (2 * i), "%02x", &c) != 1) { return(i); }

        out[i] = (char)c;
    }

    return(i);
}

int serve(stlink_t *sl, st_state_t *st) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    if (!IS_SOCK_VALID(sock)) {
        perror("socket");
        return(1);
    }

    unsigned int val = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(val));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(st->listen_port);

    if (bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close_socket(sock);
        return(1);
    }

    if (listen(sock, 5) < 0) {
        perror("listen");
        close_socket(sock);
        return(1);
    }

    ILOG("Listening at *:%d...\n", st->listen_port);

    SOCKET client = accept(sock, NULL, NULL);

    // signal (SIGINT, SIG_DFL);
    if (!IS_SOCK_VALID(client)) {
        perror("accept");
        close_socket(sock);
        return(1);
    }

    close_socket(sock);

    stlink_force_debug(sl);

    if (st->reset) { stlink_reset(sl); }

    init_code_breakpoints(sl);
    init_data_watchpoints(sl);

    ILOG("GDB connected.\n");

    /*
     * To allow resetting the chip from GDB it is required to emulate attaching
     * and detaching to target.
     */
    unsigned int attached = 1;
    // if a critical error is detected, break from the loop
    int critical_error = 0;
    int ret;

    while (1) {
        ret = 0;
        char* packet;

        int status = gdb_recv_packet(client, &packet);

        if (status < 0) {
            ELOG("cannot recv: %d\n", status);
            close_socket(client);
            return(1);
        }

        DLOG("recv: %s\n", packet);

        char* reply = NULL;
        struct stlink_reg regp;

        switch (packet[0]) {
        case 'q': {
            if (packet[1] == 'P' || packet[1] == 'C' || packet[1] == 'L') {
                reply = strdup("");
                break;
            }

            char *separator = strstr(packet, ":"), *params = "";

            if (separator == NULL) {
                separator = packet + strlen(packet);
            } else {
                params = separator + 1;
            }

            unsigned queryNameLength = (unsigned int)(separator - &packet[1]);
            char* queryName = calloc(queryNameLength + 1, 1);
            strncpy(queryName, &packet[1], queryNameLength);

            DLOG("query: %s;%s\n", queryName, params);

            if (!strcmp(queryName, "Supported")) {
                if (sl->chip_id == STLINK_CHIPID_STM32_F4 ||
                    sl->chip_id == STLINK_CHIPID_STM32_F4_HD ||
                    sl->core_id == STM32F7_CORE_ID) {
                    reply = strdup("PacketSize=3fff;qXfer:memory-map:read+;qXfer:features:read+");
                } else {
                    reply = strdup("PacketSize=3fff;qXfer:memory-map:read+");
                }
            } else if (!strcmp(queryName, "Xfer")) {
                char *type, *op, *__s_addr, *s_length;
                char *tok = params;
                char *annex __attribute__((unused));

                type     = strsep(&tok, ":");
                op       = strsep(&tok, ":");
                annex    = strsep(&tok, ":");
                __s_addr = strsep(&tok, ",");
                s_length = tok;

                unsigned addr = (unsigned int)strtoul(__s_addr, NULL, 16),
                         length = (unsigned int)strtoul(s_length, NULL, 16);

                DLOG("Xfer: type:%s;op:%s;annex:%s;addr:%d;length:%d\n",
                     type, op, annex, addr, length);

                const char* data = NULL;

                if (!strcmp(type, "memory-map") && !strcmp(op, "read")) {
                    data = current_memory_map;
                }

                if (!strcmp(type, "features") && !strcmp(op, "read")) {
                    data = target_description_F4;
                }

                if (data) {
                    unsigned data_length = (unsigned int)strlen(data);

                    if (addr + length > data_length) { length = data_length - addr; }

                    if (length == 0) {
                        reply = strdup("l");
                    } else {
                        reply = calloc(length + 2, 1);
                        reply[0] = 'm';
                        strncpy(&reply[1], data, length);
                    }
                }
            } else if (!strncmp(queryName, "Rcmd,", 4)) {
                // Rcmd uses the wrong separator
                separator = strstr(packet, ",");
                params = "";

                if (separator == NULL) {
                    separator = packet + strlen(packet);
                } else {
                    params = separator + 1;
                }

                size_t hex_len = strlen(params);
                size_t alloc_size = (hex_len / 2) + 1;
                size_t cmd_len;
                char *cmd = malloc(alloc_size);

                if (cmd == NULL) {
                    DLOG("Rcmd unhexify allocation error\n");
                    break;
                }

                cmd_len = unhexify(params, cmd, alloc_size - 1);
                cmd[cmd_len] = 0;

                DLOG("unhexified Rcmd: '%s'\n", cmd);

                if (!strncmp(cmd, "resume", 6)) {                               // resume
                    DLOG("Rcmd: resume\n");
                    cache_sync(sl);
                    ret = stlink_run(sl);

                    if (ret) {
                        DLOG("Rcmd: resume failed\n");
                        reply = strdup("E00");
                    } else {
                        reply = strdup("OK");
                    }

                } else if (!strncmp(cmd, "halt", 4)) {                          // halt
                    ret = stlink_force_debug(sl);

                    if (ret) {
                        DLOG("Rcmd: halt failed\n");
                        reply = strdup("E00");
                    } else {
                        reply = strdup("OK");
                        DLOG("Rcmd: halt\n");
                    }

                } else if (!strncmp(cmd, "jtag_reset", 10)) {                   // jtag_reset
                    reply = strdup("OK");
                    ret = stlink_jtag_reset(sl, 0);

                    if (ret) {
                        DLOG("Rcmd: jtag_reset failed with jtag_reset\n");
                        reply = strdup("E00");
                    }

                    ret = stlink_jtag_reset(sl, 1);

                    if (ret) {
                        DLOG("Rcmd: jtag_reset failed with jtag_reset\n");
                        reply = strdup("E00");
                    }

                    ret = stlink_force_debug(sl);

                    if (ret) {
                        DLOG("Rcmd: jtag_reset failed with force_debug\n");
                        reply = strdup("E00");
                    }

                    if (strcmp(reply, "E00")) {
                        // no errors have been found
                        DLOG("Rcmd: jtag_reset\n");
                    }
                } else if (!strncmp(cmd, "reset", 5)) {     // reset

                    ret = stlink_force_debug(sl);

                    if (ret) {
                        DLOG("Rcmd: reset failed with force_debug\n");
                        reply = strdup("E00");
                    }

                    ret = stlink_reset(sl);

                    if (ret) {
                        DLOG("Rcmd: reset failed with reset\n");
                        reply = strdup("E00");
                    }

                    init_code_breakpoints(sl);
                    init_data_watchpoints(sl);

                    if (reply == NULL) {
                        reply = strdup("OK");
                        DLOG("Rcmd: reset\n");
                    }

                } else if (!strncmp(cmd, "semihosting ", 12)) {
                    DLOG("Rcmd: got semihosting cmd '%s'", cmd);
                    char *arg = cmd + 12;

                    while (isspace(*arg)) { arg++; } // skip whitespaces

                    if (!strncmp(arg, "enable", 6) || !strncmp(arg, "1", 1)) {
                        semihosting = true;
                        reply = strdup("OK");
                    } else if (!strncmp(arg, "disable", 7) || !strncmp(arg, "0", 1)) {
                        semihosting = false;
                        reply = strdup("OK");
                    } else {
                        DLOG("Rcmd: unknown semihosting arg: '%s'\n", arg);
                    }
                } else {
                    DLOG("Rcmd: %s\n", cmd);
                }

                free(cmd);
            }

            if (reply == NULL) { reply = strdup(""); }

            free(queryName);
            break;
        }

        case 'v': {
            char *params = NULL;
            char *cmdName = strtok_r(packet, ":;", &params);

            cmdName++; // vCommand -> Command

            if (!strcmp(cmdName, "FlashErase")) {
                char *__s_addr, *s_length;
                char *tok = params;

                __s_addr   = strsep(&tok, ",");
                s_length = tok;

                unsigned addr = (unsigned int)strtoul(__s_addr, NULL, 16),
                         length = (unsigned int)strtoul(s_length, NULL, 16);

                DLOG("FlashErase: addr:%08x,len:%04x\n",
                     addr, length);

                if (flash_add_block(addr, length, sl) < 0) {
                    reply = strdup("E00");
                } else {
                    reply = strdup("OK");
                }
            } else if (!strcmp(cmdName, "FlashWrite")) {
                char *__s_addr, *data;
                char *tok = params;

                __s_addr = strsep(&tok, ":");
                data   = tok;

                unsigned addr = (unsigned int)strtoul(__s_addr, NULL, 16);
                unsigned data_length = status - (unsigned int)(data - packet);

                // Length of decoded data cannot be more than encoded, as escapes are removed.
                // Additional byte is reserved for alignment fix.
                uint8_t *decoded = calloc(data_length + 1, 1);
                unsigned dec_index = 0;

                for (unsigned int i = 0; i < data_length; i++) {
                    if (data[i] == 0x7d) {
                        i++;
                        decoded[dec_index++] = data[i] ^ 0x20;
                    } else {
                        decoded[dec_index++] = data[i];
                    }
                }

                // fix alignment
                if (dec_index % 2 != 0) { dec_index++; }

                DLOG("binary packet %d -> %d\n", data_length, dec_index);

                if (flash_populate(addr, decoded, dec_index) < 0) {
                    reply = strdup("E00");
                } else {
                    reply = strdup("OK");
                }

                free(decoded);
            } else if (!strcmp(cmdName, "FlashDone")) {
                if (flash_go(sl) < 0) {
                    reply = strdup("E00");
                } else {
                    reply = strdup("OK");
                }
            } else if (!strcmp(cmdName, "Kill")) {
                attached = 0;
                reply = strdup("OK");
            }

            if (reply == NULL) { reply = strdup(""); }

            break;
        }

        case 'c':
            cache_sync(sl);
            ret = stlink_run(sl);

            if (ret) { DLOG("Semihost: run failed\n"); }

            while (1) {
                status = gdb_check_for_interrupt(client);

                if (status < 0) {
                    ELOG("cannot check for int: %d\n", status);
                    close_socket(client);
                    return(1);
                }

                if (status == 1) {
                    stlink_force_debug(sl);
                    break;
                }

                ret = stlink_status(sl);

                if (ret) { DLOG("Semihost: status failed\n"); }

                if (sl->core_stat == TARGET_HALTED) {
                    struct stlink_reg reg;
                    stm32_addr_t pc;
                    stm32_addr_t addr;
                    int offset = 0;
                    uint16_t insn;

                    if (!semihosting) { break; }

                    ret = stlink_read_all_regs (sl, &reg);

                    if (ret) { DLOG("Semihost: read_all_regs failed\n"); }

                    // read PC
                    pc = reg.r[15];

                    // compute aligned value
                    offset = pc % 4;
                    addr = pc - offset;

                    // read instructions (address and length must be aligned).
                    ret = stlink_read_mem32(sl, addr, (offset > 2 ? 8 : 4));

                    if (ret != 0) {
                        DLOG("Semihost: cannot read instructions at: 0x%08x\n", addr);
                        break;
                    }

                    memcpy(&insn, &sl->q_buf[offset], sizeof(insn));

                    if (insn == 0xBEAB && !has_breakpoint(addr)) {

                        ret = do_semihosting (sl, reg.r[0], reg.r[1], &reg.r[0]);

                        if (ret) { DLOG("Semihost: do_semihosting failed\n"); }

                        // write return value
                        ret = stlink_write_reg(sl, reg.r[0], 0);

                        if (ret) { DLOG("Semihost: write_reg failed for return value\n"); }

                        // jump over the break instruction
                        ret = stlink_write_reg(sl, reg.r[15] + 2, 15);

                        if (ret) { DLOG("Semihost: write_reg failed for jumping over break\n"); }

                        // continue execution
                        cache_sync(sl);
                        ret = stlink_run(sl);

                        if (ret) { DLOG("Semihost: continue execution failed with stlink_run\n"); }
                    } else {
                        break;
                    }
                }

                usleep(100000);
            }

            reply = strdup("S05"); // TRAP
            break;

        case 's':
            cache_sync(sl);
            ret = stlink_step(sl);

            if (ret) {
                // ... having a problem sending step packet
                ELOG("Step: cannot send step request\n");
                reply = strdup("E00");
                critical_error = 1; // absolutely critical
            } else {
                reply = strdup("S05"); // TRAP
            }

            break;

        case '?':

            if (attached) {
                reply = strdup("S05"); // TRAP
            } else {
                reply = strdup("OK"); // stub shall reply OK if not attached
            }

            break;

        case 'g':
            ret = stlink_read_all_regs(sl, &regp);

            if (ret) { DLOG("g packet: read_all_regs failed\n"); }

            reply = calloc(8 * 16 + 1, 1);

            for (int i = 0; i < 16; i++) {
                sprintf(&reply[i * 8], "%08x", (uint32_t)htonl(regp.r[i]));
            }

            break;

        case 'p': {
            unsigned id = (unsigned int)strtoul(&packet[1], NULL, 16);
            unsigned myreg = 0xDEADDEAD;

            if (id < 16) {
                ret = stlink_read_reg(sl, id, &regp);
                myreg = htonl(regp.r[id]);
            } else if (id == 0x19) {
                ret = stlink_read_reg(sl, 16, &regp);
                myreg = htonl(regp.xpsr);
            } else if (id == 0x1A) {
                ret = stlink_read_reg(sl, 17, &regp);
                myreg = htonl(regp.main_sp);
            } else if (id == 0x1B) {
                ret = stlink_read_reg(sl, 18, &regp);
                myreg = htonl(regp.process_sp);
            } else if (id == 0x1C) {
                ret = stlink_read_unsupported_reg(sl, id, &regp);
                myreg = htonl(regp.control);
            } else if (id == 0x1D) {
                ret = stlink_read_unsupported_reg(sl, id, &regp);
                myreg = htonl(regp.faultmask);
            } else if (id == 0x1E) {
                ret = stlink_read_unsupported_reg(sl, id, &regp);
                myreg = htonl(regp.basepri);
            } else if (id == 0x1F) {
                ret = stlink_read_unsupported_reg(sl, id, &regp);
                myreg = htonl(regp.primask);
            } else if (id >= 0x20 && id < 0x40) {
                ret = stlink_read_unsupported_reg(sl, id, &regp);
                myreg = htonl(regp.s[id - 0x20]);
            } else if (id == 0x40) {
                ret = stlink_read_unsupported_reg(sl, id, &regp);
                myreg = htonl(regp.fpscr);
            } else {
                ret = 1;
                reply = strdup("E00");
            }

            if (ret) { DLOG("p packet: stlink_read_unsupported_reg failed with id %u\n", id); }

            if (reply == NULL) {
                // if reply is set to "E00", skip
                reply = calloc(8 + 1, 1);
                sprintf(reply, "%08x", myreg);
            }

            break;
        }

        case 'P': {
            char* s_reg = &packet[1];
            char* s_value = strstr(&packet[1], "=") + 1;

            unsigned reg   = (unsigned int)strtoul(s_reg,   NULL, 16);
            unsigned value = (unsigned int)strtoul(s_value, NULL, 16);


            if (reg < 16) {
                ret = stlink_write_reg(sl, ntohl(value), reg);
            } else if (reg == 0x19) {
                ret = stlink_write_reg(sl, ntohl(value), 16);
            } else if (reg == 0x1A) {
                ret = stlink_write_reg(sl, ntohl(value), 17);
            } else if (reg == 0x1B) {
                ret = stlink_write_reg(sl, ntohl(value), 18);
            } else if (reg == 0x1C) {
                ret = stlink_write_unsupported_reg(sl, ntohl(value), reg, &regp);
            } else if (reg == 0x1D) {
                ret = stlink_write_unsupported_reg(sl, ntohl(value), reg, &regp);
            } else if (reg == 0x1E) {
                ret = stlink_write_unsupported_reg(sl, ntohl(value), reg, &regp);
            } else if (reg == 0x1F) {
                ret = stlink_write_unsupported_reg(sl, ntohl(value), reg, &regp);
            } else if (reg >= 0x20 && reg < 0x40) {
                ret = stlink_write_unsupported_reg(sl, ntohl(value), reg, &regp);
            } else if (reg == 0x40) {
                ret = stlink_write_unsupported_reg(sl, ntohl(value), reg, &regp);
            } else {
                ret = 1;
                reply = strdup("E00");
            }

            if (ret) { DLOG("P packet: stlink_write_unsupported_reg failed with reg %u\n", reg); }

            if (reply == NULL) { reply = strdup("OK"); /* Note: NULL may not be zero */ }

            break;
        }

        case 'G':

            for (int i = 0; i < 16; i++) {
                char str[9] = {0};
                strncpy(str, &packet[1 + i * 8], 8);
                uint32_t reg = (uint32_t)strtoul(str, NULL, 16);
                ret = stlink_write_reg(sl, ntohl(reg), i);

                if (ret) { DLOG("G packet: stlink_write_reg failed"); }
            }

            reply = strdup("OK");
            break;

        case 'm': {
            char* s_start = &packet[1];
            char* s_count = strstr(&packet[1], ",") + 1;

            stm32_addr_t start = (stm32_addr_t)strtoul(s_start, NULL, 16);
            unsigned count = (unsigned int)strtoul(s_count, NULL, 16);

            unsigned adj_start = start % 4;
            unsigned count_rnd = (count + adj_start + 4 - 1) / 4 * 4;

            if (count_rnd > sl->flash_pgsz) { count_rnd = (unsigned int)sl->flash_pgsz; }

            if (count_rnd > 0x1800) { count_rnd = 0x1800; }

            if (count_rnd < count) { count = count_rnd; }

            if (stlink_read_mem32(sl, start - adj_start, count_rnd) != 0) { count = 0; }

            // read failed somehow, don't return stale buffer

            reply = calloc(count * 2 + 1, 1);

            for (unsigned int i = 0; i < count; i++) {
                reply[i * 2 + 0] = hex[sl->q_buf[i + adj_start] >> 4];
                reply[i * 2 + 1] = hex[sl->q_buf[i + adj_start] & 0xf];
            }

            break;
        }

        case 'M': {
            char* s_start = &packet[1];
            char* s_count = strstr(&packet[1], ",") + 1;
            char* hexdata = strstr(packet, ":") + 1;

            stm32_addr_t start = (stm32_addr_t)strtoul(s_start, NULL, 16);
            unsigned count = (unsigned int)strtoul(s_count, NULL, 16);
            int err = 0;

            if (start % 4) {
                unsigned align_count = 4 - start % 4;

                if (align_count > count) { align_count = count; }

                for (unsigned int i = 0; i < align_count; i++) {
                    char hextmp[3] = { hexdata[i * 2], hexdata[i * 2 + 1], 0 };
                    uint8_t byte = strtoul(hextmp, NULL, 16);
                    sl->q_buf[i] = byte;
                }

                err |= stlink_write_mem8(sl, start, align_count);
                cache_change(start, align_count);
                start += align_count;
                count -= align_count;
                hexdata += 2 * align_count;
            }

            if (count - count % 4) {
                unsigned aligned_count = count - count % 4;

                for (unsigned int i = 0; i < aligned_count; i++) {
                    char hextmp[3] = { hexdata[i * 2], hexdata[i * 2 + 1], 0 };
                    uint8_t byte = strtoul(hextmp, NULL, 16);
                    sl->q_buf[i] = byte;
                }

                err |= stlink_write_mem32(sl, start, aligned_count);
                cache_change(start, aligned_count);
                count -= aligned_count;
                start += aligned_count;
                hexdata += 2 * aligned_count;
            }

            if (count) {
                for (unsigned int i = 0; i < count; i++) {
                    char hextmp[3] = { hexdata[i * 2], hexdata[i * 2 + 1], 0 };
                    uint8_t byte = strtoul(hextmp, NULL, 16);
                    sl->q_buf[i] = byte;
                }

                err |= stlink_write_mem8(sl, start, count);
                cache_change(start, count);
            }

            reply = strdup(err ? "E00" : "OK");
            break;
        }

        case 'Z': {
            char *endptr;
            stm32_addr_t addr = (stm32_addr_t)strtoul(&packet[3], &endptr, 16);
            stm32_addr_t len  = (stm32_addr_t)strtoul(&endptr[1], NULL, 16);

            switch (packet[1]) {
            case '1':

                if (update_code_breakpoint(sl, addr, 1) < 0) {
                    reply = strdup("E00");
                } else {
                    reply = strdup("OK");
                }

                break;

            case '2':           // insert write watchpoint
            case '3':           // insert read  watchpoint
            case '4': {         // insert access watchpoint
                enum watchfun wf;

                if (packet[1] == '2') {
                    wf = WATCHWRITE;
                } else if (packet[1] == '3') {
                    wf = WATCHREAD;
                } else {
                    wf = WATCHACCESS;
                }

                if (add_data_watchpoint(sl, wf, addr, len) < 0) {
                    reply = strdup("E00");
                } else {
                    reply = strdup("OK");
                    break;
                }
            }
            break;

            default:
                reply = strdup("");
            }
            break;
        }
        case 'z': {
            char *endptr;
            stm32_addr_t addr = (stm32_addr_t)strtoul(&packet[3], &endptr, 16);
            // stm32_addr_t len  = strtoul(&endptr[1], NULL, 16);

            switch (packet[1]) {
            case '1':          // remove breakpoint
                update_code_breakpoint(sl, addr, 0);
                reply = strdup("OK");
                break;

            case '2':          // remove write watchpoint
            case '3':          // remove read watchpoint
            case '4':          // remove access watchpoint

                if (delete_data_watchpoint(sl, addr) < 0) {
                    reply = strdup("E00");
                    break;
                } else {
                    reply = strdup("OK");
                    break;
                }

            default:
                reply = strdup("");
            }
            break;
        }

        case '!': {
            // enter extended mode which allows restarting. We do support that always.
            // also, set to persistent mode to allow GDB disconnect.
            st->persistent = 1;

            reply = strdup("OK");
            break;
        }

        case 'R': {
            // reset the core.
            ret = stlink_reset(sl);

            if (ret) { DLOG("R packet : stlink_reset failed\n"); }

            init_code_breakpoints(sl);
            init_data_watchpoints(sl);

            attached = 1;

            reply = strdup("OK");
            break;
        }
        case 'k':
            // kill request - reset the connection itself
            ret = stlink_run(sl);

            if (ret) { DLOG("Kill: stlink_run failed\n"); }

            ret = stlink_exit_debug_mode(sl);

            if (ret) { DLOG("Kill: stlink_exit_debug_mode failed\n"); }

            stlink_close(sl);

            sl = do_connect(st);

            if (sl == NULL || sl->chip_id == STLINK_CHIPID_UNKNOWN) { cleanup(0); }

            connected_stlink = sl;

            if (st->reset) { stlink_reset(sl); }

            ret = stlink_force_debug(sl);

            if (ret) { DLOG("Kill: stlink_force_debug failed\n"); }

            init_cache(sl);
            init_code_breakpoints(sl);
            init_data_watchpoints(sl);

            reply = NULL; // no response
            break;

        default:
            reply = strdup("");
        }

        if (reply) {
            DLOG("send: %s\n", reply);

            int result = gdb_send_packet(client, reply);

            if (result != 0) {
                ELOG("cannot send: %d\n", result);
                free(reply);
                free(packet);
                close_socket(client);
                return(1);
            }

            free(reply);
        }

        if (critical_error) {
            close_socket(client);
            return(1);
        }

        free(packet);
    }

    close_socket(client);
    return(0);
}
