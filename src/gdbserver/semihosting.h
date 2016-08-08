#ifndef _SEMIHOSTING_H_
#define _SEMIHOSTING_H_

#include <stlink.h>

#define SYS_OPEN     0x01
#define SYS_CLOSE    0x02
#define SYS_WRITEC   0x03
#define SYS_WRITE0   0x04
#define SYS_WRITE    0x05
#define SYS_READ     0x06
#define SYS_READC    0x07
#define SYS_ISERROR  0x08
#define SYS_ISTTY    0x09
#define SYS_SEEK     0x0A

#define SYS_FLEN     0x0C
#define SYS_TMPNAM   0x0D
#define SYS_REMOVE   0x0E
#define SYS_RENAME   0x0E
#define SYS_CLOCK    0x10
#define SYS_TIME     0x11

#define SYS_ERRNO    0x13

#define SYS_GET_CMD  0x15
#define SYS_HEAPINFO 0x16

#define SYS_ELAPSED  0x30
#define SYS_TICKFREQ 0x31

int do_semihosting (stlink_t *sl, uint32_t r0, uint32_t r1, uint32_t *ret);

#endif /* ! _SEMIHOSTING_H_ */
