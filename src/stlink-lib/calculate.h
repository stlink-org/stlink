/**
  ******************************************************************************
  * @file           : calculate.h
  * @brief          : Calculation of sector numbers and pages
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef CALCULATE_H
#define CALCULATE_H

#include <stdint.h>

#include <stlink.h>


uint32_t calculate_F4_sectornum(uint32_t);
uint32_t calculate_F7_sectornum(uint32_t);
uint32_t calculate_H7_sectornum(stlink_t *, uint32_t, uint32_t);
uint32_t calculate_L4_page(stlink_t *, uint32_t);

#endif // CALCULATE_H
