

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>


#include "stlink-common.h"

void D(stlink_t *sl, char *txt) {
    if (sl->verbose > 1)
        fputs(txt, stderr);
}

void DD(stlink_t *sl, char *format, ...) {
    if (sl->verbose > 0) {
        va_list list;
        va_start(list, format);
        vfprintf(stderr, format, list);
        va_end(list);
    }
}

// Delegates to the backends...

void stlink_close(stlink_t *sl) {
    D(sl, "\n*** stlink_close ***\n");
    sl->backend->close(sl);

    free(sl);
}

void stlink_exit_debug_mode(stlink_t *sl) {
    D(sl, "\n*** stlink_exit_debug_mode ***\n");
    sl->backend->exit_debug_mode(sl);
}

void stlink_enter_swd_mode(stlink_t *sl) {
    D(sl, "\n*** stlink_enter_swd_mode ***\n");
    sl->backend->enter_swd_mode(sl);
}

void stlink_exit_dfu_mode(stlink_t *sl) {
    D(sl, "\n*** stlink_exit_duf_mode ***\n");
    sl->backend->exit_dfu_mode(sl);
}

void stlink_core_id(stlink_t *sl) {
    D(sl, "\n*** stlink_core_id ***\n");
    sl->backend->core_id(sl);
    DD(sl, "core_id = 0x%08x\n", sl->core_id);
}

void stlink_reset(stlink_t *sl) {
    D(sl, "\n*** stlink_reset ***\n");
    sl->backend->reset(sl);

}

void stlink_run(stlink_t *sl) {
    D(sl, "\n*** stlink_core_id ***\n");
    sl->backend->run(sl);
    DD(sl, "core_id = 0x%08x\n", sl->core_id);
}

void stlink_status(stlink_t *sl) {
    D(sl, "\n*** stlink_core_id ***\n");
    sl->backend->status(sl);
    stlink_core_stat(sl);
    DD(sl, "core_id = 0x%08x\n", sl->core_id);
}

void stlink_version(stlink_t *sl) {
    D(sl, "*** looking up stlink version\n");
    sl->backend->version(sl);
}

void stlink_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    D(sl, "\n*** stlink_write_mem32 ***\n");
    if (len % 4 != 0) {
        fprintf(stderr, "Error: Data length doesn't have a 32 bit alignment: +%d byte.\n", len % 4);
        return;
    }
    sl->backend->write_mem32(sl, addr, len);
}

void stlink_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len) {
    D(sl, "\n*** stlink_write_mem8 ***\n");
    sl->backend->write_mem8(sl, addr, len);
}






// End of delegates....  Common code below here...

// Endianness
// http://www.ibm.com/developerworks/aix/library/au-endianc/index.html
// const int i = 1;
// #define is_bigendian() ( (*(char*)&i) == 0 )

static inline unsigned int is_bigendian(void) {
    static volatile const unsigned int i = 1;
    return *(volatile const char*) &i == 0;
}

uint16_t read_uint16(const unsigned char *c, const int pt) {
    uint32_t ui;
    char *p = (char *) &ui;

    if (!is_bigendian()) { // le -> le (don't swap)
        p[0] = c[pt];
        p[1] = c[pt + 1];
    } else {
        p[0] = c[pt + 1];
        p[1] = c[pt];
    }
    return ui;
}

void stlink_core_stat(stlink_t *sl) {
    if (sl->q_len <= 0)
        return;

    stlink_print_data(sl);

    switch (sl->q_buf[0]) {
        case STLINK_CORE_RUNNING:
            sl->core_stat = STLINK_CORE_RUNNING;
            DD(sl, "  core status: running\n");
            return;
        case STLINK_CORE_HALTED:
            sl->core_stat = STLINK_CORE_HALTED;
            DD(sl, "  core status: halted\n");
            return;
        default:
            sl->core_stat = STLINK_CORE_STAT_UNKNOWN;
            fprintf(stderr, "  core status: unknown\n");
    }
}

void stlink_print_data(stlink_t *sl) {
    if (sl->q_len <= 0 || sl->verbose < 2)
        return;
    if (sl->verbose > 2)
        fprintf(stdout, "data_len = %d 0x%x\n", sl->q_len, sl->q_len);

    for (int i = 0; i < sl->q_len; i++) {
        if (i % 16 == 0) {
            /*
                                    if (sl->q_data_dir == Q_DATA_OUT)
                                            fprintf(stdout, "\n<- 0x%08x ", sl->q_addr + i);
                                    else
                                            fprintf(stdout, "\n-> 0x%08x ", sl->q_addr + i);
             */
        }
        fprintf(stdout, " %02x", (unsigned int) sl->q_buf[i]);
    }
    fputs("\n\n", stdout);
}

