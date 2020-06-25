#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <stlink.h>
#include <logging.h>
#include "semihosting.h"

static int mem_read_u8(stlink_t *sl, uint32_t addr, uint8_t *data) {
    int offset = addr % 4;
    int len = 4;

    if (sl == NULL || data == NULL) { return(-1); }

    // read address and length must be aligned
    if (stlink_read_mem32(sl, addr - offset, len) != 0) { return(-1); }

    *data = sl->q_buf[offset];
    return(0);
}

#ifdef UNUSED
static int mem_read_u16(stlink_t *sl, uint32_t addr, uint16_t *data) {
    int offset = addr % 4;
    int len = (offset > 2 ? 8 : 4);

    if (sl == NULL || data == NULL) { return(-1); }

    // read address and length must be aligned
    if (stlink_read_mem32(sl, addr - offset, len) != 0) { return(-1); }

    memcpy(data, &sl->q_buf[offset], sizeof(*data));
    return(0);
}

static int mem_read_u32(stlink_t *sl, uint32_t addr, uint32_t *data) {
    int offset = addr % 4;
    int len = (offset > 0 ? 8 : 4);

    if (sl == NULL || data == NULL) { return(-1); }

    // read address and length must be aligned
    if (stlink_read_mem32(sl, addr - offset, len) != 0) { return(-1); }

    memcpy(data, &sl->q_buf[offset], sizeof(*data));
    return(0);
}
#endif

static int mem_read(stlink_t *sl, uint32_t addr, void *data, uint16_t len) {
    int offset = addr % 4;
    int read_len = len + offset;

    if (sl == NULL || data == NULL) { return(-1); }

    // align read size
    if ((read_len % 4) != 0) { read_len += 4 - (read_len % 4); }

    // address and length must be aligned
    if (stlink_read_mem32(sl, addr - offset, read_len) != 0) { return(-1); }

    memcpy(data, &sl->q_buf[offset], len);
    return(0);
}

static int mem_write(stlink_t *sl, uint32_t addr, void *data, uint16_t len) {
    /* Note: this function can write more than it is asked to!
     * If addr is not an even 32 bit boundary, or len is not a multiple of 4.
     * If only 32 bit values can be written to the target, then this function should read
     * the target memory at the start and end of the buffer where it will write more that
     * the requested bytes. (perhaps reading the whole area is faster??).
     * If 16 and 8 bit writes are available, then they could be used instead.
     * Just return when the length is zero avoiding unneeded work. */
    if (len == 0) { return(0); }

    int offset = addr % 4;
    int write_len = len + offset;

    if (sl == NULL || data == NULL) { return(-1); }

    // align read size
    if ((write_len % 4) != 0) { write_len += 4 - (write_len % 4); }

    memcpy(&sl->q_buf[offset], data, len);

    // address and length must be aligned
    if (stlink_write_mem32(sl, addr - offset, write_len) != 0) { return(-1); }

    return(0);
}

/* For the SYS_WRITE0 call, we don't know the size of the null-terminated buffer
 * in the target memory. Instead of reading one byte at a time, we read by
 * chunks of WRITE0_BUFFER_SIZE bytes.
 */
#define WRITE0_BUFFER_SIZE 64

/* Define a maximum size for buffers transmitted by semihosting. There is no
 * limit in the ARM specification but this is a safety net.
 * We remove 4 byte from Q_BUF_LEN to handle alignment correction.
 */
#define MAX_BUFFER_SIZE (Q_BUF_LEN - 4)

/* Flags for Open syscall */

#ifndef O_BINARY
#define O_BINARY 0
#endif

static int open_mode_flags[12] = {
    O_RDONLY,
    O_RDONLY | O_BINARY,
    O_RDWR,
    O_RDWR   | O_BINARY,
    O_WRONLY | O_CREAT | O_TRUNC,
    O_WRONLY | O_CREAT | O_TRUNC  | O_BINARY,
    O_RDWR   | O_CREAT | O_TRUNC,
    O_RDWR   | O_CREAT | O_TRUNC  | O_BINARY,
    O_WRONLY | O_CREAT | O_APPEND,
    O_WRONLY | O_CREAT | O_APPEND | O_BINARY,
    O_RDWR   | O_CREAT | O_APPEND,
    O_RDWR   | O_CREAT | O_APPEND | O_BINARY
};

static int saved_errno = 0;

