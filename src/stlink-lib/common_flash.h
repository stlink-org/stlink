/**
  ******************************************************************************
  * @file             common_flash.h
  * @brief            Flash operations
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */


#ifndef COMMON_FLASH_H
#define COMMON_FLASH_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <stlink.h>


/**
 * @addtogroup api_flash
 * @{
 */

/** @name Flash bank selectors */
/** @{ */
#define BANK_1 0 ///< first flash bank
#define BANK_2 1 ///< second flash bank (dual bank devices)
/** @} */

/** Erase step of the flash write functions */
enum erase_type_t {
    NO_ERASE = 0,      ///< do not erase, the area must be erased already
    SECTION_ERASE = 1, ///< erase the pages covered by the data before writing
    MASS_ERASE = 2,    ///< the caller erases the whole flash (stlink_erase_flash_mass()), no erase in the write functions
};

/** @} */

/** @cond STLINK_INTERNAL */
/**
 * @addtogroup api_internal
 * @{
 */

/**
 * Base address of the flash registers of an STM32L0 or L1.
 * @param sl device handle
 * @return   the register base for the chip id; the L0 base for unknown chips
 */
uint32_t get_stm32l0_flash_base(stlink_t *sl);

/**
 * Address of the flash control register (NSCR or SECCR) of an STM32L5/U5.
 * Depends on stlink_t::flash_secure, see stlink_flash_secure_enable().
 * @param sl device handle
 * @return   the register address
 */
uint32_t get_stm32l5_flash_cr(stlink_t *sl);

/**
 * Address of the flash status register (NSSR or SECSR) of an STM32L5/U5.
 * Depends on stlink_t::flash_secure, see stlink_flash_secure_enable().
 * @param sl device handle
 * @return   the register address
 */
uint32_t get_stm32l5_flash_sr(stlink_t *sl);

/** @} */
/** @endcond */

/**
 * @addtogroup api_flash
 * @{
 */

/**
 * Check whether an address is in the secure flash alias of an STM32L5/U5.
 * @param sl   device handle
 * @param addr address
 * @return     true for an STM32L5/U5 and an address in the secure flash alias (0x0c000000 + flash size)
 */
bool stlink_is_secure_flash_addr(stlink_t *sl, stm32_addr_t addr);

/**
 * Program the flash of an STM32L5/U5 through the secure flash alias.
 *
 * With TrustZone enabled (FLASH_OPTR.TZEN = 1) secure flash pages can only be
 * erased and programmed through the secure alias (0x0c000000) and the secure
 * flash registers, using secure transfers. On success stlink_t::flash_base is
 * switched to the secure alias for all subsequent flash operations. The secure
 * area itself (SECWM option bytes) is not changed.
 *
 * Requires ST-LINK firmware V2J32 / V3J2 or later (STLINK_F_HAS_CSW) and RDP level 0.
 *
 * @param sl device handle
 * @return   0 on success, -1 if the device, the ST-LINK firmware, TrustZone or the RDP level does not allow it
 */
int32_t stlink_flash_secure_enable(stlink_t *sl);

/** @} */

/** @cond STLINK_INTERNAL */
/**
 * @addtogroup api_internal
 * @{
 */

/**
 * Read the flash control register.
 * @param sl   device handle
 * @param bank BANK_1 or BANK_2
 * @return     the register value
 */
uint32_t read_flash_cr(stlink_t *sl, uint32_t bank);

/**
 * Lock the flash control register (all banks).
 * @param sl device handle
 */
void lock_flash(stlink_t *sl);
// static inline int32_t write_flash_sr(stlink_t *sl, uint32_t bank, uint32_t val);

/**
 * Clear the latched error flags in the flash status register(s).
 * @param sl device handle
 */
void clear_flash_error(stlink_t *sl);

/**
 * Read the flash status register.
 * @param sl   device handle
 * @param bank BANK_1 or BANK_2
 * @return     the register value
 */
uint32_t read_flash_sr(stlink_t *sl, uint32_t bank);

/**
 * Check whether the flash controller is busy.
 * @param sl device handle
 * @return   the busy bit(s), 0 if idle
 */
uint32_t is_flash_busy(stlink_t *sl);

/**
 * Wait until the flash controller is idle; gives up after 30 s (logged).
 * @param sl device handle
 */
void wait_flash_busy(stlink_t *sl);

/**
 * Check the flash status register(s) for errors and log them.
 * @param sl device handle
 * @return   0 if no error flag is set, -1 otherwise
 */
int32_t check_flash_error(stlink_t *sl);
// static inline uint32_t is_flash_locked(stlink_t *sl);
// static void unlock_flash(stlink_t *sl);

