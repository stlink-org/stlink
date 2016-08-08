#include <stdio.h>
#include <string.h>

#include "semihosting.h"

#include <stlink.h>
#include <stlink/logging.h>

static int mem_read_u8(stlink_t *sl, uint32_t addr, uint8_t *data)
{
    int offset = addr % 4;
    int len = 4;

    if (sl == NULL || data == NULL) {
        return -1;
    }

    /* Read address and length must be aligned */
    if (stlink_read_mem32(sl, addr - offset, len) != 0) {
        return -1;
    }

    *data = sl->q_buf[offset];
    return 0;
}

#ifdef UNUSED
static int mem_read_u16(stlink_t *sl, uint32_t addr, uint16_t *data)
{
    int offset = addr % 4;
    int len = (offset > 2 ? 8 : 4);

    if (sl == NULL || data == NULL) {
        return -1;
    }

    /* Read address and length must be aligned */
    if (stlink_read_mem32(sl, addr - offset, len) != 0) {
        return -1;
    }

    memcpy(data, &sl->q_buf[offset], sizeof(*data));
    return 0;
}

static int mem_read_u32(stlink_t *sl, uint32_t addr, uint32_t *data)
{
    int offset = addr % 4;
    int len = (offset > 0 ? 8 : 4);

    if (sl == NULL || data == NULL) {
        return -1;
    }

    /* Read address and length must be aligned */
    if (stlink_read_mem32(sl, addr - offset, len) != 0) {
        return -1;
    }

    memcpy(data, &sl->q_buf[offset], sizeof(*data));
    return 0;
}
#endif

static int mem_read(stlink_t *sl, uint32_t addr, uint8_t *data, uint16_t len)
{
    int offset = addr % 4;
    int read_len = len + offset;

    if (sl == NULL || data == NULL) {
        return -1;
    }

    /* Align read size */
    if ((read_len % 4) != 0) {
        read_len += 4 - (read_len % 4);
    }

    /* Read address and length must be aligned */
    if (stlink_read_mem32(sl, addr - offset, read_len) != 0) {
        return -1;
    }

    memcpy(data, &sl->q_buf[offset], len);
    return 0;
}

#define WRITE0_BUFFER_SIZE 64

int do_semihosting (stlink_t *sl, uint32_t r0, uint32_t r1, uint32_t *ret) {

    if (sl == NULL || ret == NULL) {
        return -1;
    }

    switch (r0) {
    case SYS_WRITEC:
    {
        uint8_t c;
        if (mem_read_u8(sl, r1, &c) == 0) {
            fprintf(stderr, "%c", c);
        }
        break;
    }
    case SYS_WRITE0:
    {
        uint8_t buf[WRITE0_BUFFER_SIZE];

        while (true) {
            if (mem_read(sl, r1, buf, WRITE0_BUFFER_SIZE) != 0 ) {
                return -1;
            }
            for (int i = 0; i < WRITE0_BUFFER_SIZE; i++) {
                if (buf[i] == 0) {
                    return 0;
                }
                fprintf(stderr, "%c", buf[i]);
            }
            r1 += WRITE0_BUFFER_SIZE;
        }
        break;
    }
    default:
        fprintf(stderr, "semihosting: unsupported call 0x%#x\n", r0);
        return -1;
    }
    return 0;
}
