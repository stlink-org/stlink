/**
  ******************************************************************************
  * @file           : stlink_fs.h
  * @brief          : Filesystem lookups that differ per platform
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-09-17
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef STLINK_FS_H
#define STLINK_FS_H

#include <stdbool.h>
#include <stddef.h>

#define CHIP_FILE_EXT ".chip"

/* Directory holding the running executable, without a trailing separator.
 * False where the platform will not say. */
bool stlink_exe_dir(char *buf, size_t len);

/* Read every chip description file in dir. False if it held none, including
 * when dir cannot be read at all. */
bool search_for_chips(const char *dir);

#endif // STLINK_FS_H
