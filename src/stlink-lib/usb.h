/**
  ******************************************************************************
  * @file             usb.h
  * @brief            USB commands & interaction with ST-LINK devices
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef USB_H
#define USB_H

#if !defined(_MSC_VER)
#include <sys/time.h>
#endif // _MSC_VER

#if defined(_WIN32)
#include <windows.h>
#endif // _WIN32

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>

#include <stlink.h>
#include <stlink_backend.h>
#include <stlink_cmd.h>
#include <stm32_register.h>

#include "libusb_settings.h"
#include "logging.h"


/** @cond STLINK_INTERNAL */

/** @name USB vendor and product ids of the ST-LINK versions */
/** @{ */
#define STLINK_USB_VID_ST                   0x0483
#define STLINK_USB_PID_STLINK               0x3744
#define STLINK_USB_PID_STLINK_32L           0x3748
#define STLINK_USB_PID_STLINK_32L_AUDIO     0x374a
#define STLINK_USB_PID_STLINK_NUCLEO        0x374b
#define STLINK_USB_PID_STLINK_V2_1          0x3752
#define STLINK_USB_PID_STLINK_V3_USBLOADER  0x374d
#define STLINK_USB_PID_STLINK_V3E_PID       0x374e
#define STLINK_USB_PID_STLINK_V3S_PID       0x374f
#define STLINK_USB_PID_STLINK_V3_2VCP_PID   0x3753
#define STLINK_USB_PID_STLINK_V3_NO_MSD_PID 0x3754
#define STLINK_USB_PID_STLINK_V3P           0x3757

#define STLINK_V1_USB_PID(pid) ((pid) == STLINK_USB_PID_STLINK)

#define STLINK_V2_USB_PID(pid) ((pid) == STLINK_USB_PID_STLINK_32L || \
                                (pid) == STLINK_USB_PID_STLINK_32L_AUDIO || \
                                (pid) == STLINK_USB_PID_STLINK_NUCLEO)

#define STLINK_V2_1_USB_PID(pid) ((pid) == STLINK_USB_PID_STLINK_V2_1)

#define STLINK_V3_USB_PID(pid) ((pid) == STLINK_USB_PID_STLINK_V3_USBLOADER || \
                                (pid) == STLINK_USB_PID_STLINK_V3E_PID || \
                                (pid) == STLINK_USB_PID_STLINK_V3S_PID || \
                                (pid) == STLINK_USB_PID_STLINK_V3_2VCP_PID || \
                                (pid) == STLINK_USB_PID_STLINK_V3_NO_MSD_PID || \
                                (pid) == STLINK_USB_PID_STLINK_V3P)

#define STLINK_SUPPORTED_USB_PID(pid) (STLINK_V1_USB_PID(pid) || \
                                       STLINK_V2_USB_PID(pid) || \
                                       STLINK_V2_1_USB_PID(pid) || \
                                       STLINK_V3_USB_PID(pid))

#define STLINK_SG_SIZE 31
#define STLINK_CMD_SIZE 16

enum SCSI_Generic_Direction {SG_DXFER_TO_DEV = 0, SG_DXFER_FROM_DEV = 0x80};
/** @} */

/** Private data of the libusb backend (stlink_t::backend_data) */
struct stlink_libusb {
    libusb_context* libusb_ctx;
    libusb_device_handle* usb_handle;
    uint32_t ep_req;
    uint32_t ep_rep;
    uint32_t ep_trace;
    int32_t protocol;
    uint32_t sg_transfer_idx;
    uint32_t cmd_len;
};

/**
 * @name Implementation of the libusb backend
 * The functions behind stlink_backend_t for the libusb backend, see there for
 * their contract. Exported only because they are not static; use the stlink_*
 * functions instead.
 */
/** @{ */
// static inline uint32_t le_to_h_u32(const uint8_t* buf);
// static int32_t _stlink_match_speed_map(const uint32_t *map, uint32_t map_size, uint32_t khz);
/** libusb backend: implementation of stlink_backend_t::close. */
void _stlink_usb_close(stlink_t* sl);
/** Send a command to the ST-LINK and receive the reply (transport of the libusb backend). */
ssize_t send_recv(struct stlink_libusb* handle, int32_t terminate, unsigned char* txbuf, uint32_t txsize,
                    unsigned char* rxbuf, uint32_t rxsize, int32_t check_error, const char *cmd);
