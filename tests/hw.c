/**
  ******************************************************************************
  * @file             hw.c
  * @brief            Tests of the ST-LINK variant table and the feature flags
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-10-08
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

/*
 * Checks the table of ST-LINK variants and the derivation of the feature flags
 * without any hardware: product id classification (also against the PID macros
 * in usb.h), endpoint layout, decoding of recorded version replies and the
 * firmware thresholds of the capabilities.
 */


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stlink.h>
#include <stlink_hw.h>
#include <usb.h>


static int32_t failures = 0;

#define CHECK(cond) do { if(!(cond)) { fprintf(stderr, "FAILED line %d: %s\n", __LINE__, #cond); failures++; } } while (0)

static uint32_t caps(uint16_t pid, uint32_t jtag_v) {
    return (stlink_hw_capabilities(stlink_hw_lookup(pid), jtag_v));
}

static void check_table(void) {
    uint32_t count;
    const struct stlink_hw_desc *table = stlink_hw_table(&count);

    CHECK(count == 11);

    for(uint32_t i = 0; i < count; i++) {
        CHECK(table[i].name != NULL);
        CHECK(stlink_hw_lookup(table[i].pid) == &table[i]); // unique product ids

        for(uint32_t j = i + 1; j < count; j++) { CHECK(table[i].pid != table[j].pid); }

        // endpoint layout per generation
        if(table[i].gen == STLINK_HW_V1 || table[i].gen == STLINK_HW_V2) {
            CHECK(table[i].ep_req == 2 && table[i].ep_rep == 1);
        } else {
            CHECK(table[i].ep_req == 1 && table[i].ep_rep == 1 && table[i].ep_trace == 2);
        }

        CHECK(table[i].msc_framing == (table[i].gen == STLINK_HW_V1));
    }

    // classification of the ids that disagreed before
    CHECK(stlink_hw_lookup(0x3744)->gen == STLINK_HW_V1);
    CHECK(stlink_hw_lookup(0x3748)->gen == STLINK_HW_V2);
    CHECK(stlink_hw_lookup(0x374a)->gen == STLINK_HW_V2_1);
    CHECK(stlink_hw_lookup(0x374b)->gen == STLINK_HW_V2_1);
    CHECK(stlink_hw_lookup(0x3752)->gen == STLINK_HW_V2_1);
    CHECK(stlink_hw_lookup(0x3753)->gen == STLINK_HW_V3);
    CHECK(stlink_hw_lookup(0x3757)->gen == STLINK_HW_V3);
    CHECK(stlink_hw_lookup(0x3755) == NULL);
    CHECK(stlink_hw_lookup(0x0000) == NULL);

    CHECK(stlink_hw_major(stlink_hw_lookup(0x3744)) == 1);
    CHECK(stlink_hw_major(stlink_hw_lookup(0x374b)) == 2);
    CHECK(stlink_hw_major(stlink_hw_lookup(0x374e)) == 3);

    // the compatibility macros in usb.h follow the table
    for(uint32_t pid = 0x3700; pid <= 0x37ff; pid++) {
        const struct stlink_hw_desc *hw = stlink_hw_lookup((uint16_t) pid);

        CHECK(!!STLINK_SUPPORTED_USB_PID(pid) == (hw != NULL));
        CHECK(!!STLINK_V1_USB_PID(pid) == (hw != NULL && hw->gen == STLINK_HW_V1));
        CHECK(!!STLINK_V2_USB_PID(pid) == (hw != NULL && hw->gen == STLINK_HW_V2));
        CHECK(!!STLINK_V2_1_USB_PID(pid) == (hw != NULL && hw->gen == STLINK_HW_V2_1));
        CHECK(!!STLINK_V3_USB_PID(pid) == (hw != NULL && hw->gen == STLINK_HW_V3));
    }
}

