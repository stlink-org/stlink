/**
  ******************************************************************************
  * @file             sys_mmap.h
  * @brief            mmap() and munmap() for Windows
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-10-08
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */


#ifndef SYS_MMAP_H
#define SYS_MMAP_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <sys/types.h>


#ifdef STLINK_HAVE_SYS_MMAN_H
#include <sys/mman.h>
#else

#define PROT_READ     (1 << 0)
#define PROT_WRITE    (1 << 1)

#define MAP_SHARED    (1 << 0)
#define MAP_PRIVATE   (1 << 1)
#define MAP_ANONYMOUS (1 << 5)
#define MAP_FAILED    ((void *)-1)

void *mmap(void *addr, uint32_t len, int32_t prot, int32_t flags, int32_t fd, int64_t offset);
int32_t munmap(void *addr, uint32_t len);

#endif // STLINK_HAVE_SYS_MMAN_H

#endif // SYS_MMAP_H
