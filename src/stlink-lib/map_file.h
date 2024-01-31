/*
 * File: map_file.h
 *
 * File mapping
 */

#ifndef MAP_FILE_H
#define MAP_FILE_H

#ifndef O_BINARY
#define O_BINARY 0
#endif // O_BINARY

#ifdef STLINK_HAVE_SYS_MMAN_H
#include <sys/mman.h>
#else
#include <mmap.h>
#endif // STLINK_HAVE_SYS_MMAN_H

/* Memory mapped file */
typedef struct mapped_file {
  uint8_t *base;
  uint32_t len;
} mapped_file_t;

#define MAPPED_FILE_INITIALIZER                                                \
  { NULL, 0 }

int32_t check_file(stlink_t *, mapped_file_t *, stm32_addr_t);
int32_t map_file(mapped_file_t *, const char *);
void unmap_file(mapped_file_t *);

#endif // MAP_FILE_H
