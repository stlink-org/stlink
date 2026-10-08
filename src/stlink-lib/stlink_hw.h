/**
  ******************************************************************************
  * @file             stlink_hw.h
  * @brief            ST-LINK hardware variants and their capabilities (internal)
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-10-05
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */


#ifndef STLINK_HW_H
#define STLINK_HW_H

#include <stdbool.h>
#include <stdint.h>

#include <stlink.h>


#ifdef __cplusplus
extern "C" {
#endif

/** @cond STLINK_INTERNAL */
/**
 * @addtogroup api_internal
 * @{
 */

/**
 * Hardware generation of an ST-LINK.
 *
 * The generation decides about the USB protocol and the endpoint layout.
 * stlink_version_t::stlink_v keeps the numbering of the ST-LINK firmware
 * (1, 2 or 3, i.e. ST-LINK/V2-1 reports 2), see stlink_hw_major().
 */
enum stlink_hw_gen {
    STLINK_HW_V1,       ///< ST-LINK/V1: commands wrapped in USB mass storage command blocks
    STLINK_HW_V2,       ///< ST-LINK/V2: full speed USB
    STLINK_HW_V2_1,     ///< ST-LINK/V2-1: like ST-LINK/V2, other endpoint layout, mass storage and virtual COM port
    STLINK_HW_V3,       ///< STLINK-V3: high speed USB, debug API V3
};

/** Description of an ST-LINK variant, selected by its USB product id */
struct stlink_hw_desc {
    uint16_t pid;               ///< USB product id (vendor id 0x0483), 0 for the generic descriptors
    enum stlink_hw_gen gen;     ///< hardware generation
    const char *name;           ///< name of the variant, e.g. "ST-LINK/V2-1 (no mass storage)"
    bool msc_framing;           ///< commands are wrapped in USB mass storage command blocks (ST-LINK/V1)
    uint8_t ep_req;             ///< endpoint number for commands (OUT)
    uint8_t ep_rep;             ///< endpoint number for replies (IN)
    uint8_t ep_trace;           ///< endpoint number for SWO trace data (IN), 0 if none
    uint32_t trace_buf_len;     ///< size of the SWO trace buffer in bytes, 0 if none
    uint32_t max_trace_freq;    ///< maximum SWO frequency in Hz, 0 if none
};

/**
 * Look up an ST-LINK variant by its USB product id.
 * @param pid USB product id (vendor id 0x0483)
 * @return the descriptor, or NULL if the product id is not a supported ST-LINK
 */
const struct stlink_hw_desc *stlink_hw_lookup(uint16_t pid);

/**
 * Descriptor of the ST-LINK described by a version.
 *
 * Looks up stlink_version_t::stlink_pid; for an unknown product id a generic
 * descriptor of the generation in stlink_version_t::stlink_v is returned.
 *
 * @param version version of the ST-LINK, see stlink_version()
 * @return the descriptor, or NULL if neither the product id nor the generation is known
 */
const struct stlink_hw_desc *stlink_hw_of(const stlink_version_t *version);

/**
 * All supported ST-LINK variants.
 * @param count receives the number of entries
 * @return the table, sorted by generation
 */
const struct stlink_hw_desc *stlink_hw_table(uint32_t *count);

/**
 * Generation number as reported by the ST-LINK firmware (stlink_version_t::stlink_v).
 * @param hw descriptor
 * @return 1, 2 (ST-LINK/V2 and V2-1) or 3
 */
uint32_t stlink_hw_major(const struct stlink_hw_desc *hw);

/**
 * Decode the reply of the version command.
 *
 * Sets stlink_v, jtag_v, swim_v, st_vid, stlink_pid and jtag_api of
 * @p version; the feature flags are not changed (see stlink_hw_capabilities()).
 *
 * @param reply     the reply
 * @param len       length of the reply in bytes: at least 6, or 12 for the V3 format
 * @param v3_format the reply is the one of STLINK_GET_VERSION_APIV3 (STLINK-V3)
 * @param version   receives the decoded version
 * @return 0 on success, -1 if the reply is too short
 */
int32_t stlink_hw_decode_version(const uint8_t *reply, uint32_t len, bool v3_format,
                                 stlink_version_t *version);

/**
 * Feature flags of an ST-LINK.
 *
 * Combines the hardware generation and the firmware version into STLINK_F_*
 * flags. All firmware thresholds are kept in this function.
 *
 * @param hw     descriptor of the ST-LINK, NULL gives no flags
 * @param jtag_v firmware version of the JTAG/SWD part (stlink_version_t::jtag_v)
 * @return the STLINK_F_* flags
 */
uint32_t stlink_hw_capabilities(const struct stlink_hw_desc *hw, uint32_t jtag_v);

/**
 * Decode the version reply in stlink_t::q_buf and derive the capabilities.
 *
 * Fills stlink_t::version (including the flags) and stlink_t::max_trace_freq.
 * The format of the reply is selected by stlink_version_t::stlink_v, which the
 * backend sets before reading the version (3 for an STLINK-V3).
 *
 * @param sl device handle, the reply of the version command in stlink_t::q_buf
 * @return 0 on success, -1 if the reply cannot be decoded
 */
int32_t stlink_hw_parse_version(stlink_t *sl);

/** @} */
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif // STLINK_HW_H
