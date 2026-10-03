/**
  ******************************************************************************
  * @file           : gdb-remote.ch
  * @brief          : Tool: st-util
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @author         : Peter Zotov (whitequark)
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef GDB_REMOTE_H
#define GDB_REMOTE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <win32_socket.h>
#include <windows.h>
#else
#include <poll.h>
#include <unistd.h>
#endif // _WIN32


int32_t gdb_send_packet(int32_t fd, char* data);
int32_t gdb_recv_packet(int32_t fd, char** buffer);
int32_t gdb_check_for_interrupt(int32_t fd);

#endif // GDB_REMOTE_H
