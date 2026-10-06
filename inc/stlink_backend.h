/**
  ******************************************************************************
  * @file             stlink_backend.h
  * @brief            stlink backend
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef STLINK_BACKEND_H
#define STLINK_BACKEND_H

#include <stdint.h>

    /**
     * Operations a backend implements to access an ST-LINK.
     *
     * The library has three backends: libusb (usb.c, all ST-LINK versions),
     * remote (remote.c, tunnels these operations over TCP to st-server) and the
     * legacy ST-LINK/V1 backend (sg_legacy.c). Applications do not call these
     * functions directly but the stlink_* functions, which add logging, checks
     * and generic sequences on top of them.
     *
     * Unless documented otherwise, the functions return 0 on success and -1 on
     * error. Data is transferred through stlink_t::q_buf. Members documented as
     * optional may be NULL. Callers must check that: the stlink_* functions
     * currently do so only for target_voltage and init_ap.
     *
     * @ingroup api_backend
     */
    typedef struct _stlink_backend {
        /** Release the device and the private data of the backend (stlink_t::backend_data), see stlink_close() */
        void (*close) (stlink_t * sl);
        /** Leave the debug mode, see stlink_exit_debug_mode() */
        int32_t (*exit_debug_mode) (stlink_t * sl);
        /** Enter the SWD debug mode, see stlink_enter_swd_mode() */
        int32_t (*enter_swd_mode) (stlink_t * sl);
        /** Enter the JTAG debug mode (optional, NULL in the libusb backend; only called by stlink_remote_serve()) */
        int32_t (*enter_jtag_mode) (stlink_t * stl);
        /** Leave the DFU mode of the ST-LINK, see stlink_exit_dfu_mode() */
        int32_t (*exit_dfu_mode) (stlink_t * stl);
        /** Read the debug port id into stlink_t::core_id, see stlink_core_id() */
        int32_t (*core_id) (stlink_t * stl);
        /** Send the system reset command of the ST-LINK, see stlink_reset() */
        int32_t (*reset) (stlink_t * stl);
        /** Drive the NRST pin: STLINK_DEBUG_APIV2_DRIVE_NRST_LOW or STLINK_DEBUG_APIV2_DRIVE_NRST_HIGH */
        int32_t (*jtag_reset) (stlink_t * stl, int32_t value);
        /** Resume the core, see stlink_run() */
        int32_t (*run) (stlink_t * stl, enum run_type type);
        /** Read the core state into stlink_t::core_stat, see stlink_status() */
        int32_t (*status) (stlink_t * stl);
        /** Read the raw version reply of the ST-LINK into stlink_t::q_buf, decoded by stlink_version() */
        int32_t (*version) (stlink_t *sl);
        /** Read a 32 bit word from the target, see stlink_read_debug32() */
        int32_t (*read_debug32) (stlink_t *sl, uint32_t addr, uint32_t *data);
        /** Read len bytes (multiple of 4) into stlink_t::q_buf, see stlink_read_mem32() */
        int32_t (*read_mem32) (stlink_t *sl, uint32_t addr, uint16_t len);
        /** Write a 32 bit word to the target, see stlink_write_debug32() */
        int32_t (*write_debug32) (stlink_t *sl, uint32_t addr, uint32_t data);
        /** Write len bytes (multiple of 4) from stlink_t::q_buf, see stlink_write_mem32() */
        int32_t (*write_mem32) (stlink_t *sl, uint32_t addr, uint16_t len);
        /** Write len bytes from stlink_t::q_buf with byte accesses, see stlink_write_mem8() */
        int32_t (*write_mem8) (stlink_t *sl, uint32_t addr, uint16_t len);
        /** Read all core registers, see stlink_read_all_regs() */
        int32_t (*read_all_regs) (stlink_t *sl, struct stlink_reg * regp);
        /** Read one core register (index 0..20), see stlink_read_reg() */
        int32_t (*read_reg) (stlink_t *sl, int32_t r_idx, struct stlink_reg * regp);
        /** Read the special and FPU registers via DCRSR/DCRDR (optional), see stlink_read_all_unsupported_regs() */
        int32_t (*read_all_unsupported_regs) (stlink_t *sl, struct stlink_reg *regp);
        /** Read a special or FPU register, r_idx is the DCRSR register selector (optional), see stlink_read_unsupported_reg() */
        int32_t (*read_unsupported_reg) (stlink_t *sl, int32_t r_idx, struct stlink_reg *regp);
        /** Write a special or FPU register (optional), see stlink_write_unsupported_reg() */
        int32_t (*write_unsupported_reg) (stlink_t *sl, uint32_t value, int32_t idx, struct stlink_reg *regp);
        /** Write one core register (index 0..20), see stlink_write_reg() */
        int32_t (*write_reg) (stlink_t *sl, uint32_t reg, int32_t idx);
        /** Execute one instruction, see stlink_step() */
        int32_t (*step) (stlink_t * stl);
        /** Return the mode of the ST-LINK (STLINK_DEV_*_MODE), see stlink_current_mode() */
        int32_t (*current_mode) (stlink_t * stl);
        /** Halt the core, see stlink_force_debug() */
        int32_t (*force_debug) (stlink_t *sl);
        /** Return the target voltage in mV, or -1 (optional), see stlink_target_voltage() */
        int32_t (*target_voltage) (stlink_t *sl);
        /** Set the SWD clock in kHz (optional), see stlink_set_swdclk() */
        int32_t (*set_swdclk) (stlink_t * stl, int32_t freq_khz);
        /** Start capturing SWO trace data at the given trace frequency in Hz (optional) */
        int32_t (*trace_enable) (stlink_t * sl, uint32_t frequency);
        /** Stop capturing SWO trace data (optional) */
        int32_t (*trace_disable) (stlink_t * sl);
        /** Read the captured SWO trace data into buf (size bytes available); returns the number of bytes read, or -1 (optional) */
        int32_t (*trace_read) (stlink_t * sl, uint8_t* buf, uint32_t size);
        /** Select and initialise an access port (optional, NULL if unsupported) */
        int32_t (*init_ap) (stlink_t * sl, uint8_t ap);
    } stlink_backend_t;

#endif // STLINK_BACKEND_H
