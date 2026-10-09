/**
  ******************************************************************************
  * @file             chipid.h
  * @brief            Chip-ID parametres
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-07-27
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


/**
 * @addtogroup api_chipid
 * @{
 */

/**
 * Parameters of a device, read from a chip description file (*.chip).
 * See stlink_chipid_get_params() and the files in config/chips.
 */
struct stlink_chipid_params {
    char *dev_type;                   ///< name of the device, e.g. "F4xx"
    char *ref_manual_id;              ///< number of the reference manual (RMxxxx)
    uint32_t chip_id;                 ///< device id (DBGMCU_IDCODE.DEV_ID)
    enum stm32_flash_type flash_type; ///< kind of flash controller
    uint32_t flash_size_reg;          ///< address of the flash size register (in KiB); bit 1 set: size in the upper 16 bits
    uint32_t flash_pagesize;          ///< (smallest) flash page or sector size in bytes
    uint32_t sram_size;               ///< SRAM size in bytes
    uint32_t bootrom_base;            ///< address of the system memory (bootloader)
    uint32_t bootrom_size;            ///< size of the system memory
    uint32_t option_base;             ///< address of the option bytes, 0 if not supported
    uint32_t option_size;             ///< size of the option bytes
    uint32_t flags;                   ///< CHIP_F_* bits
    uint32_t otp_base;                ///< address of the OTP area, 0 if none
    uint32_t otp_size;                ///< size of the OTP area
    struct stlink_chipid_params *next; ///< next entry of the chip database (internal)
};

/**
 * Look up a device in the chip database.
 * @param chipid device id (stlink_t::chip_id)
 * @return       the parameters of the device, owned by the library, or NULL if the
 *               id is unknown or no chip description file was loaded
 */
struct stlink_chipid_params *stlink_chipid_get_params(uint32_t chipid);

/** @} */

/** @cond STLINK_INTERNAL */
/**
 * @addtogroup api_internal
 * @{
 */

/**
 * Log the parameters of a device at debug level.
 * @param dev the device
 */
void dump_a_chip(struct stlink_chipid_params *dev);

/**
 * Parse a chip description file and add the device to the chip database.
 * Errors are logged; the entry is added even if the file had errors.
 * @param fname path of the file
 */
void process_chipfile(char *fname);

/** @} */
/** @endcond */

/**
 * Load the chip database from the chip description files (*.chip).
 *
 * Call it once before opening a device: without it no target can be
 * identified. Replaces a previously loaded database. Not thread safe.
 *
 * If @p dir_to_scan is given, only this directory is read. Otherwise the first
 * of these directories containing chip files is used: the source tree
 * (debug builds only), the directory in the environment variable
 * STLINK_CHIPS_DIR, then "../share/stlink/config/chips" and "chips"
 * relative to the directory of the executable.
 *
 * @ingroup api_chipid
 * @param dir_to_scan directory with chip description files, or NULL or "" to search the default locations
 */
void init_chipids(char *dir_to_scan);

#endif // CHIPID_H
