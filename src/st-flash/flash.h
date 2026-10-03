/**
  ******************************************************************************
  * @file           : flash.h
  * @brief          : Tool: st-flash
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef FLASH_H
#define FLASH_H

#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#else
#include <unistd.h>
#endif // _WIN32

#include <stlink.h>
#include <stm32.h>

#include <chipid.h>
#include <common_flash.h>
#include <map_file.h>
#include <option_bytes.h>
#include <remote.h>
#include <usb.h>


#define DEBUG_LOG_LEVEL 100
#define STND_LOG_LEVEL  50
#define ENABLE_OPT      1

// static stlink_t *connected_stlink = NULL;
// static void cleanup(int32_t signum);
// static void usage(void);

#endif // FLASH_H
