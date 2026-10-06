/**
  ******************************************************************************
  * @file             stlink.h
  * @brief            Common top level stlink interfaces
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef STLINK_H
#define STLINK_H

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#include <stm32.h>
#include <stm32_flash.h>

#include <version.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Number of elements of a statically sized array (internal helper) */
#define STLINK_ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

/**
 * Size of the data buffer stlink_t::q_buf.
 * Bounds the length of a single stlink_read_mem32(), stlink_write_mem32() or
 * stlink_write_mem8() transfer.
 */
// 6 kB = max mem32_read block, 8 kB sram
// #define Q_BUF_LEN    96
#define Q_BUF_LEN (1024 * 100)

/** State of the target core, as last read by stlink_status() (stlink_t::core_stat) */
enum target_state {
    TARGET_UNKNOWN = 0,         ///< not read yet, or the read failed
    TARGET_RUNNING = 1,         ///< core is running
    TARGET_HALTED = 2,          ///< core is halted (debug state)
    TARGET_RESET = 3,           ///< core is held in or just left reset
    TARGET_DEBUG_RUNNING = 4,   ///< running under debugger control (not reported by the current backends)
};

/** @cond STLINK_INTERNAL */
/* Core status codes of the ST-LINK protocol */
#define STLINK_CORE_RUNNING                0x80
#define STLINK_CORE_HALTED                 0x81
/** @endcond */

/** @name ST-LINK adapter modes, as returned by stlink_current_mode() */
/** @{ */
#define STLINK_DEV_DFU_MODE                   0     ///< DFU (firmware update) mode
#define STLINK_DEV_MASS_MODE                  1     ///< USB mass storage mode (idle)
#define STLINK_DEV_DEBUG_MODE                 2     ///< debug mode (SWD or JTAG)
#define STLINK_DEV_UNKNOWN_MODE              -1     ///< mode could not be determined
/** @} */

/** @name NRST pin states for stlink_backend_t::jtag_reset */
/** @{ */
#define STLINK_DEBUG_APIV2_DRIVE_NRST_LOW  0x00     ///< drive NRST low (assert reset)
#define STLINK_DEBUG_APIV2_DRIVE_NRST_HIGH 0x01     ///< release NRST
/** @} */

/** @cond STLINK_INTERNAL */
/* Baud rate divisors for SWDCLK (ST-LINK/V2) */
#define STLINK_SWDCLK_4MHZ_DIVISOR            0
#define STLINK_SWDCLK_1P8MHZ_DIVISOR          1
#define STLINK_SWDCLK_1P2MHZ_DIVISOR          2
#define STLINK_SWDCLK_950KHZ_DIVISOR          3
#define STLINK_SWDCLK_480KHZ_DIVISOR          7
#define STLINK_SWDCLK_240KHZ_DIVISOR         15
#define STLINK_SWDCLK_125KHZ_DIVISOR         31
#define STLINK_SWDCLK_100KHZ_DIVISOR         40
#define STLINK_SWDCLK_50KHZ_DIVISOR          79
#define STLINK_SWDCLK_25KHZ_DIVISOR         158
#define STLINK_SWDCLK_15KHZ_DIVISOR         265
#define STLINK_SWDCLK_5KHZ_DIVISOR          798
/** @endcond */

/** @name Serial number of the ST-LINK adapter (hex string, see stlink_t::serial) */
/** @{ */
#define STLINK_SERIAL_LENGTH                 24     ///< characters of the serial number
#define STLINK_SERIAL_BUFFER_SIZE   (STLINK_SERIAL_LENGTH + 1)  ///< buffer size including the terminating NUL
/** @} */

/** Maximum number of SWD frequencies an ST-LINK/V3 reports (internal) */
#define STLINK_V3_MAX_FREQ_NB                10

/** @name SWO trace buffer sizes and frequencies */
/** @{ */
#define STLINK_V2_TRACE_BUF_LEN            2048     ///< trace buffer of the ST-LINK/V2 in bytes
#define STLINK_V3_TRACE_BUF_LEN            8192     ///< trace buffer of the ST-LINK/V3 in bytes
#define STLINK_V2_MAX_TRACE_FREQUENCY   2000000     ///< maximum SWO frequency of the ST-LINK/V2 in Hz
#define STLINK_V3_MAX_TRACE_FREQUENCY  24000000     ///< maximum SWO frequency of the ST-LINK/V3 in Hz
#define STLINK_DEFAULT_TRACE_FREQUENCY  2000000     ///< default SWO frequency in Hz
/** @} */

