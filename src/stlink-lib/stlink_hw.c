/**
  ******************************************************************************
  * @file             stlink_hw.c
  * @brief            ST-LINK hardware variants and their capabilities
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-10-05
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */


#include <stddef.h>
#include <stdint.h>

#include <stlink.h>

#include "stlink_hw.h"
#include "logging.h"
#include "read_write.h"


/*
 * Firmware versions (the "J" in V2J45 / V3J7) from which a feature is available.
 * Collected here, so that no other code has to know them: everything else tests
 * the STLINK_F_* flags derived from them by stlink_hw_capabilities().
 */
#define STLINK_HW_V1_J_API_V2           12  // ST-LINK/V1: debug API V2 (also SWD); OpenOCD switches from J11
#define STLINK_HW_V2_J_TRACE            13  // ST-LINK/V2: SWO trace, target voltage
#define STLINK_HW_V2_J_RW_STATUS2       15  // ST-LINK/V2: extended status of the last read/write
#define STLINK_HW_V2_J_SWD_FREQ         22  // ST-LINK/V2: SWD frequency
#define STLINK_HW_V2_J_JTAG_FREQ        24  // ST-LINK/V2: JTAG frequency, DAP registers, start of the JTAG DP read quirk
#define STLINK_HW_V2_J_MEM_16BIT        26  // ST-LINK/V2: 16 bit memory accesses
#define STLINK_HW_V2_J_AP_INIT          28  // ST-LINK/V2: access port initialisation
#define STLINK_HW_V2_J_DPBANKSEL        32  // ST-LINK/V2: DP bank selection, CSW in memory commands, end of the quirk
#define STLINK_HW_V3_J_DPBANKSEL         2  // STLINK-V3: DP bank selection, CSW in memory commands
#define STLINK_HW_V3_J_RW8_512           6  // STLINK-V3: 8 bit transfers of up to 512 bytes

/* Fields shared by all variants of a generation */
#define HW_V1   .gen = STLINK_HW_V1, .msc_framing = true, .ep_req = 2, .ep_rep = 1, .ep_trace = 0, \
                .trace_buf_len = 0, .max_trace_freq = 0
#define HW_V2   .gen = STLINK_HW_V2, .msc_framing = false, .ep_req = 2, .ep_rep = 1, .ep_trace = 3, \
                .trace_buf_len = STLINK_V2_TRACE_BUF_LEN, .max_trace_freq = STLINK_V2_MAX_TRACE_FREQUENCY
#define HW_V2_1 .gen = STLINK_HW_V2_1, .msc_framing = false, .ep_req = 1, .ep_rep = 1, .ep_trace = 2, \
                .trace_buf_len = STLINK_V2_TRACE_BUF_LEN, .max_trace_freq = STLINK_V2_MAX_TRACE_FREQUENCY
#define HW_V3   .gen = STLINK_HW_V3, .msc_framing = false, .ep_req = 1, .ep_rep = 1, .ep_trace = 2, \
                .trace_buf_len = STLINK_V3_TRACE_BUF_LEN, .max_trace_freq = STLINK_V3_MAX_TRACE_FREQUENCY

/*
 * Supported ST-LINK variants, one entry per USB product id.
 *
 * Several products share a product id: the id depends on the hardware
 * generation and on the USB configuration of the firmware (mass storage,
 * number of virtual COM ports), not on the product. Mapping according to
 * ST technical note TN1235 "Overview of ST-LINK derivatives" (Rev 7):
 *   0x374b, 0x3752  ST-LINK/V2-1, with / without mass storage
 *   0x374e, 0x3754  STLINK-V3 without bridge functions, with / without mass storage
 *   0x374f, 0x3753  STLINK-V3 with bridge functions, mass storage / second virtual COM port
 *   0x3757          STLINK-V3PWR
 * Further ids: 0x3744 ST-LINK/V1, 0x3748 ST-LINK/V2, 0x374a ST-LINK/V2-1 with the
 * "STM32+Audio" firmware (added in #790), 0x374d STLINK-V3 USB loader (firmware
 * update mode). Not supported: 0x3755, the STLINK-V3PWR USB loader (OpenOCD).
 *
 * config/udev/rules.d/ and the STLINK_*_USB_PID() macros in usb.h follow this
 * table (checked by tests/hw.c).
 */
