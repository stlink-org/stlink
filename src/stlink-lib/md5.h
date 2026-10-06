/**
  ******************************************************************************
  * @file             md5.h
  * @brief            MD5 hash function
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-07-27
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
 * @ingroup api_sram_file
 * @param mf     the file
 * @param digest receives the 16 bytes of the digest
 */
void md5_calculate_digest(const mapped_file_t *mf, uint8_t digest[16]);

/**
 * Checksum of a mapped file (sum of all bytes), compatible with the official ST tools
 * @ingroup api_sram_file
 * @param mf the file
 * @return the checksum
 */
uint32_t stlink_checksum_calculate(const mapped_file_t *mf);

/**
 * Print the MD5 digest of a mapped file to stdout.
 * @deprecated Prints to stdout, use md5_calculate_digest() instead.
 * @ingroup api_legacy
 * @param mf the file
 */
void md5_calculate(mapped_file_t *mf);

/**
 * Print the checksum of a mapped file to stdout.
 * @deprecated Prints to stdout, use stlink_checksum_calculate() instead.
 * @ingroup api_legacy
 * @param mf the file
 */
void stlink_checksum(mapped_file_t *mf);

#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // MD5_H