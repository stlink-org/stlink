#ifndef _SEMIHOSTING_H_
#define _SEMIHOSTING_H_

#include <stlink.h>

#define SEMIHOST_SYS_OPEN     0x01
#define SEMIHOST_SYS_CLOSE    0x02
#define SEMIHOST_SYS_WRITEC   0x03
#define SEMIHOST_SYS_WRITE0   0x04
#define SEMIHOST_SYS_WRITE    0x05
#define SEMIHOST_SYS_READ     0x06
#define SEMIHOST_SYS_READC    0x07
#define SEMIHOST_SYS_ISERROR  0x08
#define SEMIHOST_SYS_ISTTY    0x09
#define SEMIHOST_SYS_SEEK     0x0A

#define SEMIHOST_SYS_FLEN     0x0C
#define SEMIHOST_SYS_TMPNAM   0x0D
#define SEMIHOST_SYS_REMOVE   0x0E
#define SEMIHOST_SYS_RENAME   0x0E
#define SEMIHOST_SYS_CLOCK    0x10
#define SEMIHOST_SYS_TIME     0x11

#define SEMIHOST_SYS_ERRNO    0x13

#define SEMIHOST_SYS_GET_CMD  0x15
#define SEMIHOST_SYS_HEAPINFO 0x16

#define SEMIHOST_SYS_ELAPSED  0x30
#define SEMIHOST_SYS_TICKFREQ 0x31

int do_semihosting(stlink_t *sl, uint32_t r0, uint32_t r1, uint32_t *ret);

#endif // _SEMIHOSTING_H_