/**
 * @name Features of the ST-LINK firmware (stlink_version_t::flags)
 * Map the relevant features, quirks and workaround for specific firmware version of stlink.
 * Set by stlink_version(); currently only STLINK_F_HAS_TRACE,
 * STLINK_F_HAS_GETLASTRWSTATUS2 and STLINK_F_HAS_CSW are detected.
 */
/** @{ */
#define STLINK_F_HAS_TRACE              (1U << 0)   ///< SWO trace capture
#define STLINK_F_HAS_SWD_SET_FREQ       (1U << 1)   ///< SWD frequency can be set (not detected)
#define STLINK_F_HAS_JTAG_SET_FREQ      (1U << 2)   ///< JTAG frequency can be set (not detected)
#define STLINK_F_HAS_MEM_16BIT          (1U << 3)   ///< 16 bit memory accesses (not detected)
#define STLINK_F_HAS_GETLASTRWSTATUS2   (1U << 4)   ///< extended status of the last read/write command
#define STLINK_F_HAS_DAP_REG            (1U << 5)   ///< DAP register access (not detected)
#define STLINK_F_QUIRK_JTAG_DP_READ     (1U << 6)   ///< quirk of DP reads in JTAG mode (not detected)
#define STLINK_F_HAS_AP_INIT            (1U << 7)   ///< access port initialisation (not detected)
#define STLINK_F_HAS_DPBANKSEL          (1U << 8)   ///< DP bank selection
#define STLINK_F_HAS_RW8_512BYTES       (1U << 9)   ///< 8 bit transfers of up to 512 bytes (not detected)
// Memory read/write commands accept the MEM-AP CSW value (from V2J32 / V3J2),
// introduced together with DP bank selection
#define STLINK_F_HAS_CSW                STLINK_F_HAS_DPBANKSEL  ///< memory commands accept the MEM-AP CSW (V2J32 / V3J2 and later)
/** @} */

/**
 * MEM-AP CSW for secure memory accesses on Armv8-M targets (AHB5-AP), internal.
 * DbgSwEnable | MasterType = debug | SPROT = 0 (secure) | HPROT = privileged data access
 */
#define STLINK_CSW_SECURE               0xab000000

/** @name Additional MCU features (stlink_t::chip_flags, from the chip description file) */
/** @{ */
#define CHIP_F_HAS_DUAL_BANK            (1U << 0)   ///< flash has two banks
#define CHIP_F_HAS_SWO_TRACING          (1U << 1)   ///< SWO trace is supported
/** @} */

/** @cond STLINK_INTERNAL */
/* Error codes of the ST-LINK protocol */
#define STLINK_DEBUG_ERR_OK                 0x80
#define STLINK_DEBUG_ERR_FAULT              0x81
#define STLINK_DEBUG_ERR_WRITE              0x0c
#define STLINK_DEBUG_ERR_WRITE_VERIFY       0x0d
#define STLINK_DEBUG_ERR_AP_WAIT            0x10
#define STLINK_DEBUG_ERR_AP_FAULT           0x11
#define STLINK_DEBUG_ERR_AP_ERROR           0x12
#define STLINK_DEBUG_ERR_DP_WAIT            0x14
#define STLINK_DEBUG_ERR_DP_FAULT           0x15
#define STLINK_DEBUG_ERR_DP_ERROR           0x16

/* Reply checks of the USB backend */
#define CMD_CHECK_NO                           0
#define CMD_CHECK_REP_LEN                      1
#define CMD_CHECK_STATUS                       2
#define CMD_CHECK_RETRY                        3    // check status and retry if wait error
/** @endcond */

/** Size of the command buffer stlink_t::c_buf (internal) */
#define C_BUF_LEN                             32

/**
 * Core registers of the target.
 * Filled by stlink_read_all_regs(), stlink_read_reg(), stlink_read_unsupported_reg()
 * and stlink_read_all_unsupported_regs().
 */