static void check_decode(void) {
    stlink_version_t v;

    // ST-LINK/V2 V2J37S7, VID 0x0483, PID 0x3748
    const uint8_t v2[] = { 0x29, 0x47, 0x83, 0x04, 0x48, 0x37 };
    memset(&v, 0, sizeof(v));
    CHECK(stlink_hw_decode_version(v2, sizeof(v2), false, &v) == 0);
    CHECK(v.stlink_v == 2 && v.jtag_v == 37 && v.swim_v == 7);
    CHECK(v.st_vid == 0x0483 && v.stlink_pid == 0x3748);
    CHECK(v.jtag_api == STLINK_JTAG_API_V2);

    // ST-LINK/V1 V1J11S0: debug API V1, from J12: debug API V2
    const uint8_t v1_j11[] = { 0x12, 0xc0, 0x83, 0x04, 0x44, 0x37 };
    const uint8_t v1_j12[] = { 0x13, 0x00, 0x83, 0x04, 0x44, 0x37 };
    CHECK(stlink_hw_decode_version(v1_j11, sizeof(v1_j11), false, &v) == 0);
    CHECK(v.stlink_v == 1 && v.jtag_v == 11 && v.jtag_api == STLINK_JTAG_API_V1);
    CHECK(stlink_hw_decode_version(v1_j12, sizeof(v1_j12), false, &v) == 0);
    CHECK(v.stlink_v == 1 && v.jtag_v == 12 && v.jtag_api == STLINK_JTAG_API_V2);

    // STLINK-V3 V3J7M3B2S1, VID 0x0483, PID 0x374f
    const uint8_t v3[] = { 0x03, 0x01, 0x07, 0x03, 0x02, 0x00, 0x00, 0x00, 0x83, 0x04, 0x4f, 0x37 };
    CHECK(stlink_hw_decode_version(v3, sizeof(v3), true, &v) == 0);
    CHECK(v.stlink_v == 3 && v.jtag_v == 7 && v.swim_v == 1);
    CHECK(v.st_vid == 0x0483 && v.stlink_pid == 0x374f);
    CHECK(v.jtag_api == STLINK_JTAG_API_V3);

    // too short
    CHECK(stlink_hw_decode_version(v2, 5, false, &v) == -1);
    CHECK(stlink_hw_decode_version(v3, 11, true, &v) == -1);
}

static void check_capabilities(void) {
    // ST-LINK/V1: nothing
    CHECK(caps(0x3744, 13) == 0);

    // ST-LINK/V2 thresholds
    CHECK(caps(0x3748, 12) == STLINK_F_HAS_NRST);
    CHECK((caps(0x3748, 13) & (STLINK_F_HAS_TRACE | STLINK_F_HAS_TARGET_VOLTAGE)) ==
          (STLINK_F_HAS_TRACE | STLINK_F_HAS_TARGET_VOLTAGE));
    CHECK(!(caps(0x3748, 14) & STLINK_F_HAS_GETLASTRWSTATUS2));
    CHECK(caps(0x3748, 15) & STLINK_F_HAS_GETLASTRWSTATUS2);
    CHECK(!(caps(0x3748, 21) & STLINK_F_HAS_SWD_SET_FREQ));
    CHECK(caps(0x3748, 22) & STLINK_F_HAS_SWD_SET_FREQ);
    CHECK(!(caps(0x3748, 23) & STLINK_F_QUIRK_JTAG_DP_READ));
    CHECK(caps(0x3748, 24) & STLINK_F_QUIRK_JTAG_DP_READ);
    CHECK(caps(0x3748, 24) & STLINK_F_HAS_DAP_REG);
    CHECK(!(caps(0x3748, 25) & STLINK_F_HAS_MEM_16BIT));
    CHECK(caps(0x3748, 26) & STLINK_F_HAS_MEM_16BIT);
    CHECK(!(caps(0x3748, 27) & STLINK_F_HAS_AP_INIT));
    CHECK(caps(0x3748, 28) & STLINK_F_HAS_AP_INIT);
    CHECK(caps(0x3748, 31) & STLINK_F_QUIRK_JTAG_DP_READ);
    CHECK(!(caps(0x3748, 31) & STLINK_F_HAS_CSW));
    CHECK(!(caps(0x3748, 32) & STLINK_F_QUIRK_JTAG_DP_READ));
    CHECK(caps(0x3748, 32) & STLINK_F_HAS_CSW);
    CHECK(!(caps(0x3748, 45) & STLINK_F_HAS_RW8_512BYTES));

    // ST-LINK/V2-1 has the same thresholds
    CHECK(caps(0x374b, 37) == caps(0x3748, 37));

    // STLINK-V3
    CHECK(caps(0x374e, 1) & STLINK_F_HAS_TRACE);
    CHECK(caps(0x374e, 1) & STLINK_F_HAS_SWD_SET_FREQ);
    CHECK(caps(0x374e, 1) & STLINK_F_HAS_NRST);
    CHECK(!(caps(0x374e, 1) & STLINK_F_HAS_CSW));
    CHECK(caps(0x374e, 2) & STLINK_F_HAS_CSW);
    CHECK(!(caps(0x374e, 5) & STLINK_F_HAS_RW8_512BYTES));
    CHECK(caps(0x374e, 6) & STLINK_F_HAS_RW8_512BYTES);
    CHECK(!(caps(0x374e, 10) & STLINK_F_QUIRK_JTAG_DP_READ));

    CHECK(stlink_hw_capabilities(NULL, 40) == 0);
}

