/*
 * File: md5.c
 *
 * MD5 hash function
 */

#include <stdint.h>
#include <stdio.h>

#include <stlink.h>
#include "md5.h"

#include "map_file.h"
#include "lib_md5.h"

void md5_calculate(mapped_file_t *mf) {
  // calculate md5 checksum of given binary file
  Md5Context md5Context;
  MD5_HASH md5Hash;
  Md5Initialise(&md5Context);
  Md5Update(&md5Context, mf->base, (uint32_t)mf->len);
  Md5Finalise(&md5Context, &md5Hash);
  printf("md5 checksum: ");

  for (int32_t i = 0; i < (int32_t)sizeof(md5Hash); i++) {
    printf("%x", md5Hash.bytes[i]);
  }

  printf(", ");
}

void stlink_checksum(mapped_file_t *mp) {
  /* checksum that backward compatible with official ST tools */
  uint32_t sum = 0;
  uint8_t *mp_byte = (uint8_t *)mp->base;

  for (uint32_t i = 0; i < mp->len; ++i) {
    sum += mp_byte[i];
  }

  printf("stlink checksum: 0x%08x\n", sum);
}