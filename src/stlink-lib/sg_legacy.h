/**
  ******************************************************************************
  * @file             sg_legacy.h
  * @brief            Legacy ST-LINK/V1 backend (SCSI commands over USB mass storage)
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-10-08
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */


// TODO: CONTENT AND USE OF THIS SOURCE FILE IS TO BE VERIFIED
// This file should be split up into new or existing modules

#ifndef SG_LEGACY_H
#define SG_LEGACY_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <stlink.h>
#include <stlink_backend.h>
#include <stlink_cmd.h>
#include <stm32_register.h>

#include "libusb_settings.h"


/** @cond STLINK_INTERNAL */

/* Device access */
#define RDWR        0
#define RO      1
#define SG_TIMEOUT_SEC  1 // actually 1 is about 2 sec
#define SG_TIMEOUT_MSEC 3 * 1000

// Each CDB can be a total of 6, 10, 12, or 16 bytes, later version of the SCSI standard
// also allow for variable-length CDBs (min. CDB is 6). The stlink needs max. 10 bytes.
#define CDB_6       6
#define CDB_10      10
#define CDB_12      12
#define CDB_16      16

#define CDB_SL      10

/* Query data flow direction */
#define Q_DATA_OUT  0
#define Q_DATA_IN   1

// The SCSI Request Sense command is used to obtain sense data (error information) from
// a target device. (http://en.wikipedia.org/wiki/SCSI_Request_Sense_Command)
#define SENSE_BUF_LEN       32

/** Private data of the legacy ST-LINK/V1 backend (stlink_t::backend_data) */
struct stlink_libsg {
    libusb_context* libusb_ctx;
    libusb_device_handle *usb_handle;
    uint32_t ep_rep;
    uint32_t ep_req;

    int32_t sg_fd;
    int32_t do_scsi_pt_err;

    unsigned char cdb_cmd_blk[CDB_SL];

    int32_t q_data_dir; // Q_DATA_IN, Q_DATA_OUT
    // the start of the query data in the device memory space
    uint32_t q_addr;

    // Sense (error information) data
    // obsolete, this was fed to the scsi tools
    unsigned char sense_buf[SENSE_BUF_LEN];

    struct stlink_reg reg;
};

/**
 * @name Implementation of the legacy ST-LINK/V1 backend
 * The functions behind stlink_backend_t for the legacy SCSI/USB mass storage
 * backend, see there for their contract, plus helpers for the mass storage
 * transport. Exported only because they are not static.
 */
/** @{ */
// static void clear_cdb(struct stlink_libsg *sl);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::close. */
void _stlink_sg_close(stlink_t *sl);
// static int32_t get_usb_mass_storage_status(libusb_device_handle *handle, uint8_t endpoint, uint32_t *tag);
// static int32_t dump_CDB_command(uint8_t *cdb, uint8_t cdb_len);
/** Send a SCSI command block wrapped in a USB mass storage command block wrapper; returns the tag, or -1. */
int32_t send_usb_mass_storage_command(libusb_device_handle *handle, uint8_t endpoint_out, uint8_t *cdb, uint8_t cdb_length,
                                        uint8_t lun, uint8_t flags, uint32_t expected_rx_size);
// static void get_sense(libusb_device_handle *handle, uint8_t endpoint_in, uint8_t endpoint_out);
/** Send data on the OUT endpoint and read the mass storage status; returns the number of bytes sent, or -1. */
int32_t send_usb_data_only(libusb_device_handle *handle, unsigned char endpoint_out,
                       unsigned char endpoint_in, unsigned char *cbuf, uint32_t length);
/** Execute the SCSI command in the command block and read the reply into stlink_t::q_buf. */
int32_t stlink_q(stlink_t *sl);
/** Log the status byte of the last reply at debug level. */
void stlink_stat(stlink_t *stl, char *txt);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::version. */
int32_t _stlink_sg_version(stlink_t *stl);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::current_mode. */
int32_t _stlink_sg_current_mode(stlink_t *stl);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::enter_swd_mode. */
int32_t _stlink_sg_enter_swd_mode(stlink_t *sl);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::enter_jtag_mode. */
int32_t _stlink_sg_enter_jtag_mode(stlink_t *sl);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::exit_dfu_mode. */
int32_t _stlink_sg_exit_dfu_mode(stlink_t *sl);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::core_id. */
int32_t _stlink_sg_core_id(stlink_t *sl);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::reset. */
int32_t _stlink_sg_reset(stlink_t *sl);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::jtag_reset. */
int32_t _stlink_sg_jtag_reset(stlink_t *sl, int32_t value);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::status. */
int32_t _stlink_sg_status(stlink_t *sl);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::force_debug. */
int32_t _stlink_sg_force_debug(stlink_t *sl);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::read_all_regs. */
int32_t _stlink_sg_read_all_regs(stlink_t *sl, struct stlink_reg *regp);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::read_reg. */
int32_t _stlink_sg_read_reg(stlink_t *sl, int32_t r_idx, struct stlink_reg *regp);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::write_reg. */
int32_t _stlink_sg_write_reg(stlink_t *sl, uint32_t reg, int32_t idx);
/** Write a debug register with the API V1 command (unused). */
void stlink_write_dreg(stlink_t *sl, uint32_t reg, uint32_t addr);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::run. */
int32_t _stlink_sg_run(stlink_t *sl, enum run_type type);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::step. */
int32_t _stlink_sg_step(stlink_t *sl);
/** Set a flash patch breakpoint (unused; the arguments end up in q_buf instead of the command block). */
void stlink_set_hw_bp(stlink_t *sl, int32_t fp_nr, uint32_t addr, int32_t fp);
/** Clear a flash patch breakpoint (unused). */
void stlink_clr_hw_bp(stlink_t *sl, int32_t fp_nr);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::read_mem32. */
int32_t _stlink_sg_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::write_mem8. */
int32_t _stlink_sg_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::write_mem32. */
int32_t _stlink_sg_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::write_debug32. */
int32_t _stlink_sg_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::read_debug32. */
int32_t _stlink_sg_read_debug32(stlink_t *sl, uint32_t addr, uint32_t *data);
/** Legacy ST-LINK/V1 backend: implementation of stlink_backend_t::exit_debug_mode. */
int32_t _stlink_sg_exit_debug_mode(stlink_t *stl);
/** @} */

// static stlink_backend_t _stlink_sg_backend = { };

// static stlink_t* stlink_open(const int32_t verbose);

/**
 * Open the first ST-LINK/V1 with the legacy backend and bring it into a usable mode.
 * Does not enter the debug mode, see stlink_v1_open().
 * @ingroup api_internal
 * @param verbose log level, see stlink_open_usb()
 * @return        the device handle, or NULL on error
 */
stlink_t* stlink_v1_open_inner(const int32_t verbose);
/** @endcond */

/**
 * Open the first ST-LINK/V1 with the legacy SCSI/USB mass storage backend.
 *
 * Enters SWD mode, optionally resets the target (RESET_AUTO) and loads the
 * target parameters. Only used by the hardware test tests/sg_legacy.c:
 * stlink_open_usb() supports the ST-LINK/V1 as well.
 *
 * @ingroup api_legacy
 * @param verbose log level, see stlink_open_usb(); also stored in stlink_t::verbose
 * @param reset   non-zero to reset the target before loading its parameters
 * @return        the device handle, to be released with stlink_close(), or NULL if no
 *                ST-LINK/V1 was found or it could not be opened
 */
stlink_t* stlink_v1_open(const int32_t verbose, int32_t reset);

#endif // SG_LEGACY_H