int do_semihosting (stlink_t *sl, uint32_t r0, uint32_t r1, uint32_t *ret) {

    if (sl == NULL || ret == NULL) { return(-1); }

    DLOG("Do semihosting R0=0x%08x R1=0x%08x\n", r0, r1);

    switch (r0) {
    case SEMIHOST_SYS_OPEN:
    {
        uint32_t args[3];
        uint32_t name_address;
        uint32_t mode;
        uint32_t name_len;
        char     *name;

        if (mem_read(sl, r1, args, sizeof(args)) != 0) {
            DLOG("Semihosting SYS_OPEN error: cannot read args from target memory\n");
            *ret = -1;
            return(-1);
        }

        name_address = args[0];
        mode         = args[1];
        name_len     = args[2];

        if (mode > 12) {
            /* Invalid mode */
            DLOG("Semihosting SYS_OPEN error: invalid mode %d\n", mode);
            *ret = -1;
            return(-1);
        }

        /* Add the trailing zero that is not counted in the length argument (see
         * ARM semihosting specification)
         */
        name_len += 1;

        if (name_len > MAX_BUFFER_SIZE) {
            DLOG("Semihosting SYS_OPEN error: name buffer size is too big %d\n", name_len);
            *ret = -1;
            return(-1);
        }

        name = malloc(name_len);

        if (name == NULL) {
            DLOG("Semihosting SYS_OPEN error: cannot allocate name buffer\n");
            *ret = -1;
            return(-1);
        }

        if (mem_read(sl, name_address, name, name_len) != 0) {
            free(name);
            *ret = -1;
            DLOG("Semihosting SYS_OPEN error: cannot read name from target memory\n");
            return(-1);
        }

        DLOG("Semihosting: open('%s', (SH open mode)%d, 0644)\n", name, mode);

        *ret = (uint32_t)open(name, open_mode_flags[mode], 0644);
        saved_errno = errno;

        DLOG("Semihosting: return %d\n", *ret);

        free(name);
        break;
    }
    case SEMIHOST_SYS_CLOSE:
    {
        uint32_t args[1];
        int fd;

        if (mem_read(sl, r1, args, sizeof(args)) != 0) {
            DLOG("Semihosting SYS_CLOSE error: cannot read args from target memory\n");
            *ret = -1;
            return(-1);
        }

        fd = (int)args[0];

        DLOG("Semihosting: close(%d)\n", fd);

        *ret = (uint32_t)close(fd);
        saved_errno = errno;

        DLOG("Semihosting: return %d\n", *ret);
        break;
    }
    case SEMIHOST_SYS_WRITE:
    {
        uint32_t args[3];
        uint32_t buffer_address;
        int fd;
        uint32_t buffer_len;
        void    *buffer;

        if (mem_read(sl, r1, args, sizeof(args)) != 0) {
            DLOG("Semihosting SYS_WRITE error: cannot read args from target memory\n");
            *ret = -1;
            return(-1);
        }

        fd             = (int)args[0];
        buffer_address = args[1];
        buffer_len     = args[2];

        if (buffer_len > MAX_BUFFER_SIZE) {
            DLOG("Semihosting SYS_WRITE error: buffer size is too big %d\n",
                 buffer_len);
            *ret = buffer_len;
            return(-1);
        }

        buffer = malloc(buffer_len);

        if (buffer == NULL) {
            DLOG("Semihosting SYS_WRITE error: cannot allocate buffer\n");
            *ret = buffer_len;
            return(-1);
        }

        if (mem_read(sl, buffer_address, buffer, buffer_len) != 0) {
            DLOG("Semihosting SYS_WRITE error: cannot read buffer from target memory\n");
            free(buffer);
            *ret = buffer_len;
            return(-1);
        }

        DLOG("Semihosting: write(%d, target_addr:0x%08x, %zu)\n", fd, buffer_address,
             buffer_len);

        *ret = (uint32_t)write(fd, buffer, buffer_len);
        saved_errno = errno;

        if (*ret == (uint32_t)-1) {
            *ret = buffer_len;
        } else {
            *ret -= buffer_len;
        }

        DLOG("Semihosting: return %d\n", *ret);
        free(buffer);
        break;
    }
    case SEMIHOST_SYS_READ:
    {
        uint32_t args[3];
        uint32_t buffer_address;
        int fd;
        uint32_t buffer_len;
        void    *buffer;
        ssize_t read_result;

        if (mem_read(sl, r1, args, sizeof(args)) != 0) {
            DLOG("Semihosting SYS_READ error: cannot read args from target memory\n");
            *ret = -1;
            return(-1);
        }

        fd             = (int)args[0];
        buffer_address = args[1];
        buffer_len     = args[2];

        if (buffer_len > MAX_BUFFER_SIZE) {
            DLOG("Semihosting SYS_READ error: buffer size is too big %d\n", buffer_len);
            *ret = buffer_len;
            return(-1);
        }

        buffer = malloc(buffer_len);

        if (buffer == NULL) {
            DLOG("Semihosting SYS_READ error: cannot allocatebuffer\n");
            *ret = buffer_len;
            return(-1);
        }

        DLOG("Semihosting: read(%d, target_addr:0x%08x, %zu)\n", fd, buffer_address,
             buffer_len);

        read_result = read(fd, buffer, buffer_len);
        saved_errno = errno;

        if (read_result == -1) {
            *ret = buffer_len;
        } else {
            if (mem_write(sl, buffer_address, buffer, read_result) != 0) {
                DLOG("Semihosting SYS_READ error: cannot write buffer to target memory\n");
                free(buffer);
                *ret = buffer_len;
                return(-1);
            } else {
                *ret = buffer_len - (uint32_t)read_result;
            }
        }

        DLOG("Semihosting: return %d\n", *ret);
        free(buffer);
        break;
    }
    case SEMIHOST_SYS_ERRNO:
    {
        *ret = (uint32_t)saved_errno;
        DLOG("Semihosting: Errno return %d\n", *ret);
        break;
    }
    case SEMIHOST_SYS_REMOVE:
    {
        uint32_t args[2];
        uint32_t name_address;
        uint32_t name_len;
        char     *name;

        if (mem_read(sl, r1, args, sizeof(args)) != 0) {
            DLOG("Semihosting SYS_REMOVE error: cannot read args from target memory\n");
            *ret = -1;
            return(-1);
        }

        name_address = args[0];
        name_len     = args[1];

        /* Add the trailing zero that is not counted in the length argument (see
         * ARM semihosting specification)
         */
        name_len += 1;

        if (name_len > MAX_BUFFER_SIZE) {
            DLOG("Semihosting SYS_REMOVE error: name buffer size is too big %d\n",
                 name_len);
            *ret = -1;
            return(-1);
        }

        name = malloc(name_len);

        if (name == NULL) {
            DLOG("Semihosting SYS_REMOVE error: cannot allocate name buffer\n");
            *ret = -1;
            return(-1);
        }

        if (mem_read(sl, name_address, name, name_len) != 0) {
            free(name);
            *ret = -1;
            DLOG("Semihosting SYS_REMOVE error: cannot read name from target memory\n");
            return(-1);
        }

        DLOG("Semihosting: unlink('%s')\n", name);
        *ret = (uint32_t)unlink(name);
        saved_errno = errno;
        DLOG("Semihosting: return %d\n", *ret);
        free(name);
        break;
    }
    case SEMIHOST_SYS_SEEK:
    {
        uint32_t args[2];
        int fd;
        off_t offset;

        if (mem_read(sl, r1, args, sizeof(args)) != 0) {
            DLOG("Semihosting SYS_SEEK error: cannot read args from target memory\n");
            *ret = -1;
            return(-1);
        }

        fd = (int)args[0];
        offset = (off_t)args[1];

        DLOG("Semihosting: lseek(%d, %d, SEEK_SET)\n", fd, offset);
        *ret = (uint32_t)lseek(fd, offset, SEEK_SET);
        saved_errno = errno;

        if (*ret != (uint32_t)-1) { *ret = 0; /* Success */ }

        DLOG("Semihosting: return %d\n", *ret);
        break;
    }
    case SEMIHOST_SYS_WRITEC:
    {
        uint8_t c;

        if (mem_read_u8(sl, r1, &c) == 0) {
            fprintf(stderr, "%c", c);
        } else {
            DLOG("Semihosting WRITEC: cannot read target memory at 0x%08x\n", r1);
        }

        break;
    }
    case SEMIHOST_SYS_READC:
    {
        uint8_t c = getchar();
        *ret = c;
        break;
    }
    case SEMIHOST_SYS_WRITE0:
    {
        uint8_t buf[WRITE0_BUFFER_SIZE];

        while (true) {
            if (mem_read(sl, r1, buf, WRITE0_BUFFER_SIZE) != 0) {
                DLOG("Semihosting WRITE0: cannot read target memory at 0x%08x\n", r1);
                return(-1);
            }

            for (int i = 0; i < WRITE0_BUFFER_SIZE; i++) {
                if (buf[i] == 0) { return(0); }

                fprintf(stderr, "%c", buf[i]);
            }

            r1 += WRITE0_BUFFER_SIZE;
        }

        break;
    }
    default:
        fprintf(stderr, "semihosting: unsupported call %#x\n", r0);
        return(-1);
    }
    return(0);
}
