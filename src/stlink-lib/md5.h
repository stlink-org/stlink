/**
  ******************************************************************************
  * @file           : md5.h
  * @brief          : MD5 hash function
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef MD5_H
#define MD5_H

#include <stdint.h>
#include <stdio.h>

#include <stlink.h>

#include "map_file.h"


#ifdef  __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * MD5 digest of a mapped file
 * @param mf     the file
 * @param digest receives the 16 bytes of the digest
 */
void md5_calculate_digest(const mapped_file_t *mf, uint8_t digest[16]);

/**
 * Checksum of a mapped file (sum of all bytes), compatible with the official ST tools
 * @param mf the file
 * @return the checksum
 */
uint32_t stlink_checksum_calculate(const mapped_file_t *mf);

/* Deprecated: print the result to stdout, use the functions above instead */
void md5_calculate(mapped_file_t *);
void stlink_checksum(mapped_file_t *);

#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // MD5_H