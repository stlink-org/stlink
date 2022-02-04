#include <fcntl.h>
#include <sys/stat.h>
#include <md5.h>
#include <unistd.h>
#include <stlink.h>
#include <logging.h>

#include "map_file.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef MAX_FILE_SIZE
#define MAX_FILE_SIZE (1<<20) // 1 GB max file size
#endif

int map_file(mapped_file_t *mf, const char *path) {
  int error = -1;
  struct stat st;

  const int fd = open(path, O_RDONLY | O_BINARY);

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
      fprintf(stderr, "mmap() size_t overflow for file %s\n", path);
      goto on_error;
    }
  }

  mf->base =
      (uint8_t *)mmap(NULL, (size_t)(st.st_size), PROT_READ, MAP_SHARED, fd, 0);

  if (mf->base == MAP_FAILED) {
    fprintf(stderr, "mmap() == MAP_FAILED for file %s\n", path);
    goto on_error;
  }

  mf->len = (size_t)st.st_size;
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
