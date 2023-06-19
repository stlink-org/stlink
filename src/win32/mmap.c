#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "mmap.h"

void *mmap (void *addr, uint32_t len, int32_t prot, int32_t flags, int32_t fd, int64_t offset) {
    void *buf;
    ssize_t count;

    if ( addr || fd == -1 || (prot & PROT_WRITE)) { return(MAP_FAILED); }

    buf = malloc(len);

    if ( NULL == buf ) { return(MAP_FAILED); }

    if (lseek(fd, offset, SEEK_SET) != offset) {
        free(buf);
        return(MAP_FAILED);
    }

    count = read(fd, buf, len);

    if (count != (ssize_t)len) {
        free (buf);
        return(MAP_FAILED);
    }

    return(buf);
    (void)flags;
}

int32_t munmap (void *addr, uint32_t len) {
    free (addr);
    return(0);
    (void)len;
}
