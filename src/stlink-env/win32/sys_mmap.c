/**
  ******************************************************************************
  * @file             sys_mmap.c
  * @brief            mmap() and munmap() for Windows
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-10-08
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */


#include "sys_mmap.h"


void *mmap(void *addr, uint32_t len, int32_t prot, int32_t flags, int32_t fd, int64_t offset) {
    void *buf;
    ssize_t count;

    (void)flags;

    if (addr || fd == -1 || (prot & PROT_WRITE)) { return (MAP_FAILED); }

    buf = malloc(len);

    if (NULL == buf) { return (MAP_FAILED); }

    if (lseek(fd, offset, SEEK_SET) != offset) {
        free(buf);
        return (MAP_FAILED);
    }

    count = read(fd, buf, len);

    if (count != (ssize_t) len) {
        free(buf);
        return (MAP_FAILED);
    }

    return (buf);
}

int32_t munmap(void *addr, uint32_t len) {
    free (addr);
    (void)len;

    return (0);
}