// static inline int32_t send_only(struct stlink_libusb* handle, int32_t terminate, unsigned char* txbuf,
//                                  uint32_t txsize, const char *cmd);
// static int32_t fill_command(stlink_t * sl, enum SCSI_Generic_Direction dir, uint32_t len);
/** libusb backend: implementation of stlink_backend_t::version. */
int32_t _stlink_usb_version(stlink_t *sl);
/** libusb backend: implementation of stlink_backend_t::target_voltage. */
int32_t _stlink_usb_target_voltage(stlink_t *sl);
/** libusb backend: implementation of stlink_backend_t::read_debug32. */
int32_t _stlink_usb_read_debug32(stlink_t *sl, uint32_t addr, uint32_t *data);
/** libusb backend: implementation of stlink_backend_t::write_debug32. */
int32_t _stlink_usb_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data);
/** Read the status of the last memory read/write command (libusb backend helper). */
int32_t _stlink_usb_get_rw_status(stlink_t *sl);
/** libusb backend: implementation of stlink_backend_t::write_mem32. */
int32_t _stlink_usb_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
/** libusb backend: implementation of stlink_backend_t::write_mem8. */
int32_t _stlink_usb_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len);
/** libusb backend: implementation of stlink_backend_t::current_mode. */
int32_t _stlink_usb_current_mode(stlink_t * sl);
/** libusb backend: implementation of stlink_backend_t::core_id. */
int32_t _stlink_usb_core_id(stlink_t * sl);
/** Read the core state from DHCSR (debug API V2/V3), used by the status operation of the libusb backend. */
int32_t _stlink_usb_status_v2(stlink_t *sl);
/** libusb backend: implementation of stlink_backend_t::status. */
int32_t _stlink_usb_status(stlink_t * sl);
/** libusb backend: implementation of stlink_backend_t::force_debug. */
int32_t _stlink_usb_force_debug(stlink_t *sl);
/** libusb backend: implementation of stlink_backend_t::enter_swd_mode. */
int32_t _stlink_usb_enter_swd_mode(stlink_t * sl);
/** libusb backend: implementation of stlink_backend_t::init_ap. */
int32_t _stlink_usb_init_ap(stlink_t * sl, uint8_t ap);
/** libusb backend: implementation of stlink_backend_t::exit_dfu_mode. */
int32_t _stlink_usb_exit_dfu_mode(stlink_t* sl);
/** libusb backend: implementation of stlink_backend_t::reset. */
int32_t _stlink_usb_reset(stlink_t * sl);
/** libusb backend: implementation of stlink_backend_t::jtag_reset. */
int32_t _stlink_usb_jtag_reset(stlink_t * sl, int32_t value);
/** libusb backend: implementation of stlink_backend_t::step. */
int32_t _stlink_usb_step(stlink_t* sl);
/** libusb backend: implementation of stlink_backend_t::run. */
int32_t _stlink_usb_run(stlink_t* sl, enum run_type type);
/** libusb backend: implementation of stlink_backend_t::set_swdclk. */
int32_t _stlink_usb_set_swdclk(stlink_t* sl, int32_t clk_freq);
/** libusb backend: implementation of stlink_backend_t::exit_debug_mode. */
int32_t _stlink_usb_exit_debug_mode(stlink_t *sl);
/** libusb backend: implementation of stlink_backend_t::read_mem32. */
int32_t _stlink_usb_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
/** libusb backend: implementation of stlink_backend_t::read_all_regs. */
int32_t _stlink_usb_read_all_regs(stlink_t *sl, struct stlink_reg *regp);
/** libusb backend: implementation of stlink_backend_t::read_reg. */
int32_t _stlink_usb_read_reg(stlink_t *sl, int32_t r_idx, struct stlink_reg *regp);
/** libusb backend: implementation of stlink_backend_t::read_unsupported_reg. */
int32_t _stlink_usb_read_unsupported_reg(stlink_t *sl, int32_t r_idx, struct stlink_reg *regp);
/** libusb backend: implementation of stlink_backend_t::read_all_unsupported_regs. */
int32_t _stlink_usb_read_all_unsupported_regs(stlink_t *sl, struct stlink_reg *regp);
/** libusb backend: implementation of stlink_backend_t::write_unsupported_reg. */
int32_t _stlink_usb_write_unsupported_reg(stlink_t *sl, uint32_t val, int32_t r_idx, struct stlink_reg *regp);
/** libusb backend: implementation of stlink_backend_t::write_reg. */
int32_t _stlink_usb_write_reg(stlink_t *sl, uint32_t reg, int32_t idx);
/** libusb backend: implementation of stlink_backend_t::trace_enable. */
int32_t _stlink_usb_enable_trace(stlink_t* sl, uint32_t frequency);
/** libusb backend: implementation of stlink_backend_t::trace_disable. */
int32_t _stlink_usb_disable_trace(stlink_t* sl);
/** libusb backend: implementation of stlink_backend_t::trace_read. */
int32_t _stlink_usb_read_trace(stlink_t* sl, uint8_t* buf, uint32_t size);

