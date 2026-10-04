/**
  ******************************************************************************
  * @file           : md5.c
  * @brief          : MD5 hash function
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#include <string.h>

#include "md5.h"

#include "map_file.h"
#include "lib_md5.h"


void md5_calculate_digest(const mapped_file_t *mf, uint8_t digest[16]) {
  // calculate md5 checksum of given binary file
  Md5Context md5Context;
  MD5_HASH md5Hash;
  Md5Initialise(&md5Context);
  Md5Update(&md5Context, mf->base, (uint32_t) mf->len);
  Md5Finalise(&md5Context, &md5Hash);
  memcpy(digest, md5Hash.bytes, sizeof(md5Hash.bytes));
}

uint32_t stlink_checksum_calculate(const mapped_file_t *mf) {
  /* checksum that backward compatible with official ST tools */
  uint32_t sum = 0;
  const uint8_t *mp_byte = (const uint8_t *)mf->base;

  for(uint32_t i = 0; i < mf->len; ++i) {
    sum += mp_byte[i];
  }

  return (sum);
}

void md5_calculate(mapped_file_t *mf) {
  uint8_t digest[16];
  md5_calculate_digest(mf, digest);
  printf("md5 checksum: ");

  for(int32_t i = 0; i < (int32_t) sizeof(digest); i++) {
    printf("%x", digest[i]);
  }

  printf(", ");
}

void stlink_checksum(mapped_file_t *mp) {
  printf("stlink checksum: 0x%08x\n", stlink_checksum_calculate(mp));
}