/**
 * Unlock the flash control register if it is locked.
 * @param sl device handle
 * @return   0 on success, -1 if it is still locked (requires a reset to unlock again)
 */
int32_t unlock_flash_if(stlink_t *sl);

/**
 * Lock the option byte control register.
 * @param sl device handle
 * @return   0 on success, -1 for an unsupported flash type
 */
int32_t lock_flash_option(stlink_t *sl);
// static bool is_flash_option_locked(stlink_t *sl);
// static int32_t unlock_flash_option(stlink_t *sl);

/**
 * Unlock the option byte control register if it is locked.
 * @param sl device handle
 * @return   0 on success, -1 on error
 */
int32_t unlock_flash_option_if(stlink_t *sl);

/**
 * Set the programming parallelism (PSIZE) in the flash control register (STM32F2/F4/F7/H7).
 * @param sl   device handle
 * @param n    PSIZE value
 * @param bank BANK_1 or BANK_2
 */
void write_flash_cr_psiz(stlink_t *sl, uint32_t n, uint32_t bank);

/**
 * Clear the programming bit (PG) in the flash control register.
 * @param sl   device handle
 * @param bank BANK_1 or BANK_2
 */
void clear_flash_cr_pg(stlink_t *sl, uint32_t bank);
// static void wait_flash_busy_progress(stlink_t *sl);
// static inline void write_flash_ar(stlink_t *sl, uint32_t n, uint32_t bank);
// static inline void write_flash_cr_snb(stlink_t *sl, uint32_t n, uint32_t bank);
// static void set_flash_cr_per(stlink_t *sl, uint32_t bank);
// static void clear_flash_cr_per(stlink_t *sl, uint32_t bank);
// static inline void write_flash_cr_bker_pnb(stlink_t *sl, uint32_t n);
// static void set_flash_cr_strt(stlink_t *sl, uint32_t bank);
// static void set_flash_cr_mer(stlink_t *sl, bool v, uint32_t bank);

/** @} */
/** @endcond */

/**
 * @addtogroup api_flash
 * @{
 */

/**
 * Erase the flash page or sector containing an address.
 *
 * Unlocks the flash, erases and locks it again. The target description must
 * have been loaded (stlink_load_device_params(), done by the open functions).
 *
 * @param sl        device handle
 * @param flashaddr an address in the page or sector to erase
 * @return          0 on success, -1 on error or for an unsupported flash type
 */
int32_t stlink_erase_flash_page(stlink_t *sl, stm32_addr_t flashaddr);

/**
 * Erase all flash pages of an address range.
 *
 * Reports every erased page to the progress handler (STLINK_PROGRESS_PAGE_ERASED)
 * and STLINK_PROGRESS_ERASE_DONE at the end.
 *
 * @param sl         device handle
 * @param base_addr  start address, must be the start of a page
 * @param size       number of bytes
 * @param align_size true: erase the last page completely even if the range ends
 *                   inside it; false: fail if the range does not end on a page
 *                   boundary (checked at the last page, when the pages before
 *                   it have been erased already)
 * @return           0 on success, -1 on error (range outside the flash, misaligned, erase failed)
 */
int32_t stlink_erase_flash_section(stlink_t *sl, stm32_addr_t base_addr, uint32_t size, bool align_size);

/**
 * Erase the whole flash.
 *
 * Uses the mass erase of the flash controller where available (both banks of
 * dual bank devices) and erases page by page otherwise (STM32L0/L1, WB/WL,
 * WB05, WL3x). Progress is reported to the progress handler.
 *
 * @param sl device handle
 * @return   0 on success, -1 on error (errors of the mass erase command of
 *           STM32WB06/WB07/WB09 are not checked)
 */
int32_t stlink_erase_flash_mass(stlink_t *sl);

/**
 * Program a buffer into flash and start the target.
 *
 * Runs stlink_write_flash() (erase as requested, program, verify), then sets
 * the program counter to the word at @p addr + 4 (the reset vector of a vector
 * table at @p addr) and runs the core, also if writing failed.
 *
 * With stlink_t::opt set, trailing bytes equal to the erased pattern are not
 * written.
 *
 * @param sl     device handle
 * @param data   data to write
 * @param length number of bytes
 * @param addr   destination, the start of a flash page
 * @param erase  SECTION_ERASE to erase the pages first, NO_ERASE or MASS_ERASE
 *               if the flash was erased before
 * @return       0 on success, -1 on error
 */
int32_t stlink_mwrite_flash(stlink_t *sl, uint8_t *data, uint32_t length,
                            stm32_addr_t addr, const enum erase_type_t erase);

