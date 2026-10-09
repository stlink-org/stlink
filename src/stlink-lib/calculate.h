/**
  ******************************************************************************
  * @file             calculate.h
  * @brief            Calculation of sector numbers and pages
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-07-27
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


/** @cond STLINK_INTERNAL */
/**
 * @addtogroup api_internal
 * @{
 */

/**
 * Sector number of a flash address on STM32F2/F4 (sectors of 16, 64 and 128 KiB).
 * @param flashaddr address in flash
 * @return          the sector number; sectors in the second MiB (second bank) are numbered from 12
 */
uint32_t calculate_F4_sectornum(stlink_t *sl, uint32_t flashaddr);

/**
 * Sector number of a flash address on STM32F7 (sectors of 32, 128 and 256 KiB).
 * @param flashaddr address in flash
 * @return          the sector number
 */
uint32_t calculate_F7_sectornum(uint32_t flashaddr);

/**
 * Sector number of a flash address within its bank on STM32H7.
 * @param sl        device handle
 * @param flashaddr address in flash
 * @param bank      BANK_1 or BANK_2
 * @return          the sector number within the bank
 */
uint32_t calculate_H7_sectornum(stlink_t *sl, uint32_t flashaddr, uint32_t bank);

/**
 * Page number of a flash address on STM32L4 and similar devices, taking the
 * dual bank configuration (FLASH_OPTR) into account.
 * @param sl        device handle
 * @param flashaddr address in flash
 * @return          the page number as expected by FLASH_CR.PNB (incl. the bank bit)
 */
uint32_t calculate_L4_page(stlink_t *sl, uint32_t flashaddr);

/** @} */
/** @endcond */

#endif // CALCULATE_H