struct stlink_reg {
    uint32_t r[16];         ///< R0..R15 (R13 = SP, R14 = LR, R15 = PC), register index 0..15
    uint32_t s[32];         ///< FPU registers S0..S31, register index 0x20..0x3f of stlink_read_unsupported_reg()
    uint32_t xpsr;          ///< xPSR, register index 16
    uint32_t main_sp;       ///< MSP, register index 17
    uint32_t process_sp;    ///< PSP, register index 18
    uint32_t rw;            ///< register index 19 (meaning depends on the ST-LINK firmware)
    uint32_t rw2;           ///< register index 20 (meaning depends on the ST-LINK firmware)
    uint8_t control;        ///< CONTROL, register index 0x1c of stlink_read_unsupported_reg()
    uint8_t faultmask;      ///< FAULTMASK, register index 0x1d
    uint8_t basepri;        ///< BASEPRI, register index 0x1e
    uint8_t primask;        ///< PRIMASK, register index 0x1f
    uint32_t fpscr;         ///< FPSCR, register index 0x40
};

/** Address in the memory space of the target */
typedef uint32_t stm32_addr_t;

/**
 * State of a flash loader running in the SRAM of the target.
 * Filled by stlink_flashloader_start(), used by stlink_flashloader_write() and
 * stlink_flashloader_stop(). The caller only provides the storage.
 */
typedef struct flash_loader {
    stm32_addr_t loader_addr;       ///< loader sram addr
    stm32_addr_t buf_addr;          ///< buffer sram address
    uint32_t rcc_dma_bkp;           ///< backup RCC DMA enable state
    uint32_t iwdg_kr;               ///< IWDG key register address
} flash_loader_t;

/** Decoded CPUID register of the Cortex-M core, see stlink_cpu_id() */
typedef struct _cortex_m3_cpuid_ {
    uint16_t implementer_id;    ///< implementer code, 0x41 = ARM
    uint16_t variant;           ///< variant (major revision)
    uint16_t part;              ///< part number, e.g. 0xc24 = Cortex-M4
    uint8_t revision;           ///< revision (minor revision)
} cortex_m3_cpuid_t;

/** Version of the debug API of the ST-LINK firmware */
enum stlink_jtag_api_version {
    STLINK_JTAG_API_V1 = 1,     ///< ST-LINK/V1 up to firmware J11
    STLINK_JTAG_API_V2,         ///< ST-LINK/V1 from firmware J12, ST-LINK/V2 and V2-1
    STLINK_JTAG_API_V3,         ///< ST-LINK/V3
};

/** Version and features of the ST-LINK adapter, read by stlink_version() (stlink_t::version) */
typedef struct stlink_version_ {
    uint32_t stlink_v;          ///< hardware generation: 1, 2 or 3
    uint32_t jtag_v;            ///< firmware version of the JTAG/SWD part (the "J" in V2J45)
    uint32_t swim_v;            ///< firmware version of the SWIM part
    uint32_t st_vid;            ///< USB vendor id
    uint32_t stlink_pid;        ///< USB product id
    // jtag api version supported
    enum stlink_jtag_api_version jtag_api;  ///< debug API supported by the firmware
    // one bit for each feature supported. See macros STLINK_F_*
    uint32_t flags;             ///< supported features, STLINK_F_* bits
} stlink_version_t;

/** Transport of a device (not used by the current backends) */
enum transport_type {
    TRANSPORT_TYPE_ZERO = 0,
    TRANSPORT_TYPE_LIBSG,
    TRANSPORT_TYPE_LIBUSB,
    TRANSPORT_TYPE_INVALID
};

/** How to connect to the target, see stlink_target_connect() */
enum connect_type {
    CONNECT_HOT_PLUG = 0,       ///< attach without halting or resetting the running target (exception: see stlink_load_device_params())
    CONNECT_NORMAL = 1,         ///< attach and reset the target (RESET_AUTO)
    CONNECT_UNDER_RESET = 2,    ///< hold NRST while attaching, then halt the core at the reset vector
};

/** Kind of reset, see stlink_reset() */
enum reset_type {
    RESET_AUTO = 0,             ///< NRST (ST-LINK/V2 and later) and the system reset command of the ST-LINK, software reset if no reset is detected
    RESET_HARD = 1,             ///< NRST (ST-LINK/V2 and later) and the system reset command of the ST-LINK
    RESET_SOFT = 2,             ///< software reset via AIRCR.SYSRESETREQ
    RESET_SOFT_AND_HALT = 3,    ///< software reset, core halted at the reset vector
};

/** How to run the core, see stlink_run() */
enum run_type {
    RUN_NORMAL = 0,             ///< run with interrupts enabled
    RUN_FLASH_LOADER = 1,       ///< run with interrupts masked (C_MASKINTS), used for the flash loader
};


