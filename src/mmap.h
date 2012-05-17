#ifndef STLINK_MMAP_H
#define STLINK_MMAP_H

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#else

#define PROT_READ	(1<<0)
#define PROT_WRITE	(1<<1)

#define MAP_SHARED	(1<<0)
#define MAP_PRIVATE	(1<<1)

#define MAP_ANONYMOUS (1<<5)

#define MAP_FAILED ((void *)-1)

#ifdef __cplusplus
extern "C" {
#endif

void *mmap(void *addr, size_t len, int prot, int flags, int fd, long long offset);
int munmap(void *addr, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* HAVE_SYS_MMAN_H */

#endif /* STLINK_MMAP_H */
