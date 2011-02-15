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
#include "gdb-remote.h"
#include "stlink-hw.h"

static const char hex[] = "0123456789abcdef";

int main(int argc, char** argv) {
	struct sockaddr_in serv_addr;

	if(argc != 3) {
		fprintf(stderr, "Usage: %s <port> /dev/sgX\n", argv[0]);
		return 1;
	}

	int port = atoi(argv[1]);

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
		perror("socket");
		return 1;
	}

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	if(bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("bind");
		return 1;
	}

	if(listen(sock, 5) < 0) {
		perror("listen");
		return 1;
	}

	struct stlink *sl = stlink_quirk_open(argv[2], 0);
	if (sl == NULL)
		return 1;

	if(stlink_current_mode(sl) != STLINK_DEV_DEBUG_MODE)
		stlink_enter_swd_mode(sl);

	stlink_core_id(sl);
	printf("Debugging ARM core %08x.\n", sl->core_id);

	stlink_force_debug(sl);
	stlink_reset(sl);

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

		//printf("recv: %s\n", packet);

		char* reply = NULL;

		switch(packet[0]) {
		case 'c':
			stlink_run(sl);

			printf("Core running, waiting for interrupt.\n");

			int status = gdb_wait_for_interrupt(client);
			if(status < 0) {
				fprintf(stderr, "cannot wait for int: %d\n", status);
				return 1;
			}

			stlink_force_debug(sl);

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

		default:
			reply = strdup("");
		}

		if(reply) {
			//printf("send: %s\n", reply);

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