/** @} */

// static stlink_backend_t _stlink_usb_backend = { };

/**
 * Read the serial number of an ST-LINK as hex string.
 *
 * Repairs the 12 byte binary serial reported by some ST-LINK firmware versions.
 *
 * @ingroup api_internal
 * @param handle open libusb device
 * @param desc   its device descriptor
 * @param serial receives the serial, at least STLINK_SERIAL_BUFFER_SIZE bytes; empty on error
 * @return the length of the serial (STLINK_SERIAL_LENGTH for a valid serial), 0 on error
 */
uint32_t stlink_serial(struct libusb_device_handle *handle, struct libusb_device_descriptor *desc, char *serial);

/** @endcond */

/**
 * @addtogroup api_device
 * @{
 */

/**
 * Open an ST-LINK connected via USB and connect to its target.
 *
 * Opens the ST-LINK with the given serial number, or the first one found. Reads
 * its version, leaves the DFU mode if necessary, sets the SWD frequency and
 * connects to the target with stlink_target_connect(), which identifies the
 * target and fills the target description in the handle (flash, SRAM, option
 * bytes). ST-LINK/V1, V2, V2-1 and V3 are supported.
 *
 * Sets the log level to @p verbose, unless the application has set one with
 * stlink_log_set_level(). Call init_chipids() before, otherwise the target
 * cannot be identified.
 *
 * @note A handle is also returned if the target could not be identified (e.g.
 *       no target connected, unknown chip). stlink_t::flash_size is 0 then.
 *
 * @param verbose log level, see stlink_log_set_level()
 * @param connect how to connect to the target, see stlink_target_connect()
 * @param serial  serial number of the ST-LINK to open (hex string, as
 *                stlink_t::serial), or NULL or "" for the first ST-LINK found
 * @param freq    SWD frequency in kHz, 0 for the default (see stlink_set_swdclk())
 * @return the device handle, to be released with stlink_close(), or NULL if no
 *         matching ST-LINK was found or it could not be opened (e.g. in use)
 */
stlink_t *stlink_open_usb(enum ugly_loglevel verbose, enum connect_type connect, char serial[STLINK_SERIAL_BUFFER_SIZE], int32_t freq);
// static uint32_t stlink_probe_usb_devs(libusb_device **devs, stlink_t **sldevs[], enum connect_type connect, int32_t freq);

/**
 * Open all ST-LINKs connected via USB and connect to their targets.
 *
 * Every supported ST-LINK is opened as with stlink_open_usb() (in parallel
 * threads), devices that cannot be opened are skipped.
 *
 * @param stdevs  receives an array of device handles; release it with
 *                stlink_probe_usb_free(). Set to NULL if out of memory, unchanged
 *                if libusb fails: initialise it to NULL before the call.
 * @param connect how to connect to the targets, see stlink_target_connect()
 * @param freq    SWD frequency in kHz, 0 for the default
 * @return the number of handles in @p stdevs
 */
uint32_t stlink_probe_usb(stlink_t **stdevs[], enum connect_type connect, int32_t freq);

/**
 * Close all devices returned by stlink_probe_usb() and free the array.
 * @bug With @p size 0 the array is not freed, so it leaks when no device was found.
 * @param stdevs the array from stlink_probe_usb(), set to NULL
 * @param size   number of handles in the array
 */
void stlink_probe_usb_free(stlink_t **stdevs[], uint32_t size);

/** @} */

#endif // USB_H
