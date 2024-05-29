/*
 * File: map_file.c
 *
 * File mapping
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <stlink.h>
#include "map_file.h"

#include "read_write.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

// 1 GB max file size
#ifndef MAX_FILE_SIZE
#define MAX_FILE_SIZE (1<<20)
#endif

/* Limit the block size to compare to 0x1800 as anything larger will stall the
 * STLINK2 Maybe STLINK V1 needs smaller value!
 */
int32_t check_file(stlink_t *sl, mapped_file_t *mf, stm32_addr_t addr) {
  uint32_t off;
  uint32_t n_cmp = sl->flash_pgsz;

  if (n_cmp > 0x1800) {
    n_cmp = 0x1800;
  }

  for (off = 0; off < mf->len; off += n_cmp) {
    uint32_t aligned_size;

    uint32_t cmp_size = n_cmp; // adjust last page size

    if ((off + n_cmp) > mf->len) {
      cmp_size = mf->len - off;
    }

    aligned_size = cmp_size;

    if (aligned_size & (4 - 1)) {
      aligned_size = (cmp_size + 4) & ~(4 - 1);
    }

    stlink_read_mem32(sl, addr + off, (uint16_t)aligned_size);

    if (memcmp(sl->q_buf, mf->base + off, cmp_size)) {
      return (-1);
    }
  }

  return (0);
}

int32_t map_file(mapped_file_t *mf, const char *path) {
  int32_t error = -1;
  struct stat st;

  const int32_t fd = open(path, O_RDONLY | O_BINARY);

  if (fd == -1) {
    fprintf(stderr, "open(%s) == -1\n", path);
    return (-1);
  }

  if (fstat(fd, &st) == -1) {
    fprintf(stderr, "fstat(%s) == -1\n", path);
    goto on_error;
  }

  if (sizeof(st.st_size) != sizeof(size_t)) {
    // on 32 bit systems, check if there is an overflow
    if (st.st_size > (off_t)MAX_FILE_SIZE  /*1 GB*/ ) {
      // limit file size to 1 GB
      fprintf(stderr, "mmap() uint32_t overflow for file %s\n", path);
      goto on_error;
    }
  }

  mf->base =
      (uint8_t *)mmap(NULL, (size_t)(st.st_size), PROT_READ, MAP_SHARED, fd, 0);

  if (mf->base == MAP_FAILED) {
    fprintf(stderr, "mmap() == MAP_FAILED for file %s\n", path);
    goto on_error;
  }

  mf->len = (uint32_t)st.st_size;
  error = 0; // success

on_error:
  close(fd);
  return (error);
}

void unmap_file(mapped_file_t *mf) {
  munmap((void *)mf->base, mf->len);
  mf->base = (unsigned char *)MAP_FAILED;
  mf->len = 0;
}
