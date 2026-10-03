/**
  ******************************************************************************
  * @file           : info.h
  * @brief          : Tool: st-info
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef INFO_H
#define INFO_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <stlink.h>

#include <chipid.h>
#include <helper.h>
#include <remote.h>
#include <usb.h>


// static void usage(void);
// static void stlink_print_version(stlink_t *sl);
// static void stlink_print_info(stlink_t *sl);

// static void stlink_probe(enum connect_type connect, int32_t freq) { };
static int32_t print_data(int32_t ac, char **av);
int32_t main(int32_t ac, char** av);

#endif // INFO_H