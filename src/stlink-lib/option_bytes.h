/**
  ******************************************************************************
  * @file             option_bytes.h
  * @brief            Read and write option bytes and option control registers
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */


#ifndef OPTION_BYTES_H
#define OPTION_BYTES_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <stlink.h>


/** @cond STLINK_INTERNAL */
/**
 * @addtogroup api_internal
 * @{
 */

/**
 * @name Family specific option byte access
 * Implementations behind the generic option byte functions (stlink_read_option_bytes32()
 * etc.), which select them by stlink_t::flash_type or stlink_t::chip_id. Each one reads
 * a 32 bit option byte or option control register of one device family into
 * @p option_byte and returns 0 on success, -1 on error.
 */
/** @{ */
/** STM32F0/F1/F3: read FLASH_OBR. */
int32_t stlink_read_option_control_register_f0(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_bytes_f0(stlink_t *sl, stm32_addr_t addr, uint8_t* base, uint32_t len);
// static int32_t stlink_write_option_control_register_f0(stlink_t *sl, uint32_t option_cr);
/** STM32F2: read FLASH_OPTCR. */
int32_t stlink_read_option_control_register_f2(stlink_t *sl, uint32_t *option_byte);
/** STM32F2: read the option bytes (FLASH_OPTCR). */
int32_t stlink_read_option_bytes_f2(stlink_t *sl, uint32_t *option_byte);
/** STM32F4: read FLASH_OPTCR. */
int32_t stlink_read_option_control_register_f4(stlink_t *sl, uint32_t *option_byte);
/** STM32F4: read the option bytes (FLASH_OPTCR). */
int32_t stlink_read_option_bytes_f4(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_bytes_f4(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
/** STM32F7: read the option bytes (last word of the option byte area). */
int32_t stlink_read_option_bytes_f7(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_bytes_f7(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
/** STM32F7: read FLASH_OPTCR. */
int32_t stlink_read_option_control_register_f7(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_control_register_f7(stlink_t *sl, uint32_t option_cr);
/** STM32F7: read FLASH_OPTCR1. */
int32_t stlink_read_option_control_register1_f7(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_control_register1_f7(stlink_t *sl, uint32_t option_cr1);
/** STM32F7: read the boot address option bytes (FLASH_OPTCR1). */
int32_t stlink_read_option_bytes_boot_add_f7(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_bytes_boot_add_f7(stlink_t *sl, uint32_t option_byte_boot_add);
/** STM32G0/G4: read FLASH_OPTR. */
int32_t stlink_read_option_control_register_gx(stlink_t *sl, uint32_t *option_byte);
/** STM32G0/G4: read the option bytes (FLASH_OPTR). */
int32_t stlink_read_option_bytes_gx(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_bytes_gx(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
// static int32_t stlink_write_option_bytes_h7(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
// static int32_t stlink_write_option_bytes_l0(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
// static int32_t stlink_write_option_bytes_l4(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
// static int32_t stlink_write_option_bytes_wb(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
/** STM32WB/WL: read FLASH_OPTR. */
int32_t stlink_read_option_control_register_wb(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_control_register_wb(stlink_t *sl, uint32_t option_cr);
/** All other devices: read the 32 bit word at stlink_t::option_base. */
int32_t stlink_read_option_bytes_generic(stlink_t *sl, uint32_t *option_byte);
/** @} */

/** @} */
/** @endcond */

/**
 * @addtogroup api_option_bytes
 * @{
 */

/**
 * Write option bytes.
 *
 * Unlocks the flash and the option bytes, programs them with the procedure of
 * the device family and locks both again. On STM32C0, F0, F1, F3, G0, G4, L0,
 * L1, L4, L5, U5, WB and WL the option bytes are then reloaded (OBL_LAUNCH),
 * which resets the target; on F2, F4, F7 and H7 they take effect at the next reset.
 *
 * Supported families: STM32C0, F0, F1, F2, F3, F4, F7, G0, G4, H7, L0, L1, L4,
 * L5, U5, WB, WL, if the chip description defines the option bytes
 * (stlink_t::option_base not 0; not the case for some F4 devices). The handling
 * of @p addr and @p len differs per family: STM32C0, F2, F4, G0, G4 and L4 write
 * only the first 32 bit word of @p base to the option register; STM32F0/F1/F3
 * require @p addr = 0x1FFFF800 and at least 12 bytes.
 *
 * @warning Wrong option bytes can lock the device permanently (read
 *          protection level 2, write protection, boot configuration).
 *
 * @param sl   device handle
 * @param addr address of the first option byte to write, inside the option byte area
 *             (stlink_t::option_base, stlink_t::option_size)
 * @param base option byte values, in the layout of the memory mapped option bytes
 * @param len  number of bytes
 * @return     0 on success, -1 on error or for an unsupported device
 */
int32_t stlink_write_option_bytes(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);

/**
 * Write option bytes from a binary file.
 *
 * As stlink_write_option_bytes(), for a file. Reports the file to the progress
 * handler (STLINK_PROGRESS_FILE).
 *
 * @bug Like the other fwrite functions it finally calls stlink_fwrite_finalize()
 *      with @p addr, i.e. sets the program counter to the option byte word at
 *      @p addr + 4 and runs the core.
 *
 * @param sl   device handle
 * @param path binary file
 * @param addr address of the first option byte to write
 * @return     0 on success, -1 on error
 */
int32_t stlink_fwrite_option_bytes(stlink_t *sl, const char *path, stm32_addr_t addr);

/**
 * Read the option control register.
 * Supported families: STM32C0, F0, F1, F3, F7, WB, WL.
 * @param sl          device handle
 * @param option_byte receives the register value
 * @return            0 on success, -1 on error or for an unsupported device
 */
int32_t stlink_read_option_control_register32(stlink_t *sl, uint32_t *option_byte);

/**
 * Write the option control register.
 * Supported families: STM32C0, F0, F1, F3, F7, WB, WL. Unlocks and locks the flash and the option bytes.
 * @param sl        device handle
 * @param option_cr new register value
 * @return          0 on success, -1 on error or for an unsupported device
 */
int32_t stlink_write_option_control_register32(stlink_t *sl, uint32_t option_cr);

/**
 * Read the option control register 1 (STM32F7 only).
 * @param sl          device handle
 * @param option_byte receives the register value
 * @return            0 on success, -1 on error or for an unsupported device
 */
int32_t stlink_read_option_control_register1_32(stlink_t *sl, uint32_t *option_byte);

/**
 * Write the option control register 1 (STM32F7 only).
 * Unlocks and locks the flash and the option bytes.
 * @param sl         device handle
 * @param option_cr1 new register value
 * @return           0 on success, -1 on error or for an unsupported device
 */
int32_t stlink_write_option_control_register1_32(stlink_t *sl, uint32_t option_cr1);

/**
 * Read the main option byte word.
 *
 * Selected by chip id: the option register for STM32C0, F2, F4 (chip ids 0x413
 * and 0x421), F76x/F77x, G0 (categories 1 and 2) and G4, the 32 bit word at
 * stlink_t::option_base for all other devices.
 *
 * @param sl          device handle
 * @param option_byte receives the value
 * @return            0 on success, -1 on error or if the device has no option bytes
 */
int32_t stlink_read_option_bytes32(stlink_t *sl, uint32_t* option_byte);

/**
 * Write the 32 bit option byte word at stlink_t::option_base.
 * Shorthand for stlink_write_option_bytes() with 4 bytes, so it fails on
 * STM32F0/F1/F3, which require at least 12 bytes.
 * @param sl          device handle
 * @param option_byte new value
 * @return            0 on success, -1 on error or for an unsupported device
 */
int32_t stlink_write_option_bytes32(stlink_t *sl, uint32_t option_byte);

/**
 * Read the boot address option bytes (STM32F7 only).
 * @param sl          device handle
 * @param option_byte receives the value
 * @return            0 on success, -1 on error or for an unsupported device
 */
int32_t stlink_read_option_bytes_boot_add32(stlink_t *sl, uint32_t* option_byte);

/**
 * Write the boot address option bytes (STM32F7 only).
 * Unlocks and locks the flash and the option bytes.
 * @param sl                    device handle
 * @param option_bytes_boot_add new value
 * @return                      0 on success, -1 on error or for an unsupported device
 */
int32_t stlink_write_option_bytes_boot_add32(stlink_t *sl, uint32_t option_bytes_boot_add);

/** @} */

#endif // OPTION_BYTES_H
