/**
  ******************************************************************************
  * @file             progress.h
  * @brief            Progress reporting of longer operations
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-10-04
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */


#ifndef PROGRESS_H
#define PROGRESS_H

#include <stdbool.h>
#include <stdint.h>

#include <stlink.h>

#include "map_file.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/** @cond STLINK_INTERNAL */
/**
 * @addtogroup api_internal
 * @{
 */

/**
 * Report an event without data to the progress handler set with
 * stlink_set_progress_handler(), or print it to stdout without a handler.
 * @param sl           device handle
 * @param event        the event
 * @param verbose_only print to stdout only if sl->verbose >= 1 (as before for these call sites)
 */
void stlink_progress_event(stlink_t *sl, enum stlink_progress_event event, bool verbose_only);

/**
 * Report the progress of a flash write (STLINK_PROGRESS_WRITE).
 * @param sl           device handle
 * @param unit         unit of @p done and @p total
 * @param done         amount written
 * @param total        amount to write
 * @param verbose_only print to stdout only if sl->verbose >= 1
 */
void stlink_progress_write(stlink_t *sl, enum stlink_progress_unit unit, uint32_t done, uint32_t total,
                           bool verbose_only);

/**
 * Report an erased flash page or sector (STLINK_PROGRESS_PAGE_ERASED).
 * @param sl   device handle
 * @param addr start address of the page
 * @param size size of the page in bytes
 */
void stlink_progress_page_erased(stlink_t *sl, uint32_t addr, uint32_t size);

/**
 * Report a file to be written, with its size, MD5 digest and checksum (STLINK_PROGRESS_FILE).
 * @param sl   device handle
 * @param path path of the file
 * @param mf   the mapped file
 */
void stlink_progress_file(stlink_t *sl, const char *path, const mapped_file_t *mf);

/** @} */
/** @endcond */

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // PROGRESS_H
