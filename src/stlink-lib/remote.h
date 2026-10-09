/**
  ******************************************************************************
  * @file             remote.h
  * @brief            Remote backend and server dispatch
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @author           James Walmsley (jameswalmsley)
  * @date             2026-07-27
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
#include <windows.h>
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

#include "read_write.h"
#include "logging.h"


/** Default TCP port of st-server, see stlink_open_remote() @ingroup api_device */
#define STLINK_REMOTE_DEFAULT_PORT 4500

/** @cond STLINK_INTERNAL */
#define STLINK_REMOTE_MAGIC        0x4b4c5453 /* "STLK" */

/*
 * Version of the remote protocol: the stlink release that last changed it,
 * encoded as (major << 16) | (minor << 8) | patch. Client and server must use
 * the same version. History:
 *   1      first version
 *   1.9.1  STLINK_F_* flags in the handshake derived from the ST-LINK hardware
 *          table (stlink_hw.c): new flags, more flags set
 */
#define STLINK_REMOTE_PROTOCOL_VERSION_ENCODE(major, minor, patch) \
    (((uint32_t) (major) << 16) | ((uint32_t) (minor) << 8) | (uint32_t) (patch))
#define STLINK_REMOTE_PROTOCOL_VERSION STLINK_REMOTE_PROTOCOL_VERSION_ENCODE(1, 9, 1)

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

/** @endcond */

/**
 * @addtogroup api_device
 * @{
 */

/**
 * Open an ST-LINK attached to another machine running st-server.
 *
 * Connects via TCP, checks the handshake of the server and runs the same open
 * and connect sequence as stlink_open_usb(); the ST-LINK version is taken from
 * the handshake. All high-level logic (chip identification, flash loaders,
 * erase and programming) runs on this side, only the backend operations
 * (stlink_backend_t) are sent over the network. The connection is not
 * encrypted or authenticated.
 *
 * @param verbose log level, see stlink_open_usb()
 * @param host    host name or IPv4 address of the server
 * @param port    TCP port, 0 for STLINK_REMOTE_DEFAULT_PORT
 * @param connect how to connect to the target, see stlink_target_connect()
 * @param freq    SWD frequency in kHz, 0 for the default
 * @return        the device handle, to be released with stlink_close(), or NULL if the
 *                server cannot be reached, the handshake fails or the connection is
 *                lost while connecting
 */
stlink_t *stlink_open_remote(int32_t verbose, const char *host, int32_t port,
                             enum connect_type connect, int32_t freq);

/**
 * Open a remote ST-LINK given as "host" or "host:port".
 * As stlink_open_remote(); without a port STLINK_REMOTE_DEFAULT_PORT is used.
 * @param verbose  log level, see stlink_open_usb()
 * @param hostport "host" or "host:port"
 * @param connect  how to connect to the target, see stlink_target_connect()
 * @param freq     SWD frequency in kHz, 0 for the default
 * @return         the device handle, or NULL on error (also for an invalid port)
 */
stlink_t *stlink_open_remote_str(int32_t verbose, const char *hostport,
                                 enum connect_type connect, int32_t freq);

/**
 * Serve one client of st-server.
 *
 * Sends the handshake, then executes the backend requests of the client on the
 * local device @p sl until the client disconnects. Used by st-server.
 *
 * @bug A request for stlink_backend_t::enter_jtag_mode calls a NULL pointer
 *      with the libusb backend, so any client can crash the server.
 *
 * @param sl        local device, opened with stlink_open_usb()
 * @param client_fd connected socket of the client; not closed by this function
 * @return          0 when the client disconnects or the connection breaks while waiting
 *                  for a request, -1 if a reply cannot be sent or a request is too
 *                  large; other protocol errors are answered and serving continues
 */
int32_t stlink_remote_serve(stlink_t *sl, int32_t client_fd);

/** @} */

#endif // REMOTE_H
