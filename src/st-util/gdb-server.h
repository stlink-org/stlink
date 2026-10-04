/**
  ******************************************************************************
  * @file           : gdb-server.h
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

#ifndef GDB_SERVER_H
#define GDB_SERVER_H

#include <ctype.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#if defined(_MSC_VER)
#include <stdbool.h>
#define __attribute__(x)
#endif

#if defined(_WIN32)
#include <windows.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif // _WIN32

#include <stlink.h>
#include <stm32_register.h>

#include <chipid.h>
#include <common_flash.h>
#include <flash_loader.h>
#include <helper.h>
#include <logging.h>
#include <read_write.h>
#include <remote.h>
#include <usb.h>


#define STRINGIFY_inner(name) #name
#define STRINGIFY(name) STRINGIFY_inner(name)

#define DEFAULT_LOGGING_LEVEL 50
#define DEBUG_LOGGING_LEVEL 100
#define DEFAULT_GDB_LISTEN_PORT 4242

#endif // GDB_SERVER_H