/**
 * Program a binary file into flash and start the target.
 *
 * As stlink_mwrite_flash(), for a file. An address inside the OTP area of the
 * device is programmed with stlink_write_otp() instead (no erase). Reports the
 * file to the progress handler (STLINK_PROGRESS_FILE).
 *
 * @bug For an address in the OTP area the core is started as well, with the
 *      program counter taken from the OTP data at @p addr + 4.
 *
 * @param sl    device handle
 * @param path  binary file
 * @param addr  destination, the start of a flash page or inside the OTP area
 * @param erase SECTION_ERASE to erase the pages first, NO_ERASE or MASS_ERASE
 *              if the flash was erased before
 * @return      0 on success, -1 on error
 */
int32_t stlink_fwrite_flash(stlink_t *sl, const char *path, stm32_addr_t addr,
                            const enum erase_type_t erase);

/**
 * Compare a binary file with the target memory.
 * @param sl   device handle
 * @param path binary file
 * @param addr address of the first byte to compare
 * @return     0 if the memory matches the file, -1 if it differs or the file cannot be read
 */
int32_t stlink_fcheck_flash(stlink_t *sl, const char *path, stm32_addr_t addr);

/**
 * Compare a buffer with the target memory.
 * @param sl      device handle
 * @param address address of the first byte to compare
 * @param data    expected data
 * @param length  number of bytes
 * @return        0 if the memory matches, -1 at the first difference
 */
int32_t stlink_verify_write_flash(stlink_t *sl, stm32_addr_t address, uint8_t *data, uint32_t length);

/**
 * Check that an address range is inside the flash.
 * @param sl   device handle
 * @param addr start address
 * @param size number of bytes
 * @return     0 if the range is inside the flash, -1 otherwise (logged)
 */
int32_t stlink_check_address_range_validity(stlink_t *sl, stm32_addr_t addr, uint32_t size);

/**
 * Check that an address range is inside the OTP area.
 * @param sl   device handle
 * @param addr start address
 * @param size number of bytes
 * @return     0 if the range is inside the OTP area, -1 otherwise (logged)
 * @bug A range ending exactly at the end of the OTP area is rejected.
 */
int32_t stlink_check_address_range_validity_otp(stlink_t *sl, stm32_addr_t addr, uint32_t size);

/**
 * Check that an address is the start of a flash page or sector.
 * @param sl   device handle
 * @param addr address in flash
 * @return     0 if @p addr starts a page, -1 otherwise
 */
int32_t stlink_check_address_alignment(stlink_t *sl, stm32_addr_t addr);

/**
 * Erase (optionally), program and verify flash.
 *
 * Checks the range, erases the covered pages for SECTION_ERASE, programs the
 * data with the flash loader (stlink_flashloader_start(), stlink_flashloader_write(),
 * stlink_flashloader_stop()) and verifies it with stlink_verify_write_flash().
 * The core is not resumed. It is halted only on devices programmed through a
 * flash loader in SRAM (STM32F0/F1/F3, F2/F4/F7, L0/L1, L4, H5, WB0); halt it
 * with stlink_force_debug() before, as st-flash does.
 *
 * @param sl         device handle
 * @param addr       destination, the start of a flash page
 * @param base       data to write
 * @param len        number of bytes; an odd length is padded by one byte on
 *                   most devices, so @p base must have room for it
 * @param erase_only only erase the pages (with SECTION_ERASE), do not program
 * @param erase      SECTION_ERASE to erase the pages first, NO_ERASE or MASS_ERASE
 *                   if the flash was erased before
 * @return           0 on success, -1 on error
 *
 * @bug For an odd @p len the check that @p addr starts a page is skipped, and
 *      the padding byte is read from @p base[len] (zero only for a file mapped
 *      with mmap(), not on Windows). See stlink_flashloader_write() for more
 *      reads past the end of @p base.
 */
int32_t stlink_write_flash(stlink_t *sl, stm32_addr_t addr, uint8_t *base,
                           uint32_t len, uint8_t erase_only,
                           const enum erase_type_t erase);

/**
 * Program and verify the OTP area.
 *
 * OTP memory can be programmed only once and cannot be erased.
 *
 * @param sl   device handle
 * @param addr destination inside the OTP area
 * @param base data to write
 * @param len  number of bytes
 * @return     0 on success, -1 on error
 */
int32_t stlink_write_otp(stlink_t *sl, stm32_addr_t addr, uint8_t *base,
                         uint32_t len);

/** @} */

/** @cond STLINK_INTERNAL */
/**
 * Start the written image: set the program counter to the word at @p addr + 4 and run the core.
 * Called at the end of the stlink_mwrite_* and stlink_fwrite_* functions.
 * @ingroup api_internal
 * @param sl   device handle
 * @param addr address of the vector table of the image
 */
void stlink_fwrite_finalize(stlink_t *sl, stm32_addr_t addr);
/** @endcond */

#endif // COMMON_FLASH_H
