#ifndef MMAP_H
#define MMAP_H

#include <stdint.h>

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

#endif // MMAP_H