/**
 * Handle of an open ST-LINK device and its target.
 * Created by stlink_open_usb(), stlink_open_remote() or stlink_probe_usb(),
 * released by stlink_close(). The structure is public for historical reasons,
 * see struct _stlink for the fields applications use today.
 */
typedef struct _stlink stlink_t;

/**
 * Progress events of longer operations (erase, flash write) and information about files
 * written to the target. Without a handler (see stlink_set_progress_handler()) the
 * progress is printed to stdout, as in previous versions.
 */
enum stlink_progress_event {
    STLINK_PROGRESS_FILE,               ///< file to be written: path, size, md5, checksum
    STLINK_PROGRESS_MASS_ERASE_START,   ///< mass erase started
    STLINK_PROGRESS_MASS_ERASE_TICK,    ///< mass erase still running (about once a second)
    STLINK_PROGRESS_MASS_ERASE_DONE,    ///< mass erase finished
    STLINK_PROGRESS_PAGE_ERASED,        ///< flash page or sector erased: addr, size
    STLINK_PROGRESS_ERASE_DONE,         ///< erase of a flash section finished
    STLINK_PROGRESS_WRITE,              ///< flash write progress: done of total in unit
    STLINK_PROGRESS_WRITE_DONE,         ///< flash write finished
};

/** Unit of stlink_progress::done and stlink_progress::total */
enum stlink_progress_unit {
    STLINK_PROGRESS_UNIT_PAGES,         ///< flash pages
    STLINK_PROGRESS_UNIT_HALFPAGES,     ///< half pages (STM32L0/L1)
    STLINK_PROGRESS_UNIT_BYTES,         ///< bytes
};

/** A progress event and its data, passed to a stlink_progress_handler_t */
struct stlink_progress {
    enum stlink_progress_event event;   ///< the event
    enum stlink_progress_unit unit;     ///< STLINK_PROGRESS_WRITE
    uint32_t done;                      ///< STLINK_PROGRESS_WRITE
    uint32_t total;                     ///< STLINK_PROGRESS_WRITE
    uint32_t addr;                      ///< STLINK_PROGRESS_PAGE_ERASED
    uint32_t size;                      ///< STLINK_PROGRESS_PAGE_ERASED, STLINK_PROGRESS_FILE (file size)
    const char *path;                   ///< STLINK_PROGRESS_FILE
    uint8_t md5[16];                    ///< STLINK_PROGRESS_FILE
    uint32_t checksum;                  ///< STLINK_PROGRESS_FILE: sum of all bytes, as shown by the ST tools
};

/**
 * Progress handler, see stlink_set_progress_handler()
 * @param sl       the device the progress belongs to
 * @param user     pointer given to stlink_set_progress_handler()
 * @param progress event and its data, only valid during the call
 */
typedef void (*stlink_progress_handler_t)(stlink_t *sl, void *user, const struct stlink_progress *progress);

#include "stm32.h"
#include "stlink_backend.h"

/**
 * Handle of an open ST-LINK device and its target (see stlink_t).
 *
 * Applications currently read the target description (flash_*, sram_*,
 * option_*, otp_*, chip_id, core_id, version, serial) directly from here, and
 * the memory functions transfer their data through q_buf. All other fields
 * are internal. Applications set verbose and opt after opening the device;
 * they must not modify the other fields.
 */
struct _stlink {
    struct _stlink_backend *backend;    ///< backend implementing the device access (internal)
    void *backend_data;                 ///< private data of the backend (internal)

    // room for the command header
    unsigned char c_buf[C_BUF_LEN];     ///< command buffer (internal)
    // data transferred from or to device
    unsigned char q_buf[Q_BUF_LEN];     ///< data buffer of stlink_read_mem32(), stlink_write_mem32() and stlink_write_mem8()
    int32_t q_len;                      ///< number of valid bytes in q_buf after a read (internal)

    // transport layer verboseness: 0 for no debug info, 10 for lots
    int32_t verbose;                    ///< verbosity of the debug output for this device (UDEBUG adds data dumps);
                                        ///< set by stlink_v1_open(), after stlink_open_usb() set by the application
    int32_t opt;                        ///< skip trailing erased bytes when writing flash (st-flash --opt)
    uint32_t core_id;               ///< set by stlink_core_id(), result from STLINK_DEBUGREADCOREID
    uint32_t chip_id;               ///< set by stlink_load_device_params(), used to identify flash and sram
    uint8_t ap;                     ///< set by stlink_probe_ap(), access port for debug/memory access (0 = AP0)
    enum target_state core_stat;    ///< set by stlink_status()

