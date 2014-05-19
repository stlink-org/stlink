/* -*- tab-width:8 -*- */
#define DEBUG 0
/*
 Copyright (C)  2011 Peter Zotov <whitequark@whitequark.org>
 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file.
*/

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef __MINGW32__
#include "mingw.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <stlink-common.h>

#include "gdb-remote.h"

#define DEFAULT_LOGGING_LEVEL 50
#define DEFAULT_GDB_LISTEN_PORT 4242

#define STRINGIFY_inner(name) #name
#define STRINGIFY(name) STRINGIFY_inner(name)

#define FLASH_BASE 0x08000000

//Allways update the FLASH_PAGE before each use, by calling stlink_calculate_pagesize
#define FLASH_PAGE (sl->flash_pgsz)

stlink_t *connected_stlink = NULL;

static const char hex[] = "0123456789abcdef";

static const char* current_memory_map = NULL;

typedef struct _st_state_t {
    // things from command line, bleh
    int stlink_version;
    int logging_level;
    int listen_port;
    int persistent;
    int reset;
} st_state_t;


int serve(stlink_t *sl, st_state_t *st);
char* make_memory_map(stlink_t *sl);

static void cleanup(int signal __attribute__((unused))) {
    if (connected_stlink) {
        /* Switch back to mass storage mode before closing. */
        stlink_run(connected_stlink);
        stlink_exit_debug_mode(connected_stlink);
        stlink_close(connected_stlink);
    }

    exit(1);
}



int parse_options(int argc, char** argv, st_state_t *st) {
    static struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"verbose", optional_argument, NULL, 'v'},
        {"stlink_version", required_argument, NULL, 's'},
        {"stlinkv1", no_argument, NULL, '1'},
		{"listen_port", required_argument, NULL, 'p'},
		{"multi", optional_argument, NULL, 'm'},
		{"no-reset", optional_argument, NULL, 'n'},
        {0, 0, 0, 0},
    };
	const char * help_str = "%s - usage:\n\n"
	"  -h, --help\t\tPrint this help\n"
	"  -vXX, --verbose=XX\tSpecify a specific verbosity level (0..99)\n"
	"  -v, --verbose\t\tSpecify generally verbose logging\n"
	"  -s X, --stlink_version=X\n"
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
	"\n"
	"The STLINKv2 device to use can be specified in the environment\n"
	"variable STLINK_DEVICE on the format <USB_BUS>:<USB_ADDR>.\n"
	"\n"
	;


    int option_index = 0;
    int c;
    int q;
    while ((c = getopt_long(argc, argv, "hv::s:1p:mn", long_options, &option_index)) != -1) {
        switch (c) {
        case 0:
            printf("XXXXX Shouldn't really normally come here, only if there's no corresponding option\n");
            printf("option %s", long_options[option_index].name);
            if (optarg) {
                printf(" with arg %s", optarg);
            }
            printf("\n");
            break;
        case 'h':
            printf(help_str, argv[0]);
            exit(EXIT_SUCCESS);
            break;
        case 'v':
            if (optarg) {
                st->logging_level = atoi(optarg);
            } else {
                st->logging_level = DEFAULT_LOGGING_LEVEL;
            }
            break;
		case '1':
			st->stlink_version = 1;
			break;
		case 's':
			sscanf(optarg, "%i", &q);
			if (q < 0 || q > 2) {
				fprintf(stderr, "stlink version %d unknown!\n", q);
				exit(EXIT_FAILURE);
			}
			st->stlink_version = q;
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
        }
    }

    if (optind < argc) {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
    }
    return 0;
}


