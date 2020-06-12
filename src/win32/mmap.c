#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#include "mmap.h"

void *mmap (void *addr, size_t len, int prot, int flags, int fd, long long offset) {
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

int munmap (void *addr, size_t len) {
    free (addr);
    return(0);
    (void)len;
}
