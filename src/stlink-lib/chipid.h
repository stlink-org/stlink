/**
  ******************************************************************************
  * @file           : chipid.h
  * @brief          : Chip-ID parametres
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef CHIPID_H
#define CHIPID_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stm32.h>
#include <stlink.h>


/* Chipid parametres */
struct stlink_chipid_params {
    char *dev_type;
    char *ref_manual_id;
    uint32_t chip_id;
    enum stm32_flash_type flash_type;
    uint32_t flash_size_reg;
    uint32_t flash_pagesize;
    uint32_t sram_size;
    uint32_t bootrom_base;
    uint32_t bootrom_size;
    uint32_t option_base;
    uint32_t option_size;
    uint32_t flags;
    uint32_t otp_base;
    uint32_t otp_size;
  struct stlink_chipid_params *next;
};

struct stlink_chipid_params *stlink_chipid_get_params(uint32_t chipid);

void dump_a_chip(struct stlink_chipid_params *dev);
void process_chipfile(char *fname);
void init_chipids(char *dir_to_scan);

#endif // CHIPID_H