    char serial[STLINK_SERIAL_BUFFER_SIZE]; ///< serial number of the ST-LINK as hex string
    int32_t freq;                   ///< set by stlink_open_usb(), SWD frequency in kHz as requested (0 = default)

    enum stm32_flash_type flash_type;
    ///< stlink_chipid_params.flash_type, set by stlink_load_device_params(), values: STM32_FLASH_TYPE_xx

    stm32_addr_t flash_base;        ///< STM32_FLASH_BASE (STM32WB0: STM32WB0_FLASH_BASE), set by stlink_load_device_params(); the secure alias after stlink_flash_secure_enable()
    uint32_t flash_size;            ///< calculated by stlink_load_device_params()
    uint32_t flash_pgsz;            ///< stlink_chipid_params.flash_pagesize, set by stlink_load_device_params()

    /* sram settings */
    stm32_addr_t sram_base;         ///< STM32_SRAM_BASE, set by stlink_load_device_params()
    uint32_t sram_size;             ///< stlink_chipid_params.sram_size, set by stlink_load_device_params()

    /* option settings */
    stm32_addr_t option_base;       ///< address of the option bytes, set by stlink_load_device_params()
    uint32_t option_size;           ///< size of the option bytes, set by stlink_load_device_params()

    // bootloader
    // sys_base and sys_size are not used by the tools, but are only there to download the bootloader code
    // (see tests/sg_legacy.c)
    stm32_addr_t sys_base;          ///< stlink_chipid_params.bootrom_base, set by stlink_load_device_params()
    uint32_t sys_size;              ///< stlink_chipid_params.bootrom_size, set by stlink_load_device_params()

    struct stlink_version_ version; ///< version and features of the ST-LINK, set by stlink_version()

    uint32_t chip_flags;            ///< stlink_chipid_params.flags, set by stlink_load_device_params(), values: CHIP_F_xxx

    uint32_t max_trace_freq;        ///< set by stlink_open_usb()

    uint32_t otp_base;              ///< address of the OTP area, set by stlink_load_device_params()
    uint32_t otp_size;              ///< size of the OTP area, set by stlink_load_device_params()

    /* TrustZone settings, set by stlink_flash_secure_enable() */
    bool flash_secure;              ///< flash is programmed via the secure flash alias and the secure flash registers
    uint32_t secure_csw;            ///< MEM-AP CSW for accesses to secure alias addresses, 0 = ST-LINK default (non-secure)

    /* Progress reporting, set by stlink_set_progress_handler() */
    stlink_progress_handler_t progress_handler; ///< see stlink_set_progress_handler()
    void *progress_user;                        ///< see stlink_set_progress_handler()
};


/* === Declaration of functions defined in common_legacy.c === */

/**
 * @addtogroup api_device
 * @{
 */

/**
 * Close a device and release its handle.
 *
 * Releases the backend (USB device, remote connection) and frees @p sl. The
 * target is left in its current state; call stlink_exit_debug_mode() first to
 * detach the debugger cleanly.
 *
 * @param sl device handle, NULL is ignored. Invalid after the call.
 */
void stlink_close(stlink_t *sl);

/** @} */

/**
 * @addtogroup api_adapter
 * @{
 */

/**
 * Read the firmware version of the ST-LINK.
 *
 * Fills stlink_t::version, including the supported debug API and the feature
 * flags (STLINK_F_*), and stlink_t::max_trace_freq (ST-LINK/V2 from firmware
 * J13 and V3). Called by stlink_open_usb(), so applications rarely need it.
 *
 * @warning Do not call it on a handle from stlink_open_remote(): the version
 *          comes from the handshake there, and the remote backend returns no
 *          version data, so stlink_t::version would be decoded from stale data.
 *
 * @param sl device handle
 * @return 0 on success, -1 if the version could not be read
 */
int32_t stlink_version(stlink_t *sl);

/**
 * Query the mode of the ST-LINK.
 * @param sl device handle
 * @return STLINK_DEV_DFU_MODE, STLINK_DEV_MASS_MODE, STLINK_DEV_DEBUG_MODE, or
 *         STLINK_DEV_UNKNOWN_MODE if the mode is unknown or could not be read
 */
int32_t stlink_current_mode(stlink_t *sl);

/**
 * Switch the ST-LINK to SWD debug mode.
 * @param sl device handle
 * @return 0 on success, -1 on error
 */
