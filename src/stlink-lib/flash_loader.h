/**
  ******************************************************************************
  * @file             flash_loader.h
  * @brief            Flash loaders
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */


#ifndef FLASH_LOADER_H
#define FLASH_LOADER_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <stm32.h>
#include <stm32_register.h>
#include <stlink.h>


/** @cond STLINK_INTERNAL */
/**
 * @addtogroup api_internal
 * @{
 */

/**
 * Halt the core with interrupts masked, copy the flash loader for the device
 * into SRAM and clear the fault status registers.
 * @param sl device handle
 * @param fl receives the loader and buffer addresses
 * @return   0 on success, -1 on error
 */
int32_t stlink_flash_loader_init(stlink_t *sl, flash_loader_t* fl);
// static int32_t loader_v_dependent_assignment(stlink_t *sl,
//                                             const uint8_t **loader_code, uint32_t *loader_size,
//                                             const uint8_t *high_v_loader, uint32_t high_v_loader_size,
//                                             const uint8_t *low_v_loader, uint32_t low_v_loader_size);

/**
 * Copy the flash loader for the device to the start of the SRAM.
 * @param sl   device handle
 * @param addr receives the address of the loader
 * @param size receives the size of the loader in bytes
 * @return     0 on success, non-zero on error (e.g. no loader for the device)
 */
int32_t stlink_flash_loader_write_to_sram(stlink_t *sl, stm32_addr_t* addr, uint32_t* size);

/**
 * Program one block with the flash loader and wait for it to finish.
 * @param sl     device handle
 * @param fl     flash loader, see stlink_flash_loader_init()
 * @param target destination address in flash
 * @param buf    data
 * @param size   number of bytes, at most the size of the loader buffer
 * @return       0 on success, -1 on error or timeout
 */
int32_t stlink_flash_loader_run(stlink_t *sl, flash_loader_t* fl, stm32_addr_t target, const uint8_t* buf, uint32_t size);


/* === Functions from old header file flashloader.h === */

/**
 * Program STM32L0/L1 flash in half pages, with the flash loader or, if it
 * fails on the first half page, with direct memory writes. Only whole half
 * pages are written; the caller writes the remainder.
 * @param sl       device handle
 * @param fl       flash loader
 * @param addr     destination address
 * @param base     data
 * @param len      number of bytes; len / pagesize half pages are written
 * @param pagesize size of a half page
 * @return         0 on success, -1 on error
 */
int32_t stm32l1_write_half_pages(stlink_t *sl, flash_loader_t *fl, stm32_addr_t addr, uint8_t *base, uint32_t len, uint32_t pagesize);
// static void set_flash_cr_pg(stlink_t *sl, uint32_t bank);
// static void set_dma_state(stlink_t *sl, flash_loader_t *fl, int32_t bckpRstr);

/** @} */
/** @endcond */

/**
 * @addtogroup api_flash
 * @{
 */

/**
 * Prepare the flash for programming.
 *
 * Disables the DMA of the target (except on STM32H5), waits for the flash
 * controller, clears its error flags and unlocks the flash. Depending on the
 * device it halts the core with interrupts masked and loads a flash loader into
 * SRAM (STM32F0/F1/F3, F2/F4/F7, L0/L1, L4, H5, WB0), and sets the controller up
 * for programming (e.g. the programming parallelism from the target voltage on
 * STM32F2/F4/F7). On the other devices a running core keeps running: halt it
 * before with stlink_force_debug().
 *
 * The flash must be erased before stlink_flashloader_write(). Always finish
 * with stlink_flashloader_stop(), also after an error.
 *
 * @param sl device handle
 * @param fl storage for the state of the flash loader, used by the next calls
 * @return   0 on success, -1 on error (e.g. target voltage too low, unknown device)
 */
int32_t stlink_flashloader_start(stlink_t *sl, flash_loader_t *fl);

/**
 * Program data into erased flash or OTP memory.
 *
 * Requires a successful stlink_flashloader_start(). Can be called several
 * times before stlink_flashloader_stop(). Does not erase and does not verify.
 *
 * @param sl   device handle
 * @param fl   state from stlink_flashloader_start()
 * @param addr destination address, aligned to the programming unit of the device
 * @param base data
 * @param len  number of bytes
 * @return     0 on success, -1 on error
 *
 * @bug Reads past the end of @p base: up to 15 bytes on STM32L5/U5 (the length
 *      is rounded up to 16 bytes) and up to 3 bytes on STM32L0/L1 (word reads).
 *      On STM32H7 the last partial 64 byte block is completed with stale data
 *      from stlink_t::q_buf, which is programmed behind the data (not caught by
 *      the verification, which only compares @p len bytes).
 */
int32_t stlink_flashloader_write(stlink_t *sl, flash_loader_t *fl, stm32_addr_t addr, uint8_t *base, uint32_t len);

/**
 * Finish programming.
 *
 * Clears the programming bit, locks the flash, unmasks the interrupts of the
 * core and restores the DMA state saved by stlink_flashloader_start(). The core
 * is not resumed.
 *
 * @param sl device handle
 * @param fl state from stlink_flashloader_start()
 * @return   0
 */
int32_t stlink_flashloader_stop(stlink_t *sl, flash_loader_t *fl);

/** @} */

#endif // FLASH_LOADER_H