static void check_parse_version(void) {
    stlink_t *sl = calloc(1, sizeof(stlink_t));
    const uint8_t v2[] = { 0x29, 0x47, 0x83, 0x04, 0x48, 0x37 };     // V2J37S7
    const uint8_t v3[] = { 0x03, 0x01, 0x07, 0x03, 0x02, 0x00, 0x00, 0x00, 0x83, 0x04, 0x4f, 0x37 };
    const uint8_t v2_j12[] = { 0x23, 0x00, 0x83, 0x04, 0x48, 0x37 }; // V2J12S0

    // ST-LINK/V2: the backend leaves stlink_v at 2, the reply has the V1/V2 format
    sl->version.stlink_v = 2;
    memcpy(sl->q_buf, v2, sizeof(v2));
    CHECK(stlink_hw_parse_version(sl) == 0);
    CHECK(sl->version.jtag_v == 37);
    CHECK(sl->version.flags == caps(0x3748, 37));
    CHECK(sl->max_trace_freq == STLINK_V2_MAX_TRACE_FREQUENCY);

    // before J13: no trace, so no trace frequency
    sl->version.stlink_v = 2;
    memcpy(sl->q_buf, v2_j12, sizeof(v2_j12));
    CHECK(stlink_hw_parse_version(sl) == 0);
    CHECK(!(sl->version.flags & STLINK_F_HAS_TRACE));
    CHECK(sl->max_trace_freq == 0);

    // STLINK-V3: stlink_v is 3 before the version is read, the reply has the V3 format
    memset(sl, 0, sizeof(*sl));
    sl->version.stlink_v = 3;
    memcpy(sl->q_buf, v3, sizeof(v3));
    CHECK(stlink_hw_parse_version(sl) == 0);
    CHECK(sl->version.stlink_pid == 0x374f);
    CHECK(sl->version.flags == caps(0x374f, 7));
    CHECK(sl->max_trace_freq == STLINK_V3_MAX_TRACE_FREQUENCY);

    // unknown product id: generic descriptor of the generation
    stlink_version_t v;
    memset(&v, 0, sizeof(v));
    v.stlink_v = 2;
    v.stlink_pid = 0x1234;
    CHECK(stlink_hw_of(&v) != NULL && stlink_hw_of(&v)->gen == STLINK_HW_V2);
    v.stlink_v = 0;
    CHECK(stlink_hw_of(&v) == NULL);

    free(sl);
}

int32_t main(void) {
    check_table();
    check_decode();
    check_capabilities();
    check_parse_version();

    if(failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return (1);
    }

    printf("all checks passed\n");
    return (0);
}