int32_t stlink_enter_swd_mode(stlink_t *sl);

/**
 * Leave the debug mode and detach from the target.
 *
 * If the ST-LINK is in debug mode and the target has been identified, the debug
 * enable bit (DHCSR.C_DEBUGEN) is cleared and the vector catches armed while
 * connecting (DEMCR.VC_CORERESET, VC_HARDERR, VC_BUSERR) are disarmed, so the
 * target neither halts on its next reset nor on a fault once the debugger is gone.
 * A halted core resumes execution. Call this before stlink_close().
 *
 * @param sl device handle
 * @return 0 on success or if the ST-LINK was not in debug mode, -1 on error
 */
int32_t stlink_exit_debug_mode(stlink_t *sl);

/**
 * Leave the DFU mode of the ST-LINK.
 * Called by the open functions if the ST-LINK is found in DFU mode.
 * @param sl device handle
 * @return 0 on success, -1 on error
 */
int32_t stlink_exit_dfu_mode(stlink_t *sl);

/**
 * Set the SWD clock frequency.
 *
 * The frequency supported by the ST-LINK closest to @p freq_khz is selected.
 * Only supported by ST-LINK/V2 with firmware J22 or later and by ST-LINK/V3.
 * The open functions set the frequency given to them.
 *
 * @warning The legacy ST-LINK/V1 backend (stlink_v1_open()) does not implement
 *          it; the call then dereferences a NULL pointer.
 *
 * @param sl       device handle
 * @param freq_khz frequency in kHz, 0 selects the default (1800 kHz on V2, 1000 kHz on V3)
 * @return 0 on success, -1 on error or if the ST-LINK does not support it
 */
int32_t stlink_set_swdclk(stlink_t *sl, int32_t freq_khz);

/**
 * Read the target voltage measured by the ST-LINK.
 * @param sl device handle
 * @return the voltage in mV (0 if the ST-LINK reports an invalid reading), or -1
 *         on error or if the backend cannot measure it
 */
int32_t stlink_target_voltage(stlink_t *sl);

/** @} */

/**
 * @addtogroup api_target
 * @{
 */

/**
 * Connect to the target and identify it.
 *
 * Runs the connect sequence selected by @p connect, enters SWD mode and calls
 * stlink_load_device_params(). The open functions already call it; use it to
 * reconnect, e.g. after the target was power cycled.
 *
 * @param sl      device handle
 * @param connect CONNECT_HOT_PLUG: attach to the running target;
 *                CONNECT_NORMAL: attach and reset the target (RESET_AUTO);
 *                CONNECT_UNDER_RESET: hold NRST low while attaching, then halt
 *                the core before its first instruction
 * @return 0 on success, -1 if SWD mode could not be entered or the target could
 *         not be identified (see stlink_load_device_params())
 */
int32_t stlink_target_connect(stlink_t *sl, enum connect_type connect);

/**
 * Identify the target and load its parameters.
 *
 * Reads the core id, selects the access port of the CPU, reads the chip id and
 * looks it up in the chip database (see init_chipids()). Fills the target
 * description in @p sl: chip_id, flash_type, flash_base, flash_size,
 * flash_pgsz, sram_base, sram_size, option_base, option_size, otp_base,
 * otp_size, sys_base, sys_size and chip_flags.
 *
 * On targets whose CPU is on access port 1 (e.g. STM32H5) the core is reset and
 * left halted if the flash size cannot be read from the running target, also
 * when connecting with CONNECT_HOT_PLUG.
 *
 * @param sl device handle
 * @return 0 on success (also for a chip with unknown flash type, then
 *         stlink_t::flash_size is 0), -1 if the chip id could not be read or is
 *         not in the chip database
 */
int32_t stlink_load_device_params(stlink_t *sl);

/**
 * Read the id of the debug port (CPUTAPID) into stlink_t::core_id.
 * @param sl device handle
 * @return 0 on success, -1 on error
 */
int32_t stlink_core_id(stlink_t *sl);

/**
 * Read and decode the CPUID register of the Cortex-M core.
 * @param sl    device handle
 * @param cpuid receives the decoded register, all fields 0 on error
 * @return 0 on success, -1 on error
 */
int32_t stlink_cpu_id(stlink_t *sl, cortex_m3_cpuid_t *cpuid);

/**
 * Read the state of the core into stlink_t::core_stat.
 * @param sl device handle
 * @return 0 on success, -1 on error (with the USB backend stlink_t::core_stat is then TARGET_UNKNOWN)
 */
