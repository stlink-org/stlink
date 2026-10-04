/**
  ******************************************************************************
  * @file           : md5.h
  * @brief          : MD5 hash function
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef MD5_H
#define MD5_H

#include <stdint.h>
#include <stdio.h>

#include <stlink.h>

#include "map_file.h"


void md5_calculate(mapped_file_t *);
void stlink_checksum(mapped_file_t *);

#endif // MD5_H