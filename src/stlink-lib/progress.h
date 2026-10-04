/**
  ******************************************************************************
  * @file           : progress.h
  * @brief          : Progress reporting of longer operations
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-10-04
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

#ifdef  __cplusplus
extern "C" {
#endif // __cplusplus

/*
 * Internal: report progress to the handler set with stlink_set_progress_handler(),
 * or print it to stdout without a handler.
 * verbose_only: print to stdout only if sl->verbose >= 1 (as before for these call sites).
 */
void stlink_progress_event(stlink_t *sl, enum stlink_progress_event event, bool verbose_only);
void stlink_progress_write(stlink_t *sl, enum stlink_progress_unit unit, uint32_t done, uint32_t total,
                           bool verbose_only);
void stlink_progress_page_erased(stlink_t *sl, uint32_t addr, uint32_t size);
void stlink_progress_file(stlink_t *sl, const char *path, const mapped_file_t *mf);

#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // PROGRESS_H