int32_t stlink_status(stlink_t *sl);

/**
 * Check whether the core is halted.
 * Reads the state with stlink_status().
 * @param sl device handle
 * @return true if the core is halted, false if it runs or the state could not be read
 */
bool stlink_is_core_halted(stlink_t *sl);

/**
 * Halt the core.
 *
 * Also freezes the watchdogs while the core is halted (DBGMCU), so the target
 * is not reset by them during a debug session: the independent and the window
 * watchdog on STM32F0, F1, F2, F3, F4, F7, G0, G4, L0, L1, L4, WB and WL, the
 * independent watchdog on STM32H7. On STM32WB0, which has no such freeze bit,
 * the watchdog is disabled instead. Other families are left unchanged.
 *
 * @param sl device handle
 * @return 0 on success, -1 on error
 */
int32_t stlink_force_debug(stlink_t *sl);

/**
 * Resume execution of the halted core.
 *
 * Sets the Thumb bit in xPSR first if it is cleared, as Cortex-M cores cannot
 * execute ARM code (an invalid vector table would otherwise lead to a fault).
 *
 * @param sl   device handle
 * @param type RUN_NORMAL, or RUN_FLASH_LOADER to keep interrupts masked
 * @return 0 on success, -1 on error
 */
int32_t stlink_run(stlink_t *sl, enum run_type type);

/**
 * Execute a single instruction on the halted core.
 * @param sl device handle
 * @return 0 on success, -1 on error
 */
int32_t stlink_step(stlink_t *sl);

/**
 * Reset the target.
 *
 * Sets stlink_t::core_stat to TARGET_RESET.
 *
 * @param sl   device handle
 * @param type RESET_AUTO: pulse NRST (ST-LINK/V2 and later) and send the system
 *             reset command of the ST-LINK; if no reset is detected (DHCSR.S_RESET_ST,
 *             e.g. NRST not connected) fall back to a software reset, otherwise
 *             wait up to 500 ms for the core to leave reset;
 *             RESET_HARD: pulse NRST (ST-LINK/V2 and later) and send the system
 *             reset command of the ST-LINK, without any check;
 *             RESET_SOFT: software reset via AIRCR.SYSRESETREQ;
 *             RESET_SOFT_AND_HALT: software reset with the core halted at the
 *             reset vector
 * @return 0 on success (always for RESET_HARD), -1 if the core is still in
 *         reset after 500 ms (RESET_AUTO) or the software reset failed
 */
int32_t stlink_reset(stlink_t *sl, enum reset_type type);

/**
 * Set the program counter, run the core and wait while it is halted.
 *
 * Used by the hardware test of the ST-LINK/V1 backend only (tests/sg_legacy.c).
 *
 * @param sl   device handle
 * @param addr new program counter
 */
void stlink_run_at(stlink_t *sl, stm32_addr_t addr);

/** @} */

/**
 * @addtogroup api_flash
 * @{
 */

/**
 * Size of the flash page or sector containing an address.
 *
 * For devices with sectors of different size (STM32F2, F4, F7) the size of the
 * sector at @p flashaddr is calculated and also stored in stlink_t::flash_pgsz;
 * for all others stlink_t::flash_pgsz is returned unchanged.
 *
 * @param sl        device handle
 * @param flashaddr address in flash
 * @return the page or sector size in bytes
 */
uint32_t stlink_calculate_pagesize(stlink_t *sl, uint32_t flashaddr);

/**
 * Value of an erased flash byte of the target.
 * @param sl device handle
 * @return 0x00 for STM32L0/L1, 0xff for all other devices
 */
uint8_t stlink_get_erased_pattern(stlink_t *sl);

/** @} */

/**
 * @addtogroup api_sram_file
 * @{
 */

/**
 * Read target memory into a file.
 *
 * Reads in chunks of at most one flash page (max. 6 KiB) with
 * stlink_read_mem32() and writes a raw binary file or an Intel HEX file. An
 * existing file is overwritten.
 *
 * @bug @p size is limited to stlink_t::flash_size, even if @p addr is not in
 *      flash, and the last chunk is written to the file rounded up to a
 *      multiple of 4 bytes, so the file can be up to 3 bytes longer than @p size.
 *
 * @param sl      device handle
 * @param path    file to write
 * @param is_ihex write Intel HEX instead of raw binary
 * @param addr    first address to read, should be word aligned
 * @param size    number of bytes, 0 reads the whole flash size
 * @return 0 on success, -1 on a file error (failed reads on the target are not detected)
 */