static const struct stlink_hw_desc hw_table[] = {
    { .pid = 0x3744, .name = "ST-LINK/V1",                                   HW_V1 },
    { .pid = 0x3748, .name = "ST-LINK/V2",                                   HW_V2 },
    { .pid = 0x374a, .name = "ST-LINK/V2-1 (STM32+Audio firmware)",          HW_V2_1 },
    { .pid = 0x374b, .name = "ST-LINK/V2-1",                                 HW_V2_1 },
    { .pid = 0x3752, .name = "ST-LINK/V2-1 (no mass storage)",               HW_V2_1 },
    { .pid = 0x374d, .name = "STLINK-V3 (USB loader)",                       HW_V3 },
    { .pid = 0x374e, .name = "STLINK-V3 (no bridge)",                        HW_V3 },
    { .pid = 0x3754, .name = "STLINK-V3 (no bridge, no mass storage)",       HW_V3 },
    { .pid = 0x374f, .name = "STLINK-V3 (bridge)",                           HW_V3 },
    { .pid = 0x3753, .name = "STLINK-V3 (bridge, two virtual COM ports)",    HW_V3 },
    { .pid = 0x3757, .name = "STLINK-V3PWR",                                 HW_V3 },
};

/* Generic descriptors for an ST-LINK whose product id is not in the table */
static const struct stlink_hw_desc hw_generic[] = {
    { .pid = 0, .name = "ST-LINK/V1 (unknown product id)",                   HW_V1 },
    { .pid = 0, .name = "ST-LINK/V2 (unknown product id)",                   HW_V2 },
    { .pid = 0, .name = "STLINK-V3 (unknown product id)",                    HW_V3 },
};


const struct stlink_hw_desc *stlink_hw_lookup(uint16_t pid) {
    for(size_t i = 0; i < STLINK_ARRAY_SIZE(hw_table); i++) {
        if(hw_table[i].pid == pid) { return (&hw_table[i]); }
    }

    return (NULL);
}

const struct stlink_hw_desc *stlink_hw_of(const stlink_version_t *version) {
    const struct stlink_hw_desc *hw = stlink_hw_lookup((uint16_t) version->stlink_pid);

    if(hw != NULL) { return (hw); }

    if(version->stlink_v >= 1 && version->stlink_v <= 3) {
        return (&hw_generic[version->stlink_v - 1]);
    }

    return (NULL);
}

const struct stlink_hw_desc *stlink_hw_table(uint32_t *count) {
    *count = (uint32_t) STLINK_ARRAY_SIZE(hw_table);
    return (hw_table);
}

uint32_t stlink_hw_major(const struct stlink_hw_desc *hw) {
    switch (hw->gen) {
    case STLINK_HW_V1:
        return (1);
    case STLINK_HW_V2:
    case STLINK_HW_V2_1:
        return (2);
    case STLINK_HW_V3:
    default:
        return (3);
    }
}

int32_t stlink_hw_decode_version(const uint8_t *reply, uint32_t len, bool v3_format,
                                 stlink_version_t *version) {
    if(v3_format) {
        // STLINK-V3: one byte per version, VID and PID at offset 8 and 10
        if(len < 12) { return (-1); }

        version->stlink_v = reply[0];
        version->swim_v = reply[1];
        version->jtag_v = reply[2];
        version->st_vid = read_uint16(reply, 8);
        version->stlink_pid = read_uint16(reply, 10);
        version->jtag_api = STLINK_JTAG_API_V3;
        return (0);
    }

    // ST-LINK/V1, V2, V2-1:
    // byte 0      | byte 1          || byte 2-3 | byte 4-5
    // 4 bit       | 6 bit  | 6 bit  || 16 bit   | 16 bit
    // stlink_v    | jtag_v | swim_v || st_vid   | stlink_pid
    if(len < 6) { return (-1); }

    version->stlink_v = (reply[0] & 0xf0) >> 4;
    version->jtag_v = ((reply[0] & 0x0f) << 2) | ((reply[1] & 0xc0) >> 6);
    version->swim_v = reply[1] & 0x3f;
    version->st_vid = read_uint16(reply, 2);
    version->stlink_pid = read_uint16(reply, 4);

    if(version->stlink_v == 1) {
        version->jtag_api = (version->jtag_v >= STLINK_HW_V1_J_API_V2) ? STLINK_JTAG_API_V2 : STLINK_JTAG_API_V1;
    } else {
        version->jtag_api = STLINK_JTAG_API_V2;
    }

    return (0);
}

