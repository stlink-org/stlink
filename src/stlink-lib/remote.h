/**
  ******************************************************************************
  * @file           : remote.h
  * @brief          : Remote backend and server dispatch
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @author         : James Walmsley (jameswalmsley)
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

/*
 * Remote backend: tunnels the stlink backend operations over TCP so that
 * st-flash / st-info can drive an ST-LINK attached to another machine
 * (the server, st-server). The client runs all the high-level logic
 * (chip-id lookup, flash loaders, erase/program sequencing) and only the
 * low-level backend primitives cross the wire.
 */

#ifndef REMOTE_H
#define REMOTE_H


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(_WIN32)
#include <win32_socket.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif // _WIN32

#include <stlink.h>
#include <stlink_backend.h>

#include "remote.h"
#include "read_write.h"
#include "logging.h"


#define STLINK_REMOTE_DEFAULT_PORT 4500
#define STLINK_REMOTE_MAGIC        0x4b4c5453 /* "STLK" */
#define STLINK_REMOTE_PROTOCOL_VERSION 1

/*
 * Fixed on-wire size of the handshake serial field. Deliberately decoupled
 * from STLINK_SERIAL_BUFFER_SIZE so that changing the internal serial buffer
 * cannot silently alter the protocol layout. The buffer must fit within it; if
 * a future ST-LINK serial needs more room, enlarge this AND bump
 * STLINK_REMOTE_PROTOCOL_VERSION.
 */
#define STLINK_REMOTE_SERIAL_WIRE_LEN 32

#if STLINK_SERIAL_BUFFER_SIZE > STLINK_REMOTE_SERIAL_WIRE_LEN
#error "STLINK_SERIAL_BUFFER_SIZE exceeds the remote serial wire field; \
enlarge STLINK_REMOTE_SERIAL_WIRE_LEN and bump STLINK_REMOTE_PROTOCOL_VERSION"
#endif

enum stlink_remote_reply_status {
    REMOTE_REPLY_OK = 0,
    REMOTE_REPLY_PROTOCOL_ERROR = 1,
};

/* Backend operations, one opcode per stlink_backend_t function pointer. */
enum stlink_remote_op {
    RPC_EXIT_DEBUG = 1,
    RPC_ENTER_SWD,
    RPC_ENTER_JTAG,
    RPC_EXIT_DFU,
    RPC_CORE_ID,
    RPC_RESET,
    RPC_JTAG_RESET,
    RPC_RUN,
    RPC_STATUS_REMOTE,
    RPC_VERSION_REMOTE,
    RPC_READ_DEBUG32,
    RPC_READ_MEM32,
    RPC_WRITE_DEBUG32,
    RPC_WRITE_MEM32,
    RPC_WRITE_MEM8,
    RPC_READ_ALL_REGS,
    RPC_READ_REG,
    RPC_READ_ALL_UNSUPPORTED_REGS,
    RPC_READ_UNSUPPORTED_REG,
    RPC_WRITE_UNSUPPORTED_REG,
    RPC_WRITE_REG,
    RPC_STEP,
    RPC_CURRENT_MODE,
    RPC_FORCE_DEBUG,
    RPC_TARGET_VOLTAGE,
    RPC_SET_SWDCLK,
    RPC_INIT_AP,
    RPC_TRACE_ENABLE,
    RPC_TRACE_DISABLE,
    RPC_TRACE_READ,
    RPC_CLOSE,
};

/*
 * Client: open a remote ST-LINK over TCP and run the same open/connect
 * sequence as stlink_open_usb (version is taken from the server handshake).
 * Returns NULL on failure.
 */
stlink_t *stlink_open_remote(int32_t verbose, const char *host, int32_t port,
                             enum connect_type connect, int32_t freq);

/* As stlink_open_remote, but parses a "host" or "host:port" string. */
stlink_t *stlink_open_remote_str(int32_t verbose, const char *hostport,
                                 enum connect_type connect, int32_t freq);

/*
 * Server: send the handshake then serve backend requests for one connected
 * client against an already-open local stlink, until the client disconnects.
 * Returns 0 on a clean disconnect, -1 on a transport error.
 */
int32_t stlink_remote_serve(stlink_t *sl, int32_t client_fd);

#endif // REMOTE_H