/* memory mapped file */

typedef struct mapped_file {
    uint8_t* base;
    size_t len;
} mapped_file_t;

#define MAPPED_FILE_INITIALIZER { NULL, 0 }

static int map_file(mapped_file_t* mf, const char* path) {
    int error = -1;
    struct stat st;

    const int fd = open(path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "open(%s) == -1\n", path);
        return -1;
    }

    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "fstat() == -1\n");
        goto on_error;
    }

    mf->base = (uint8_t*) mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (mf->base == MAP_FAILED) {
        fprintf(stderr, "mmap() == MAP_FAILED\n");
        goto on_error;
    }

    mf->len = st.st_size;

    /* success */
    error = 0;

on_error:
    close(fd);

    return error;
}

static void unmap_file(mapped_file_t* mf) {
    munmap((void*) mf->base, mf->len);
    mf->base = (unsigned char*) MAP_FAILED;
    mf->len = 0;
}

static int check_file
(stlink_t* sl, mapped_file_t* mf, stm32_addr_t addr) {
    size_t off;

    for (off = 0; off < mf->len; off += sl->flash_pgsz) {
        size_t aligned_size;

        /* adjust last page size */
        size_t cmp_size = sl->flash_pgsz;
        if ((off + sl->flash_pgsz) > mf->len)
            cmp_size = mf->len - off;

        aligned_size = cmp_size;
        if (aligned_size & (4 - 1))
            aligned_size = (cmp_size + 4) & ~(4 - 1);

        stlink_read_mem32(sl, addr + off, aligned_size);

        if (memcmp(sl->q_buf, mf->base + off, cmp_size))
            return -1;
    }

    return 0;
}

int stlink_fwrite_sram
(stlink_t * sl, const char* path, stm32_addr_t addr) {
    /* write the file in sram at addr */

    int error = -1;
    size_t off;
    mapped_file_t mf = MAPPED_FILE_INITIALIZER;

    if (map_file(&mf, path) == -1) {
        fprintf(stderr, "map_file() == -1\n");
        return -1;
    }

    /* check addr range is inside the sram */
    if (addr < sl->sram_base) {
        fprintf(stderr, "addr too low\n");
        goto on_error;
    } else if ((addr + mf.len) < addr) {
        fprintf(stderr, "addr overruns\n");
        goto on_error;
    } else if ((addr + mf.len) > (sl->sram_base + sl->sram_size)) {
        fprintf(stderr, "addr too high\n");
        goto on_error;
    } else if ((addr & 3) || (mf.len & 3)) {
        /* todo */
        fprintf(stderr, "unaligned addr or size\n");
        goto on_error;
    }

    /* do the copy by 1k blocks */
    for (off = 0; off < mf.len; off += 1024) {
        size_t size = 1024;
        if ((off + size) > mf.len)
            size = mf.len - off;

        memcpy(sl->q_buf, mf.base + off, size);

        /* round size if needed */
        if (size & 3)
            size += 2;

        stlink_write_mem32(sl, addr + off, size);
    }

    /* check the file ha been written */
    if (check_file(sl, &mf, addr) == -1) {
        fprintf(stderr, "check_file() == -1\n");
        goto on_error;
    }

    /* success */
    error = 0;

on_error:
    unmap_file(&mf);
    return error;
}

int stlink_fread(stlink_t* sl, const char* path, stm32_addr_t addr, size_t size) {
    /* read size bytes from addr to file */

    int error = -1;
    size_t off;

    const int fd = open(path, O_RDWR | O_TRUNC | O_CREAT, 00700);
    if (fd == -1) {
        fprintf(stderr, "open(%s) == -1\n", path);
        return -1;
    }

    /* do the copy by 1k blocks */
    for (off = 0; off < size; off += 1024) {
        size_t read_size = 1024;
        if ((off + read_size) > size)
            read_size = off + read_size;

        /* round size if needed */
        if (read_size & 3)
            read_size = (read_size + 4) & ~(3);

        stlink_read_mem32(sl, addr + off, read_size);

        if (write(fd, sl->q_buf, read_size) != (ssize_t) read_size) {
            fprintf(stderr, "write() != read_size\n");
            goto on_error;
        }
    }

    /* success */
    error = 0;

on_error:
    close(fd);

    return error;
}

typedef struct flash_loader {
    stm32_addr_t loader_addr; /* loader sram adddr */
    stm32_addr_t buf_addr; /* buffer sram address */
} flash_loader_t;

int write_buffer_to_sram
(stlink_t *sl, flash_loader_t* fl, const uint8_t* buf, size_t size) {
    /* write the buffer right after the loader */
    memcpy(sl->q_buf, buf, size);
    stlink_write_mem8(sl, fl->buf_addr, size);
    return 0;
}