uint32_t stlink_hw_capabilities(const struct stlink_hw_desc *hw, uint32_t jtag_v) {
    uint32_t flags = 0;

    if(hw == NULL) { return (0); }

    switch (hw->gen) {
    case STLINK_HW_V1:
        // no SWO trace, no target voltage, no NRST control
        break;

    case STLINK_HW_V2:
    case STLINK_HW_V2_1:
        flags |= STLINK_F_HAS_NRST;

        if(jtag_v >= STLINK_HW_V2_J_TRACE) {
            flags |= STLINK_F_HAS_TRACE | STLINK_F_HAS_TARGET_VOLTAGE;
        }

        if(jtag_v >= STLINK_HW_V2_J_RW_STATUS2) { flags |= STLINK_F_HAS_GETLASTRWSTATUS2; }

        if(jtag_v >= STLINK_HW_V2_J_SWD_FREQ) { flags |= STLINK_F_HAS_SWD_SET_FREQ; }

        if(jtag_v >= STLINK_HW_V2_J_JTAG_FREQ) {
            flags |= STLINK_F_HAS_JTAG_SET_FREQ | STLINK_F_HAS_DAP_REG;

            if(jtag_v < STLINK_HW_V2_J_DPBANKSEL) { flags |= STLINK_F_QUIRK_JTAG_DP_READ; }
        }

        if(jtag_v >= STLINK_HW_V2_J_MEM_16BIT) { flags |= STLINK_F_HAS_MEM_16BIT; }

        if(jtag_v >= STLINK_HW_V2_J_AP_INIT) { flags |= STLINK_F_HAS_AP_INIT; }

        if(jtag_v >= STLINK_HW_V2_J_DPBANKSEL) { flags |= STLINK_F_HAS_DPBANKSEL; }

        break;

    case STLINK_HW_V3:
        // all firmware versions
        flags |= STLINK_F_HAS_NRST | STLINK_F_HAS_TRACE | STLINK_F_HAS_TARGET_VOLTAGE |
                 STLINK_F_HAS_GETLASTRWSTATUS2 | STLINK_F_HAS_SWD_SET_FREQ |
                 STLINK_F_HAS_JTAG_SET_FREQ | STLINK_F_HAS_DAP_REG | STLINK_F_HAS_MEM_16BIT |
                 STLINK_F_HAS_AP_INIT;

        if(jtag_v >= STLINK_HW_V3_J_DPBANKSEL) { flags |= STLINK_F_HAS_DPBANKSEL; }

        if(jtag_v >= STLINK_HW_V3_J_RW8_512) { flags |= STLINK_F_HAS_RW8_512BYTES; }

        break;
    }

    return (flags);
}

int32_t stlink_hw_parse_version(stlink_t *sl) {
    const bool v3_format = (sl->version.stlink_v >= 3);
    const struct stlink_hw_desc *hw;

    if(stlink_hw_decode_version(sl->q_buf, v3_format ? 12 : 6, v3_format, &sl->version)) {
        return (-1);
    }

    hw = stlink_hw_of(&sl->version);
    sl->version.flags = stlink_hw_capabilities(hw, sl->version.jtag_v);
    sl->max_trace_freq = (hw != NULL && (sl->version.flags & STLINK_F_HAS_TRACE)) ? hw->max_trace_freq : 0;

    DLOG("stlink variant  = %s\n", (hw != NULL) ? hw->name : "unknown");
    DLOG("stlink features = 0x%04x\n", sl->version.flags);
    return (0);
}