int32_t stlink_fread(stlink_t* sl, const char* path, bool is_ihex, stm32_addr_t addr, uint32_t size);

/**
 * Write a binary file into SRAM, verify it and start it.
 *
 * The file must fit into the SRAM of the target and @p addr must be word
 * aligned. After the verification the core is started at the reset vector of
 * the written image: the program counter is set to the word at @p addr + 4 and
 * the core runs (see stlink_mwrite_sram()).
 *
 * @param sl   device handle
 * @param path binary file to write
 * @param addr destination address in SRAM, word aligned
 * @return 0 on success, -1 on error (range, mapping, verification)
 */
int32_t stlink_fwrite_sram(stlink_t *sl, const char* path, stm32_addr_t addr);

/**
 * Write a buffer into SRAM and start it.
 *
 * Writes @p length bytes to @p addr, then sets the program counter to the word
 * at @p addr + 4 (the reset vector of a vector table at @p addr) and runs the
 * core. The written data is not verified, and transfer errors are not detected.
 *
 * @param sl     device handle
 * @param data   data to write
 * @param length number of bytes
 * @param addr   destination address in SRAM, word aligned
 * @return 0 on success, -1 if the range is not inside the SRAM or @p addr is unaligned
 */
int32_t stlink_mwrite_sram(stlink_t *sl, uint8_t* data, uint32_t length, stm32_addr_t addr);

/**
 * Load an Intel HEX file into a contiguous memory image.
 *
 * The image covers the lowest to the highest address of the data records; gaps
 * are filled with @p erased_pattern. Supported record types: data (00), end of
 * file (01), extended linear address (04) and start linear address (05, ignored).
 * Segment address records (02, 03) are rejected.
 *
 * @bug A file with a single data byte is rejected ("No data found").
 *
 * @param path           Intel HEX file
 * @param erased_pattern fill value for gaps, see stlink_get_erased_pattern()
 * @param mem            receives the image, allocated with malloc(); the caller
 *                       frees it. Unchanged on error.
 * @param size           receives the size of the image in bytes
 * @param begin          receives the address of the first byte of the image
 * @return 0 on success, -1 on error (file, format, checksum, memory)
 */
int32_t stlink_parse_ihex(const char* path, uint8_t erased_pattern, uint8_t* *mem, uint32_t* size, uint32_t* begin);

/** @} */

/** @cond STLINK_INTERNAL */
/**
 * @addtogroup api_internal
 * @{
 */

/**
 * Read the chip id (DBGMCU_IDCODE) of the target.
 * Called by stlink_load_device_params(), do not call it directly.
 * @param sl      device handle
 * @param chip_id receives the 12 bit device id; 0 on most errors (unchanged if the CPUID cannot be read)
 * @return 0 on success, non-zero on error
 */
int32_t stlink_chip_id(stlink_t *sl, uint32_t *chip_id);

/**
 * Log stlink_t::core_stat at debug level.
 * Called by stlink_status(), do not call it directly.
 * @param sl device handle
 */
void stlink_core_stat(stlink_t *sl);

/**
 * Log the data of the last transfer (stlink_t::q_buf) at debug level.
 * @param sl device handle
 */
void stlink_print_data(stlink_t *sl);

/**
 * Copy a data block into the buffer of a running flash loader.
 * Pads the block with 0xff from @p size to @p padded_size.
 * @param sl          device handle
 * @param fl          flash loader, see stlink_flashloader_start()
 * @param buf         data
 * @param size        number of bytes in @p buf
 * @param padded_size number of bytes to write, at least @p size
 * @return 0 on success, -1 on error
 */
int32_t stlink_write_buffer_to_sram(stlink_t *sl, flash_loader_t* fl, const uint8_t* buf, uint16_t size, uint16_t padded_size);

/** @} */
/** @endcond */

/* === Declaration of functions defined in progress.c === */

/**
 * Report the progress of operations on this device to the application instead of stdout.
 * @ingroup api_progress
 * @param sl      the device
 * @param handler function receiving the progress, NULL restores the output on stdout
 * @param user    pointer passed to every call of the handler
 */
void stlink_set_progress_handler(stlink_t *sl, stlink_progress_handler_t handler, void *user);

#ifdef __cplusplus
}
#endif

#endif // STLINK_H
