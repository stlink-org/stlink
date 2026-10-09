/**
  ******************************************************************************
  * @file             map_file.h
  * @brief            File mapping
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */


#ifndef MAP_FILE_H
#define MAP_FILE_H

/** @cond STLINK_INTERNAL */
#ifndef O_BINARY
#define O_BINARY 0
#endif // O_BINARY
/** @endcond */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef STLINK_HAVE_SYS_MMAN_H
    #include <sys/mman.h> // use system-header
#else
    #include <sys_mmap.h> // implemented fallback
#endif

#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <stlink.h>


/**
 * @addtogroup api_sram_file
 * @{
 */

/** A file mapped into memory (read only), see map_file() */
typedef struct mapped_file {
    uint8_t *base; ///< start of the file contents
    uint32_t len;  ///< size of the file in bytes
} mapped_file_t;

/** Initializer for an unmapped mapped_file_t */
#define MAPPED_FILE_INITIALIZER                                                \
  { NULL, 0 }

/** @} */

/** @cond STLINK_INTERNAL */
/**
 * Compare a mapped file with the target memory.
 * Reads in chunks of at most one flash page (max. 6 KiB).
 * @ingroup api_internal
 * @param sl   device handle
 * @param mf   the file
 * @param addr address of the first byte to compare
 * @return     0 if the memory matches, -1 at the first difference
 */
int32_t check_file(stlink_t *sl, mapped_file_t *mf, stm32_addr_t addr);
/** @endcond */

/**
 * @addtogroup api_sram_file
 * @{
 */

/**
 * Map a file into memory for reading.
 * @param mf   receives the mapping; release it with unmap_file()
 * @param path the file, at most 1 GiB on 32 bit systems
 * @return     0 on success, -1 on error (logged)
 */
int32_t map_file(mapped_file_t *mf, const char *path);

/**
 * Release a mapping created by map_file().
 * @param mf the mapping, invalid afterwards
 */
void unmap_file(mapped_file_t *mf);

/** @} */

#endif // MAP_FILE_H