int main(int argc, char** argv) {
	int32_t voltage;

	stlink_t *sl = NULL;

	st_state_t state;
	memset(&state, 0, sizeof(state));
	// set defaults...
	state.stlink_version = 2;
	state.logging_level = DEFAULT_LOGGING_LEVEL;
	state.listen_port = DEFAULT_GDB_LISTEN_PORT;
	state.reset = 1;    /* By default, reset board */
	parse_options(argc, argv, &state);
	switch (state.stlink_version) {
	case 2:
		sl = stlink_open_usb(state.logging_level, 0);
		if(sl == NULL) return 1;
		break;
	case 1:
		sl = stlink_v1_open(state.logging_level, 0);
		if(sl == NULL) return 1;
		break;
    }

    connected_stlink = sl;
    signal(SIGINT, &cleanup);
    signal(SIGTERM, &cleanup);

    if (state.reset) {
		stlink_reset(sl);
    }

	printf("Chip ID is %08x, Core ID is  %08x.\n", sl->chip_id, sl->core_id);

	voltage = stlink_target_voltage(sl);
	if (voltage != -1) {
		printf("Target voltage is %d mV.\n", voltage);
	}

	sl->verbose=0;

	current_memory_map = make_memory_map(sl);

#ifdef __MINGW32__
	WSADATA	wsadata;
	if (WSAStartup(MAKEWORD(2,2),&wsadata) !=0 ) {
		goto winsock_error;
	}
#endif

	do {
		serve(sl, &state);

		/* Continue */
		stlink_run(sl);
	} while (state.persistent);

#ifdef __MINGW32__
winsock_error:
	WSACleanup();
#endif

	/* Switch back to mass storage mode before closing. */
	stlink_exit_debug_mode(sl);
	stlink_close(sl);

	return 0;
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
  "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x100000\"/>"       // code = sram, bootrom or flash; flash is bigger
  "  <memory type=\"ram\" start=\"0x10000000\" length=\"0x10000\"/>"        // ccm ram
  "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x20000\"/>"        // sram
  "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x10000\">"		//Sectors 0..3
  "    <property name=\"blocksize\">0x4000</property>"						//16kB
  "  </memory>"
  "  <memory type=\"flash\" start=\"0x08010000\" length=\"0x10000\">"		//Sector 4
  "    <property name=\"blocksize\">0x10000</property>"						//64kB
  "  </memory>"
  "  <memory type=\"flash\" start=\"0x08020000\" length=\"0x70000\">"		//Sectors 5..11
  "    <property name=\"blocksize\">0x20000</property>"						//128kB
  "  </memory>"
  "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>" 	// peripheral regs
  "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>" 	// cortex regs
  "  <memory type=\"rom\" start=\"0x1fff0000\" length=\"0x7800\"/>"         // bootrom
  "  <memory type=\"rom\" start=\"0x1fffc000\" length=\"0x10\"/>"        	// option byte area
  "</memory-map>";

static const char* const memory_map_template =
  "<?xml version=\"1.0\"?>"
  "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
  "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
  "<memory-map>"
  "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x%zx\"/>"       // code = sram, bootrom or flash; flash is bigger
  "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x%zx\"/>"       // sram 8k
  "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x%zx\">"
  "    <property name=\"blocksize\">0x%zx</property>"
  "  </memory>"
  "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>" // peripheral regs
  "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>" // cortex regs
  "  <memory type=\"rom\" start=\"0x%08x\" length=\"0x%zx\"/>"           // bootrom
  "  <memory type=\"rom\" start=\"0x1ffff800\" length=\"0x10\"/>"        // option byte area
  "</memory-map>";

char* make_memory_map(stlink_t *sl) {
	/* This will be freed in serve() */
	char* map = malloc(4096);
	map[0] = '\0';

	if(sl->chip_id==STM32_CHIPID_F4) {
    	strcpy(map, memory_map_template_F4);
    } else {
        snprintf(map, 4096, memory_map_template,
			sl->flash_size,
			sl->sram_size,
			sl->flash_size, sl->flash_pgsz,
			sl->sys_base, sl->sys_size);
    }
	return map;
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

struct code_hw_watchpoint data_watches[DATA_WATCH_NUM];

static void init_data_watchpoints(stlink_t *sl) {
	#if DEBUG
	printf("init watchpoints\n");
	#endif

	// set trcena in debug command to turn on dwt unit
	stlink_write_debug32(sl, 0xE000EDFC,
			     stlink_read_debug32(sl, 0xE000EDFC) | (1<<24));

	// make sure all watchpoints are cleared
	for(int i = 0; i < DATA_WATCH_NUM; i++) {
		data_watches[i].fun = WATCHDISABLED;
		stlink_write_debug32(sl, 0xe0001028 + i * 16, 0);
	}
}

static int add_data_watchpoint(stlink_t *sl, enum watchfun wf, stm32_addr_t addr, unsigned int len)
{
	int i = 0;
	uint32_t mask;

	// computer mask
	// find a free watchpoint
	// configure

	mask = -1;
	i = len;
	while(i) {
		i >>= 1;
		mask++;
	}

	if((mask != (uint32_t)-1) && (mask < 16)) {
		for(i = 0; i < DATA_WATCH_NUM; i++) {
			// is this an empty slot ?
			if(data_watches[i].fun == WATCHDISABLED) {
				#if DEBUG
				printf("insert watchpoint %d addr %x wf %u mask %u len %d\n", i, addr, wf, mask, len);
				#endif

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
				stlink_read_debug32(sl,  0xE0001028 + i * 16);
				return 0;
			}
		}
	}

	#if DEBUG
	printf("failure: add watchpoints addr %x wf %u len %u\n", addr, wf, len);
	#endif
	return -1;
}

static int delete_data_watchpoint(stlink_t *sl, stm32_addr_t addr)
{
	int i;

	for(i = 0 ; i < DATA_WATCH_NUM; i++) {
		if((data_watches[i].addr == addr) && (data_watches[i].fun != WATCHDISABLED)) {
			#if DEBUG
			printf("delete watchpoint %d addr %x\n", i, addr);
			#endif

			data_watches[i].fun = WATCHDISABLED;
			stlink_write_debug32(sl, 0xe0001028 + i * 16, 0);

			return 0;
		}
	}

	#if DEBUG
	printf("failure: delete watchpoint addr %x\n", addr);
	#endif

	return -1;
}

#define CODE_BREAK_NUM	6
#define CODE_LIT_NUM 	2
#define CODE_BREAK_LOW	0x01
#define CODE_BREAK_HIGH	0x02

struct code_hw_breakpoint {
	stm32_addr_t addr;
	int          type;
};

struct code_hw_breakpoint code_breaks[CODE_BREAK_NUM];

static void init_code_breakpoints(stlink_t *sl) {
	memset(sl->q_buf, 0, 4);
	stlink_write_debug32(sl, CM3_REG_FP_CTRL, 0x03 /*KEY | ENABLE4*/);
        unsigned int val = stlink_read_debug32(sl, CM3_REG_FP_CTRL);
        if (((val & 3) != 1) ||
            ((((val >> 8) & 0x70) | ((val >> 4) & 0xf)) != CODE_BREAK_NUM) ||
            (((val >> 8) & 0xf) != CODE_LIT_NUM)){
          fprintf(stderr, "[FP_CTRL] = 0x%08x expecting 0x%08x\n", val,
            ((CODE_BREAK_NUM & 0x70) << 8) | (CODE_LIT_NUM << 8) |  ((CODE_BREAK_NUM & 0xf) << 4) | 1);
        }
        

	for(int i = 0; i < CODE_BREAK_NUM; i++) {
		code_breaks[i].type = 0;
		stlink_write_debug32(sl, CM3_REG_FP_COMP0 + i * 4, 0);
	}
}

static int update_code_breakpoint(stlink_t *sl, stm32_addr_t addr, int set) {
	stm32_addr_t fpb_addr = addr & ~0x3;
	int type = addr & 0x2 ? CODE_BREAK_HIGH : CODE_BREAK_LOW;

	if(addr & 1) {
		fprintf(stderr, "update_code_breakpoint: unaligned address %08x\n", addr);
		return -1;
	}

	int id = -1;
	for(int i = 0; i < CODE_BREAK_NUM; i++) {
		if(fpb_addr == code_breaks[i].addr ||
			(set && code_breaks[i].type == 0)) {
			id = i;
			break;
		}
	}

	if(id == -1) {
		if(set) return -1; // Free slot not found
		else	return 0;  // Breakpoint is already removed
	}

	struct code_hw_breakpoint* brk = &code_breaks[id];

	brk->addr = fpb_addr;

	if(set) brk->type |= type;
	else	brk->type &= ~type;

	if(brk->type == 0) {
		#if DEBUG
		printf("clearing hw break %d\n", id);
		#endif

		stlink_write_debug32(sl, 0xe0002008 + id * 4, 0);
	} else {
	        uint32_t mask = (brk->addr) | 1 | (brk->type << 30);

		#if DEBUG
		printf("setting hw break %d at %08x (%d)\n",
			id, brk->addr, brk->type);
		printf("reg %08x \n",
			mask);
		#endif

		stlink_write_debug32(sl, 0xe0002008 + id * 4, mask);
	}

	return 0;
}


struct flash_block {
	stm32_addr_t addr;
	unsigned     length;
	uint8_t*     data;

	struct flash_block* next;
};

static struct flash_block* flash_root;

static int flash_add_block(stm32_addr_t addr, unsigned length, stlink_t *sl) {

	if(addr < FLASH_BASE || addr + length > FLASH_BASE + sl->flash_size) {
		fprintf(stderr, "flash_add_block: incorrect bounds\n");
		return -1;
	}

	stlink_calculate_pagesize(sl, addr);
	if(addr % FLASH_PAGE != 0 || length % FLASH_PAGE != 0) {
		fprintf(stderr, "flash_add_block: unaligned block\n");
		return -1;
	}

	struct flash_block* new = malloc(sizeof(struct flash_block));
	new->next = flash_root;

	new->addr   = addr;
	new->length = length;
	new->data   = calloc(length, 1);

	flash_root = new;

	return 0;
}

static int flash_populate(stm32_addr_t addr, uint8_t* data, unsigned length) {
	unsigned int fit_blocks = 0, fit_length = 0;

	for(struct flash_block* fb = flash_root; fb; fb = fb->next) {
		/* Block: ------X------Y--------
		 * Data:            a-----b
		 *                a--b
		 *            a-----------b
		 * Block intersects with data, if:
		 *  a < Y && b > x
		 */

		unsigned X = fb->addr, Y = fb->addr + fb->length;
		unsigned a = addr, b = addr + length;
		if(a < Y && b > X) {
			// from start of the block
			unsigned start = (a > X ? a : X) - X;
			unsigned end   = (b > Y ? Y : b) - X;

			memcpy(fb->data + start, data, end - start);

			fit_blocks++;
			fit_length += end - start;
		}
	}

	if(fit_blocks == 0) {
		fprintf(stderr, "Unfit data block %08x -> %04x\n", addr, length);
		return -1;
	}

	if(fit_length != length) {
		fprintf(stderr, "warning: data block %08x -> %04x truncated to %04x\n",
			addr, length, fit_length);
		fprintf(stderr, "(this is not an error, just a GDB glitch)\n");
	}

	return 0;
}

static int flash_go(stlink_t *sl) {
	int error = -1;

	// Some kinds of clock settings do not allow writing to flash.
	stlink_reset(sl);

	for(struct flash_block* fb = flash_root; fb; fb = fb->next) {
		#if DEBUG
		printf("flash_do: block %08x -> %04x\n", fb->addr, fb->length);
		#endif

		unsigned length = fb->length;
		for(stm32_addr_t page = fb->addr; page < fb->addr + fb->length; page += FLASH_PAGE) {

			//Update FLASH_PAGE
			stlink_calculate_pagesize(sl, page);

			#if DEBUG
			printf("flash_do: page %08x\n", page);
			#endif

			if(stlink_write_flash(sl, page, fb->data + (page - fb->addr),
					length > FLASH_PAGE ? FLASH_PAGE : length) < 0)
				goto error;
			}
	}

	stlink_reset(sl);

	error = 0;

error:
	for(struct flash_block* fb = flash_root, *next; fb; fb = next) {
		next = fb->next;
		free(fb->data);
		free(fb);
	}

	flash_root = NULL;

	return error;
}

int serve(stlink_t *sl, st_state_t *st) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
		perror("socket");
		return 1;
	}

	unsigned int val = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(val));

	struct sockaddr_in serv_addr;
	memset(&serv_addr,0,sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(st->listen_port);

	if(bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("bind");
		return 1;
	}

	if(listen(sock, 5) < 0) {
		perror("listen");
		return 1;
	}

	printf("Listening at *:%d...\n", st->listen_port);

	int client = accept(sock, NULL, NULL);
	//signal (SIGINT, SIG_DFL);
	if(client < 0) {
		perror("accept");
		return 1;
	}

	close(sock);

	stlink_force_debug(sl);
	if (st->reset) {
		stlink_reset(sl);
    }
	init_code_breakpoints(sl);
	init_data_watchpoints(sl);

	printf("GDB connected.\n");

	/*
	 * To allow resetting the chip from GDB it is required to
	 * emulate attaching and detaching to target.
	 */
	unsigned int attached = 1;

	while(1) {
		char* packet;

		int status = gdb_recv_packet(client, &packet);
		if(status < 0) {
			fprintf(stderr, "cannot recv: %d\n", status);
			return 1;
		}

		#if DEBUG
		printf("recv: %s\n", packet);
		#endif

		char* reply = NULL;
                reg regp;

		switch(packet[0]) {
		case 'q': {
			if(packet[1] == 'P' || packet[1] == 'C' || packet[1] == 'L') {
				reply = strdup("");
				break;
			}

			char *separator = strstr(packet, ":"), *params = "";
			if(separator == NULL) {
				separator = packet + strlen(packet);
			} else {
				params = separator + 1;
			}

			unsigned queryNameLength = (separator - &packet[1]);
			char* queryName = calloc(queryNameLength + 1, 1);
			strncpy(queryName, &packet[1], queryNameLength);

			#if DEBUG
			printf("query: %s;%s\n", queryName, params);
			#endif

			if(!strcmp(queryName, "Supported")) {
                if(sl->chip_id==STM32_CHIPID_F4) {
                    reply = strdup("PacketSize=3fff;qXfer:memory-map:read+;qXfer:features:read+");
                }
                else {
                    reply = strdup("PacketSize=3fff;qXfer:memory-map:read+");
                }
			} else if(!strcmp(queryName, "Xfer")) {
				char *type, *op, *__s_addr, *s_length;
				char *tok = params;
				char *annex __attribute__((unused));

				type     = strsep(&tok, ":");
				op       = strsep(&tok, ":");
				annex    = strsep(&tok, ":");
				__s_addr   = strsep(&tok, ",");
				s_length = tok;

				unsigned addr = strtoul(__s_addr, NULL, 16),
				       length = strtoul(s_length, NULL, 16);

				#if DEBUG
				printf("Xfer: type:%s;op:%s;annex:%s;addr:%d;length:%d\n",
					type, op, annex, addr, length);
				#endif

				const char* data = NULL;

				if(!strcmp(type, "memory-map") && !strcmp(op, "read"))
					data = current_memory_map;

				if(!strcmp(type, "features") && !strcmp(op, "read"))
					data = target_description_F4;

				if(data) {
					unsigned data_length = strlen(data);
					if(addr + length > data_length)
						length = data_length - addr;

					if(length == 0) {
						reply = strdup("l");
					} else {
						reply = calloc(length + 2, 1);
						reply[0] = 'm';
						strncpy(&reply[1], data, length);
					}
				}
			} else if(!strncmp(queryName, "Rcmd,",4)) {
				// Rcmd uses the wrong separator
				char *separator = strstr(packet, ","), *params = "";
				if(separator == NULL) {
					separator = packet + strlen(packet);
				} else {
					params = separator + 1;
				}


				if (!strncmp(params,"726573756d65",12)) {// resume
#if DEBUG
					printf("Rcmd: resume\n");
#endif
					stlink_run(sl);

					reply = strdup("OK");
                } else if (!strncmp(params,"68616c74",8)) { //halt
					reply = strdup("OK");

					stlink_force_debug(sl);

#if DEBUG
					printf("Rcmd: halt\n");
#endif
                } else if (!strncmp(params,"6a7461675f7265736574",20)) { //jtag_reset
					reply = strdup("OK");

					stlink_jtag_reset(sl, 1);
					stlink_jtag_reset(sl, 0);
					stlink_force_debug(sl);

#if DEBUG
					printf("Rcmd: jtag_reset\n");
#endif
                } else if (!strncmp(params,"7265736574",10)) { //reset
					reply = strdup("OK");

					stlink_force_debug(sl);
					stlink_reset(sl);
					init_code_breakpoints(sl);
					init_data_watchpoints(sl);

#if DEBUG
					printf("Rcmd: reset\n");
#endif
				} else {
#if DEBUG
					printf("Rcmd: %s\n", params);
#endif

				}

			}

			if(reply == NULL)
				reply = strdup("");

			free(queryName);

			break;
		}

		case 'v': {
			char *params = NULL;
			char *cmdName = strtok_r(packet, ":;", &params);

			cmdName++; // vCommand -> Command

			if(!strcmp(cmdName, "FlashErase")) {
				char *__s_addr, *s_length;
				char *tok = params;

				__s_addr   = strsep(&tok, ",");
				s_length = tok;

				unsigned addr = strtoul(__s_addr, NULL, 16),
				       length = strtoul(s_length, NULL, 16);

				#if DEBUG
				printf("FlashErase: addr:%08x,len:%04x\n",
					addr, length);
				#endif

				if(flash_add_block(addr, length, sl) < 0) {
					reply = strdup("E00");
				} else {
					reply = strdup("OK");
				}
			} else if(!strcmp(cmdName, "FlashWrite")) {
				char *__s_addr, *data;
				char *tok = params;

				__s_addr = strsep(&tok, ":");
				data   = tok;

				unsigned addr = strtoul(__s_addr, NULL, 16);
				unsigned data_length = status - (data - packet);

				// Length of decoded data cannot be more than
				// encoded, as escapes are removed.
				// Additional byte is reserved for alignment fix.
				uint8_t *decoded = calloc(data_length + 1, 1);
				unsigned dec_index = 0;
				for(unsigned int i = 0; i < data_length; i++) {
					if(data[i] == 0x7d) {
						i++;
						decoded[dec_index++] = data[i] ^ 0x20;
					} else {
						decoded[dec_index++] = data[i];
					}
				}

				// Fix alignment
				if(dec_index % 2 != 0)
					dec_index++;

				#if DEBUG
				printf("binary packet %d -> %d\n", data_length, dec_index);
				#endif

				if(flash_populate(addr, decoded, dec_index) < 0) {
					reply = strdup("E00");
				} else {
					reply = strdup("OK");
				}
			} else if(!strcmp(cmdName, "FlashDone")) {
				if(flash_go(sl) < 0) {
					reply = strdup("E00");
				} else {
					reply = strdup("OK");
				}
			} else if(!strcmp(cmdName, "Kill")) {
				attached = 0;

				reply = strdup("OK");
			}

			if(reply == NULL)
				reply = strdup("");

			break;
		}

		case 'c':
			stlink_run(sl);

			while(1) {
				int status = gdb_check_for_interrupt(client);
				if(status < 0) {
					fprintf(stderr, "cannot check for int: %d\n", status);
					return 1;
				}

				if(status == 1) {
					stlink_force_debug(sl);
					break;
				}

				stlink_status(sl);
				if(sl->core_stat == STLINK_CORE_HALTED) {
					break;
				}

				usleep(100000);
			}

			reply = strdup("S05"); // TRAP
			break;

		case 's':
			stlink_step(sl);

			reply = strdup("S05"); // TRAP
			break;

		case '?':
			if(attached) {
				reply = strdup("S05"); // TRAP
			} else {
				/* Stub shall reply OK if not attached. */
				reply = strdup("OK");
			}
			break;

		case 'g':
			stlink_read_all_regs(sl, &regp);

			reply = calloc(8 * 16 + 1, 1);
			for(int i = 0; i < 16; i++)
				sprintf(&reply[i * 8], "%08x", htonl(regp.r[i]));

			break;

		case 'p': {
			unsigned id = strtoul(&packet[1], NULL, 16);
                        unsigned myreg = 0xDEADDEAD;

			if(id < 16) {
				stlink_read_reg(sl, id, &regp);
				myreg = htonl(regp.r[id]);
			} else if(id == 0x19) {
				stlink_read_reg(sl, 16, &regp);
				myreg = htonl(regp.xpsr);
			} else if(id == 0x1A) {
				stlink_read_reg(sl, 17, &regp);
				myreg = htonl(regp.main_sp);
			} else if(id == 0x1B) {
				stlink_read_reg(sl, 18, &regp);
				myreg = htonl(regp.process_sp);
			} else if(id == 0x1C) {
				stlink_read_unsupported_reg(sl, id, &regp);
				myreg = htonl(regp.control);
			} else if(id == 0x1D) {
				stlink_read_unsupported_reg(sl, id, &regp);
				myreg = htonl(regp.faultmask);
			} else if(id == 0x1E) {
				stlink_read_unsupported_reg(sl, id, &regp);
				myreg = htonl(regp.basepri);
			} else if(id == 0x1F) {
				stlink_read_unsupported_reg(sl, id, &regp);
				myreg = htonl(regp.primask);
            } else if(id >= 0x20 && id < 0x40) {
                stlink_read_unsupported_reg(sl, id, &regp);
                myreg = htonl(regp.s[id-0x20]);
			} else if(id == 0x40) {
                stlink_read_unsupported_reg(sl, id, &regp);
                myreg = htonl(regp.fpscr);
			} else {
				reply = strdup("E00");
			}

			reply = calloc(8 + 1, 1);
			sprintf(reply, "%08x", myreg);

			break;
		}

		case 'P': {
			char* s_reg = &packet[1];
			char* s_value = strstr(&packet[1], "=") + 1;

			unsigned reg   = strtoul(s_reg,   NULL, 16);
			unsigned value = strtoul(s_value, NULL, 16);

			if(reg < 16) {
				stlink_write_reg(sl, ntohl(value), reg);
			} else if(reg == 0x19) {
				stlink_write_reg(sl, ntohl(value), 16);
			} else if(reg == 0x1A) {
				stlink_write_reg(sl, ntohl(value), 17);
			} else if(reg == 0x1B) {
				stlink_write_reg(sl, ntohl(value), 18);
			} else if(reg == 0x1C) {
				stlink_write_unsupported_reg(sl, ntohl(value), reg, &regp);
			} else if(reg == 0x1D) {
				stlink_write_unsupported_reg(sl, ntohl(value), reg, &regp);
			} else if(reg == 0x1E) {
				stlink_write_unsupported_reg(sl, ntohl(value), reg, &regp);
			} else if(reg == 0x1F) {
				stlink_write_unsupported_reg(sl, ntohl(value), reg, &regp);
            } else if(reg >= 0x20 && reg < 0x40) {
                stlink_write_unsupported_reg(sl, ntohl(value), reg, &regp);
			} else if(reg == 0x40) {
                stlink_write_unsupported_reg(sl, ntohl(value), reg, &regp);
			} else {
				reply = strdup("E00");
			}

			if(!reply) {
				reply = strdup("OK");
			}

			break;
		}

		case 'G':
			for(int i = 0; i < 16; i++) {
				char str[9] = {0};
				strncpy(str, &packet[1 + i * 8], 8);
				uint32_t reg = strtoul(str, NULL, 16);
				stlink_write_reg(sl, ntohl(reg), i);
			}

			reply = strdup("OK");
			break;

		case 'm': {
			char* s_start = &packet[1];
			char* s_count = strstr(&packet[1], ",") + 1;

			stm32_addr_t start = strtoul(s_start, NULL, 16);
			unsigned     count = strtoul(s_count, NULL, 16);

			unsigned adj_start = start % 4;
			unsigned count_rnd = (count + adj_start + 4 - 1) / 4 * 4;

			stlink_read_mem32(sl, start - adj_start, count_rnd);

			reply = calloc(count * 2 + 1, 1);
			for(unsigned int i = 0; i < count; i++) {
				reply[i * 2 + 0] = hex[sl->q_buf[i + adj_start] >> 4];
				reply[i * 2 + 1] = hex[sl->q_buf[i + adj_start] & 0xf];
			}

			break;
		}

		case 'M': {
			char* s_start = &packet[1];
			char* s_count = strstr(&packet[1], ",") + 1;
			char* hexdata = strstr(packet, ":") + 1;

			stm32_addr_t start = strtoul(s_start, NULL, 16);
			unsigned     count = strtoul(s_count, NULL, 16);

			if(start % 4) {
			  unsigned align_count = 4 - start % 4;
			  if (align_count > count) align_count = count;
			  for(unsigned int i = 0; i < align_count; i ++) {
				char hex[3] = { hexdata[i*2], hexdata[i*2+1], 0 };
				uint8_t byte = strtoul(hex, NULL, 16);
				sl->q_buf[i] = byte;
			  }
			  stlink_write_mem8(sl, start, align_count);
			  start += align_count;
			  count -= align_count;
			  hexdata += 2*align_count;
			}

			if(count - count % 4) {
			  unsigned aligned_count = count - count % 4;

			  for(unsigned int i = 0; i < aligned_count; i ++) {
			    char hex[3] = { hexdata[i*2], hexdata[i*2+1], 0 };
			    uint8_t byte = strtoul(hex, NULL, 16);
			    sl->q_buf[i] = byte;
			  }
			  stlink_write_mem32(sl, start, aligned_count);
			  count -= aligned_count;
			  start += aligned_count;
			  hexdata += 2*aligned_count;
			}

			if(count) {
			  for(unsigned int i = 0; i < count; i ++) {
			    char hex[3] = { hexdata[i*2], hexdata[i*2+1], 0 };
			    uint8_t byte = strtoul(hex, NULL, 16);
			    sl->q_buf[i] = byte;
			  }
			  stlink_write_mem8(sl, start, count);
			}
			reply = strdup("OK");
			break;
		}

		case 'Z': {
			char *endptr;
			stm32_addr_t addr = strtoul(&packet[3], &endptr, 16);
			stm32_addr_t len  = strtoul(&endptr[1], NULL, 16);

			switch (packet[1]) {
				case '1':
				if(update_code_breakpoint(sl, addr, 1) < 0) {
					reply = strdup("E00");
				} else {
					reply = strdup("OK");
				}
				break;

				case '2':   // insert write watchpoint
				case '3':   // insert read  watchpoint
				case '4':   // insert access watchpoint
		    		{
					enum watchfun wf;
		 			if(packet[1] == '2') {
						wf = WATCHWRITE;
					} else if(packet[1] == '3') {
						wf = WATCHREAD;
					} else {
						wf = WATCHACCESS;
					}

                    if(add_data_watchpoint(sl, wf, addr, len) < 0) {
                        reply = strdup("E00");
                    } else {
                        reply = strdup("OK");
                        break;
                    }
				}

				default:
				reply = strdup("");
			}
			break;
		}
		case 'z': {
			char *endptr;
			stm32_addr_t addr = strtoul(&packet[3], &endptr, 16);
			//stm32_addr_t len  = strtoul(&endptr[1], NULL, 16);

			switch (packet[1]) {
				case '1': // remove breakpoint
				update_code_breakpoint(sl, addr, 0);
				reply = strdup("OK");
				break;

				case '2' : // remove write watchpoint
				case '3' : // remove read watchpoint
				case '4' : // remove access watchpoint
				if(delete_data_watchpoint(sl, addr) < 0) {
					reply = strdup("E00");
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
			/*
			 * Enter extended mode which allows restarting.
			 * We do support that always.
			 */

			/*
			 * Also, set to persistent mode
			 * to allow GDB disconnect.
			 */
			st->persistent = 1;

			reply = strdup("OK");

			break;
		}

		case 'R': {
			/* Reset the core. */

			stlink_reset(sl);
			init_code_breakpoints(sl);
			init_data_watchpoints(sl);

			attached = 1;

			reply = strdup("OK");

			break;
		}

		default:
			reply = strdup("");
		}

		if(reply) {
			#if DEBUG
			printf("send: %s\n", reply);
			#endif

			int result = gdb_send_packet(client, reply);
			if(result != 0) {
				fprintf(stderr, "cannot send: %d\n", result);
				free(reply);
				free(packet);
				return 1;
			}

			free(reply);
		}

		free(packet);
	}

	return 0;
}
