/**
  ******************************************************************************
  * @file             read_write.h
  * @brief            Read and write operations
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef READ_WRITE_H
#define READ_WRITE_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <stlink.h>
#include <stlink_backend.h>


/**
 * @addtogroup api_util
 * @{
 */

/**
 * Decode a little endian 16 bit value.
 * @param c  buffer
 * @param pt offset of the value in @p c
 * @return the value
 */
uint16_t read_uint16(const unsigned char *c, const int32_t pt);

/**
 * Encode a 16 bit value little endian.
 * @param buf destination, 2 bytes
 * @param ui  the value
 */
void write_uint16(unsigned char *buf, uint16_t ui);

/**
 * Decode a little endian 32 bit value, e.g. from stlink_t::q_buf after stlink_read_mem32().
 * @param c  buffer
 * @param pt offset of the value in @p c
 * @return the value
 */
uint32_t read_uint32(const unsigned char *c, const int32_t pt);

/**
 * Encode a 32 bit value little endian, e.g. into stlink_t::q_buf before stlink_write_mem32().
 * @param buf destination, 4 bytes
 * @param ui  the value
 */
void write_uint32(unsigned char *buf, uint32_t ui);

/** @} */

/**
 * @addtogroup api_memory
 * @{
 */

/**
 * Read a 32 bit word from the target.
 *
 * Suited for registers (core debug, peripherals, flash controller). For blocks
 * of memory use stlink_read_mem32().
 *
 * @note On targets whose CPU is on access port 1 (e.g. STM32H5) and for secure
 *       addresses the USB backend reads through the memory path, which does
 *       not detect failed accesses on the target (see stlink_read_mem32()).
 *
 * @param sl   device handle
 * @param addr address, word aligned
 * @param data receives the word
 * @return 0 on success, -1 on error
 */
int32_t stlink_read_debug32(stlink_t *sl, uint32_t addr, uint32_t *data);

/**
 * Write a 32 bit word to the target.
 * @param sl   device handle
 * @param addr address, word aligned
 * @param data the word
 * @return 0 on success, -1 on error
 */
int32_t stlink_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data);

/**
 * Read a block of target memory into stlink_t::q_buf.
 *
 * The data is placed at the start of stlink_t::q_buf and stays valid until the
 * next operation on the device.
 *
 * @note The USB backend does not check whether the access on the target
 *       succeeded: after a bus or access port fault the function returns 0 and
 *       the data is undefined.
 *
 * @param sl   device handle
 * @param addr address, word aligned
 * @param len  number of bytes, a multiple of 4. Keep blocks at 6 KiB or below:
 *             larger reads can stall an ST-LINK/V2. Never above Q_BUF_LEN.
 * @return 0 on success, -1 on a transport error or if @p len is not a multiple of 4
 */
int32_t stlink_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len);

/**
 * Write a block from stlink_t::q_buf to target memory with 32 bit accesses.
 *
 * Copy the data to the start of stlink_t::q_buf before the call. Does not
 * program flash; see the @ref api_flash functions for that.
 *
 * @param sl   device handle
 * @param addr address, word aligned
 * @param len  number of bytes, a multiple of 4, at most Q_BUF_LEN
 * @return 0 on success, -1 on error or if @p len is not a multiple of 4
 */
int32_t stlink_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len);

/**
 * Write a block from stlink_t::q_buf to target memory with 8 bit accesses.
 *
 * For data that is not a multiple of 4 bytes or not word aligned. Copy the data
 * to the start of stlink_t::q_buf before the call. The USB backend does not
 * check whether the access on the target succeeded.
 *
 * @param sl   device handle
 * @param addr address
 * @param len  number of bytes: at most 64 (ST-LINK/V1, V2) or 512 (ST-LINK/V3);
 *             not checked by the legacy ST-LINK/V1 backend
 * @return 0 on success, -1 on a transport error or if @p len exceeds the limit
 */
int32_t stlink_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len);

/**
 * Read a core register of the halted core.
 * @param sl    device handle
 * @param r_idx 0..15: R0..R15, 16: xPSR, 17: MSP, 18: PSP, 19, 20: see stlink_reg::rw
 * @param regp  receives the value in the field matching @p r_idx; other fields are not changed
 * @return 0 on success, -1 on error or if @p r_idx is out of range
 */
int32_t stlink_read_reg(stlink_t *sl, int32_t r_idx, struct stlink_reg *regp);

/**
 * Write a core register of the halted core.
 * @param sl  device handle
 * @param reg new value
 * @param idx register index, as for stlink_read_reg() (15 = PC)
 * @return 0 on success, -1 on error
 */
int32_t stlink_write_reg(stlink_t *sl, uint32_t reg, int32_t idx);

/**
 * Read a special or FPU register of the halted core.
 *
 * These registers are not covered by the register commands of the ST-LINK
 * firmware and are accessed through DCRSR/DCRDR (ARMv7-M ARM, C1.6). The
 * register numbering is the one of GDB for Cortex-M targets with FPU.
 *
 * @param sl    device handle
 * @param r_idx 0x1c: CONTROL, 0x1d: FAULTMASK, 0x1e: BASEPRI, 0x1f: PRIMASK
 *              (all four are read together), 0x20..0x3f: S0..S31, 0x40: FPSCR
 * @param regp  receives the value(s) in the matching field(s) of struct stlink_reg
 * @return 0 on success, -1 on error or if @p r_idx is out of range
 */
int32_t stlink_read_unsupported_reg(stlink_t *sl, int32_t r_idx, struct stlink_reg *regp);

/**
 * Write a special or FPU register of the halted core.
 *
 * Uses DCRSR/DCRDR, see stlink_read_unsupported_reg(). CONTROL, FAULTMASK,
 * BASEPRI and PRIMASK share one register: the other three are read first and
 * written back unchanged.
 *
 * @warning Not implemented by the legacy ST-LINK/V1 backend (stlink_v1_open());
 *          the call then dereferences a NULL pointer. The same applies to
 *          stlink_read_unsupported_reg() and stlink_read_all_unsupported_regs().
 *
 * @param sl    device handle
 * @param value new value; for 0x1c..0x1f the 8 bit value is taken from bits 31..24
 * @param r_idx register index, as for stlink_read_unsupported_reg()
 * @param regp  scratch, receives the values read for 0x1c..0x1f
 * @return 0 on success, -1 on error or if @p r_idx is out of range
 */
int32_t stlink_write_unsupported_reg(stlink_t *sl, uint32_t value, int32_t r_idx, struct stlink_reg *regp);

/**
 * Read R0..R15, xPSR, MSP, PSP and the registers 19 and 20 of the halted core.
 * @param sl   device handle
 * @param regp receives the registers
 * @return 0 on success, -1 on error
 */
int32_t stlink_read_all_regs(stlink_t *sl, struct stlink_reg *regp);

/**
 * Read CONTROL, FAULTMASK, BASEPRI, PRIMASK, FPSCR and S0..S31 of the halted core.
 * @param sl   device handle
 * @param regp receives the registers
 * @return 0 on success, -1 on error
 */
int32_t stlink_read_all_unsupported_regs(stlink_t *sl, struct stlink_reg *regp);

/** @} */

#endif // READ_WRITE_H
