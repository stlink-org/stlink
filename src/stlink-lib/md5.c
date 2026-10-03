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

#include "md5.h"

#include "map_file.h"
#include "lib_md5.h"


void md5_calculate(mapped_file_t *mf) {
  // calculate md5 checksum of given binary file
  Md5Context md5Context;
  MD5_HASH md5Hash;
  Md5Initialise(&md5Context);
  Md5Update(&md5Context, mf->base, (uint32_t) mf->len);
  Md5Finalise(&md5Context, &md5Hash);
  printf("md5 checksum: ");

  for(int32_t i = 0; i < (int32_t) sizeof(md5Hash); i++) {
    printf("%x", md5Hash.bytes[i]);
  }

  printf(", ");
}

void stlink_checksum(mapped_file_t *mp) {
  /* checksum that backward compatible with official ST tools */
  uint32_t sum = 0;
  uint8_t *mp_byte = (uint8_t *)mp->base;

  for(uint32_t i = 0; i < mp->len; ++i) {
    sum += mp_byte[i];
  }

  printf("stlink checksum: 0x%08x\n", sum);
}