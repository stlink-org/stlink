/* -*- tab-width:8 -*- */

/*
 Copyright (C)  2011 Peter Zotov <whitequark@whitequark.org>
 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "gdb-remote.h"
#include "stlink-hw.h"

static const char hex[] = "0123456789abcdef";

// configured for STM32F100RB
static const char* const c_memory_map =
  "<?xml version=\"1.0\"?>"
  "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
  "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
  "<memory-map>"
  "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x20000\"/>"    // code = sram or flash
  "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x2000\"/>"     // sram 8k
  "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x20000\">"   // flash 128k
  "    <property name=\"blocksize\">0x400</property>"                   // 1k pages
  "  </memory>"
  "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>" // peripheral regs
  "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>" // cortex regs
  "</memory-map>";

int serve(struct stlink* sl, int port);

int main(int argc, char** argv) {
	if(argc != 3) {
		fprintf(stderr, "Usage: %s <port> /dev/sgX\n", argv[0]);
		return 1;
	}

	struct stlink *sl = stlink_quirk_open(argv[2], 0);
	if (sl == NULL)
		return 1;

	if(stlink_current_mode(sl) != STLINK_DEV_DEBUG_MODE)
		stlink_enter_swd_mode(sl);

	stlink_core_id(sl);
	printf("Debugging ARM core %08x.\n", sl->core_id);

	int port = atoi(argv[1]);

	while(serve(sl, port) == 0);

	stlink_close(sl);

	return 0;
}

#define CODE_BREAK_NUM	6

#define CODE_BREAK_LOW	0x01
#define CODE_BREAK_HIGH	0x02

struct code_hw_breakpoint {
	stm32_addr_t addr;
	int          type;
};

struct code_hw_breakpoint code_breaks[CODE_BREAK_NUM];

static void init_code_breakpoints(struct stlink* sl) {
	memset(sl->q_buf, 0, 4);
	sl->q_buf[0] = 0x03; // KEY | ENABLE
	stlink_write_mem32(sl, 0xe0002000, 4);

	memset(sl->q_buf, 0, 4);
	for(int i = 0; i < CODE_BREAK_NUM; i++) {
		code_breaks[i].type = 0;
		stlink_write_mem32(sl, 0xe0002008 + i * 4, 4);
	}
}

static int update_code_breakpoint(struct stlink* sl, stm32_addr_t addr, int set) {
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

	memset(sl->q_buf, 0, 4);

	if(brk->type == 0) {
		#ifdef DEBUG
		printf("clearing hw break %d\n", id);
		#endif

		stlink_write_mem32(sl, 0xe0002008 + id * 4, 4);
	} else {
		sl->q_buf[0] = ( brk->addr        & 0xff) | 1;
		sl->q_buf[1] = ((brk->addr >> 8)  & 0xff);
		sl->q_buf[2] = ((brk->addr >> 16) & 0xff);
		sl->q_buf[3] = ((brk->addr >> 24) & 0xff) | (brk->type << 6);

		#ifdef DEBUG
		printf("setting hw break %d at %08x (%d)\n",
			id, brk->addr, brk->type);
		printf("reg %02x %02x %02x %02x\n",
			sl->q_buf[3], sl->q_buf[2], sl->q_buf[1], sl->q_buf[0]);
		#endif

		stlink_write_mem32(sl, 0xe0002008 + id * 4, 4);
	}

	return 0;
}

#define FLASH_BASE 0x08000000
#define FLASH_PAGE 0x400
#define FLASH_PAGE_MASK (~((1 << 10) - 1))
#define FLASH_SIZE (FLASH_PAGE * 128)

struct flash_block {
	stm32_addr_t addr;
	unsigned     length;
	uint8_t*     data;

	struct flash_block* next;
};

static struct flash_block* flash_root;

static int flash_add_block(stm32_addr_t addr, unsigned length) {
	if(addr < FLASH_BASE || addr + length > FLASH_BASE + FLASH_SIZE) {
		fprintf(stderr, "flash_add_block: incorrect bounds\n");
		return -1;
	}

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
	int fit_blocks = 0, fit_length = 0;

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

static int flash_go(struct stlink* sl) {
	int error = -1;

	for(struct flash_block* fb = flash_root; fb; fb = fb->next) {
		#ifdef DEBUG
		printf("flash_do: block %08x -> %04x\n", fb->addr, fb->length);
		#endif

		stlink_erase_flash_page(sl, fb->addr);

		if(!stlink_write_flash(sl, fb->addr, fb->data, fb->length) < 0) {
			fprintf(stderr, "Flash writing failed.\n");
			goto error;
		}
	}

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

int serve(struct stlink* sl, int port) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
		perror("socket");
		return 1;
	}

	unsigned int val = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	struct sockaddr_in serv_addr = {0};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(port);

	if(bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("bind");
		return 1;
	}

	if(listen(sock, 5) < 0) {
		perror("listen");
		return 1;
	}

	stlink_force_debug(sl);
	stlink_reset(sl);
	init_code_breakpoints(sl);

	printf("Listening at *:%d...\n", port);

	int client = accept(sock, NULL, NULL);
	if(client < 0) {
		perror("accept");
		return 1;
	}

	close(sock);

	printf("GDB connected.\n");

	while(1) {
		char* packet;

		int status = gdb_recv_packet(client, &packet);
		if(status < 0) {
			fprintf(stderr, "cannot recv: %d\n", status);
			return 1;
		}

		#ifdef DEBUG
		printf("recv: %s\n", packet);
		#endif

		char* reply = NULL;

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

			#ifdef DEBUG
			printf("query: %s;%s\n", queryName, params);
			#endif

			if(!strcmp(queryName, "Supported")) {
				reply = strdup("PacketSize=3fff;qXfer:memory-map:read+");
			} else if(!strcmp(queryName, "Xfer")) {
				char *type, *op, *annex, *s_addr, *s_length;
				char *tok = params;

				type     = strsep(&tok, ":");
				op       = strsep(&tok, ":");
				annex    = strsep(&tok, ":");
				s_addr   = strsep(&tok, ",");
				s_length = tok;

				unsigned addr = strtoul(s_addr, NULL, 16),
				       length = strtoul(s_length, NULL, 16);

				#ifdef DEBUG
				printf("Xfer: type:%s;op:%s;annex:%s;addr:%d;length:%d\n",
					type, op, annex, addr, length);
				#endif

				const char* data = NULL;

				if(!strcmp(type, "memory-map") && !strcmp(op, "read"))
					data = c_memory_map;

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
			}

			if(reply == NULL)
				reply = strdup("");

			free(queryName);

			break;
		}

		case 'v': {
			char *separator = strstr(packet, ":"), *params = "";
			if(separator == NULL) {
				separator = packet + strlen(packet);
			} else {
				params = separator + 1;
			}

			unsigned cmdNameLength = (separator - &packet[1]);
			char* cmdName = calloc(cmdNameLength + 1, 1);
			strncpy(cmdName, &packet[1], cmdNameLength);

			if(!strcmp(cmdName, "FlashErase")) {
				char *s_addr, *s_length;
				char *tok = params;

				s_addr   = strsep(&tok, ",");
				s_length = tok;

				unsigned addr = strtoul(s_addr, NULL, 16),
				       length = strtoul(s_length, NULL, 16);

				#ifdef DEBUG
				printf("FlashErase: addr:%08x,len:%04x\n",
					addr, length);
				#endif

				if(flash_add_block(addr, length) < 0) {
					reply = strdup("E00");
				} else {
					reply = strdup("OK");
				}
			} else if(!strcmp(cmdName, "FlashWrite")) {
				char *s_addr, *data;
				char *tok = params;

				s_addr = strsep(&tok, ":");
				data   = tok;

				unsigned addr = strtoul(s_addr, NULL, 16);
				unsigned data_length = status - (data - packet);

				// Length of decoded data cannot be more than
				// encoded, as escapes are removed.
				// Additional byte is reserved for alignment fix.
				uint8_t *decoded = calloc(data_length + 1, 1);
				unsigned dec_index = 0;
				for(int i = 0; i < data_length; i++) {
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

				#ifdef DEBUG
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

				stlink_reset(sl);
			}

			if(reply == NULL)
				reply = strdup("");

			free(cmdName);

			break;
		}

		case 'c':
			stlink_run(sl);

			printf("Core running, waiting for interrupt (either in chip or GDB).\n");

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

				usleep(200000);
			}

			reply = strdup("S05"); // TRAP
			break;

		case 's':
			stlink_step(sl);

			reply = strdup("S05"); // TRAP
			break;

		case '?':
			reply = strdup("S05"); // TRAP
			break;

		case 'g':
			stlink_read_all_regs(sl);

			reply = calloc(8 * 16 + 1, 1);
			for(int i = 0; i < 16; i++)
				sprintf(&reply[i * 8], "%08x", htonl(sl->reg.r[i]));

			break;

		case 'p': {
			unsigned id = strtoul(&packet[1], NULL, 16), reg = 0xDEADDEAD;

			if(id < 16) {
				stlink_read_reg(sl, id);
				reg = htonl(sl->reg.r[id]);
			} else if(id == 0x19) {
				stlink_read_reg(sl, 16);
				reg = htonl(sl->reg.xpsr);
			} else {
				reply = strdup("E00");
			}

			reply = calloc(8 + 1, 1);
			sprintf(reply, "%08x", reg);

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

			stlink_read_mem32(sl, start - adj_start, (count % 4 == 0) ?
						count : count + 4 - (count % 4));

			reply = calloc(count * 2 + 1, 1);
			for(int i = 0; i < count; i++) {
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

			for(int i = 0; i < count; i ++) {
				char hex[3] = { hexdata[i*2], hexdata[i*2+1], 0 };
				uint8_t byte = strtoul(hex, NULL, 16);
				sl->q_buf[i] = byte;
			}

			if((count % 4) == 0 && (start % 4) == 0) {
				stlink_write_mem32(sl, start, count);
			} else {
				stlink_write_mem8(sl, start, count);
			}

			reply = strdup("OK");

			break;
		}

		case 'Z': {
			if(packet[1] == '1') {
				stm32_addr_t addr = strtoul(&packet[3], NULL, 16);
				if(update_code_breakpoint(sl, addr, 1) < 0) {
					reply = strdup("E00");
				} else {
					reply = strdup("OK");
				}
			} else {
				reply = strdup("");
			}

			break;
		}

		case 'z': {
			if(packet[1] == '1') {
				stm32_addr_t addr = strtoul(&packet[3], NULL, 16);
				update_code_breakpoint(sl, addr, 0);

				reply = strdup("OK");
			} else {
				reply = strdup("");
			}

			break;
		}

		case 'k': {
			// After this function will be entered afterwards, the
			// chip will be reset anyway. So this is a no-op.

			close(client);
			return 0;
		}

		default:
			reply = strdup("");
		}

		if(reply) {
			#ifdef DEBUG
			printf("send: %s\n", reply);
			#endif

			int result = gdb_send_packet(client, reply);
			if(result != 0) {
				fprintf(stderr, "cannot send: %d\n", result);
				return 1;
			}

			free(reply);
		}

		free(packet);
	}

	return 0;
}
