

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



/* FPEC flash controller interface, pm0063 manual
 */

#define FLASH_REGS_ADDR 0x40022000
#define FLASH_REGS_SIZE 0x28

#define FLASH_ACR (FLASH_REGS_ADDR + 0x00)
#define FLASH_KEYR (FLASH_REGS_ADDR + 0x04)
#define FLASH_SR (FLASH_REGS_ADDR + 0x0c)
#define FLASH_CR (FLASH_REGS_ADDR + 0x10)
#define FLASH_AR (FLASH_REGS_ADDR + 0x14)
#define FLASH_OBR (FLASH_REGS_ADDR + 0x1c)
#define FLASH_WRPR (FLASH_REGS_ADDR + 0x20)

#define FLASH_RDPTR_KEY 0x00a5
#define FLASH_KEY1 0x45670123
#define FLASH_KEY2 0xcdef89ab

#define FLASH_SR_BSY 0
#define FLASH_SR_EOP 5

#define FLASH_CR_PG 0
#define FLASH_CR_PER 1
#define FLASH_CR_MER 2
#define FLASH_CR_STRT 6
#define FLASH_CR_LOCK 7

void write_uint32(unsigned char* buf, uint32_t ui) {
    if (!is_bigendian()) { // le -> le (don't swap)
        buf[0] = ((unsigned char*) &ui)[0];
        buf[1] = ((unsigned char*) &ui)[1];
        buf[2] = ((unsigned char*) &ui)[2];
        buf[3] = ((unsigned char*) &ui)[3];
    } else {
        buf[0] = ((unsigned char*) &ui)[3];
        buf[1] = ((unsigned char*) &ui)[2];
        buf[2] = ((unsigned char*) &ui)[1];
        buf[3] = ((unsigned char*) &ui)[0];
    }
}

void write_uint16(unsigned char* buf, uint16_t ui) {
    if (!is_bigendian()) { // le -> le (don't swap)
        buf[0] = ((unsigned char*) &ui)[0];
        buf[1] = ((unsigned char*) &ui)[1];
    } else {
        buf[0] = ((unsigned char*) &ui)[1];
        buf[1] = ((unsigned char*) &ui)[0];
    }
}

uint32_t read_uint32(const unsigned char *c, const int pt) {
    uint32_t ui;
    char *p = (char *) &ui;

    if (!is_bigendian()) { // le -> le (don't swap)
        p[0] = c[pt];
        p[1] = c[pt + 1];
        p[2] = c[pt + 2];
        p[3] = c[pt + 3];
    } else {
        p[0] = c[pt + 3];
        p[1] = c[pt + 2];
        p[2] = c[pt + 1];
        p[3] = c[pt];
    }
    return ui;
}

static uint32_t __attribute__((unused)) read_flash_rdp(stlink_t *sl) {
    stlink_read_mem32(sl, FLASH_WRPR, sizeof (uint32_t));
    return (*(uint32_t*) sl->q_buf) & 0xff;
}

static inline uint32_t read_flash_wrpr(stlink_t *sl) {
    stlink_read_mem32(sl, FLASH_WRPR, sizeof (uint32_t));
    return *(uint32_t*) sl->q_buf;
}

static inline uint32_t read_flash_obr(stlink_t *sl) {
    stlink_read_mem32(sl, FLASH_OBR, sizeof (uint32_t));
    return *(uint32_t*) sl->q_buf;
}

static inline uint32_t read_flash_cr(stlink_t *sl) {
    stlink_read_mem32(sl, FLASH_CR, sizeof (uint32_t));
    return *(uint32_t*) sl->q_buf;
}

static inline unsigned int is_flash_locked(stlink_t *sl) {
    /* return non zero for true */
    return read_flash_cr(sl) & (1 << FLASH_CR_LOCK);
}

static void unlock_flash(stlink_t *sl) {
    /* the unlock sequence consists of 2 write cycles where
       2 key values are written to the FLASH_KEYR register.
       an invalid sequence results in a definitive lock of
       the FPEC block until next reset.
     */

    write_uint32(sl->q_buf, FLASH_KEY1);
    stlink_write_mem32(sl, FLASH_KEYR, sizeof (uint32_t));

    write_uint32(sl->q_buf, FLASH_KEY2);
    stlink_write_mem32(sl, FLASH_KEYR, sizeof (uint32_t));
}

static int unlock_flash_if(stlink_t *sl) {
    /* unlock flash if already locked */

    if (is_flash_locked(sl)) {
        unlock_flash(sl);
        if (is_flash_locked(sl))
            return -1;
    }

    return 0;
}

static void lock_flash(stlink_t *sl) {
    /* write to 1 only. reset by hw at unlock sequence */

    const uint32_t n = read_flash_cr(sl) | (1 << FLASH_CR_LOCK);

    write_uint32(sl->q_buf, n);
    stlink_write_mem32(sl, FLASH_CR, sizeof (uint32_t));
}

static void set_flash_cr_pg(stlink_t *sl) {
    const uint32_t n = 1 << FLASH_CR_PG;
    write_uint32(sl->q_buf, n);
    stlink_write_mem32(sl, FLASH_CR, sizeof (uint32_t));
}

static void __attribute__((unused)) clear_flash_cr_pg(stlink_t *sl) {
    const uint32_t n = read_flash_cr(sl) & ~(1 << FLASH_CR_PG);
    write_uint32(sl->q_buf, n);
    stlink_write_mem32(sl, FLASH_CR, sizeof (uint32_t));
}

static void set_flash_cr_per(stlink_t *sl) {
    const uint32_t n = 1 << FLASH_CR_PER;
    write_uint32(sl->q_buf, n);
    stlink_write_mem32(sl, FLASH_CR, sizeof (uint32_t));
}

static void __attribute__((unused)) clear_flash_cr_per(stlink_t *sl) {
    const uint32_t n = read_flash_cr(sl) & ~(1 << FLASH_CR_PER);
    write_uint32(sl->q_buf, n);
    stlink_write_mem32(sl, FLASH_CR, sizeof (uint32_t));
}

static void set_flash_cr_mer(stlink_t *sl) {
    const uint32_t n = 1 << FLASH_CR_MER;
    write_uint32(sl->q_buf, n);
    stlink_write_mem32(sl, FLASH_CR, sizeof (uint32_t));
}

static void __attribute__((unused)) clear_flash_cr_mer(stlink_t *sl) {
    const uint32_t n = read_flash_cr(sl) & ~(1 << FLASH_CR_MER);
    write_uint32(sl->q_buf, n);
    stlink_write_mem32(sl, FLASH_CR, sizeof (uint32_t));
}

static void set_flash_cr_strt(stlink_t *sl) {
    /* assume come on the flash_cr_per path */
    const uint32_t n = (1 << FLASH_CR_PER) | (1 << FLASH_CR_STRT);
    write_uint32(sl->q_buf, n);
    stlink_write_mem32(sl, FLASH_CR, sizeof (uint32_t));
}

static inline uint32_t read_flash_acr(stlink_t *sl) {
    stlink_read_mem32(sl, FLASH_ACR, sizeof (uint32_t));
    return *(uint32_t*) sl->q_buf;
}

static inline uint32_t read_flash_sr(stlink_t *sl) {
    stlink_read_mem32(sl, FLASH_SR, sizeof (uint32_t));
    return *(uint32_t*) sl->q_buf;
}

static inline unsigned int is_flash_busy(stlink_t *sl) {
    return read_flash_sr(sl) & (1 << FLASH_SR_BSY);
}

static void wait_flash_busy(stlink_t *sl) {
    /* todo: add some delays here */
    while (is_flash_busy(sl))
        ;
}

static inline unsigned int is_flash_eop(stlink_t *sl) {
    return read_flash_sr(sl) & (1 << FLASH_SR_EOP);
}

static void __attribute__((unused)) clear_flash_sr_eop(stlink_t *sl) {
    const uint32_t n = read_flash_sr(sl) & ~(1 << FLASH_SR_EOP);
    write_uint32(sl->q_buf, n);
    stlink_write_mem32(sl, FLASH_SR, sizeof (uint32_t));
}

static void __attribute__((unused)) wait_flash_eop(stlink_t *sl) {
    /* todo: add some delays here */
    while (is_flash_eop(sl) == 0)
        ;
}

static inline void write_flash_ar(stlink_t *sl, uint32_t n) {
    write_uint32(sl->q_buf, n);
    stlink_write_mem32(sl, FLASH_AR, sizeof (uint32_t));
}

#if 0 /* todo */

static void disable_flash_read_protection(stlink_t *sl) {
    /* erase the option byte area */
    /* rdp = 0x00a5; */
    /* reset */
}
#endif /* todo */


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
    D(sl, "\n*** stlink_exit_dfu_mode ***\n");
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
    D(sl, "\n*** stlink_run ***\n");
    sl->backend->run(sl);
}

void stlink_status(stlink_t *sl) {
    D(sl, "\n*** stlink_status ***\n");
    sl->backend->status(sl);
    stlink_core_stat(sl);
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

void stlink_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    D(sl, "\n*** stlink_read_mem32 ***\n");
    if (len % 4 != 0) { // !!! never ever: fw gives just wrong values
        fprintf(stderr, "Error: Data length doesn't have a 32 bit alignment: +%d byte.\n",
                len % 4);
        return;
    }
    sl->backend->read_mem32(sl, addr, len);
}

void stlink_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len) {
    D(sl, "\n*** stlink_write_mem8 ***\n");
    sl->backend->write_mem8(sl, addr, len);
}

void stlink_read_all_regs(stlink_t *sl, reg *regp) {
    D(sl, "\n*** stlink_read_all_regs ***\n");
    sl->backend->read_all_regs(sl, regp);
}

void stlink_write_reg(stlink_t *sl, uint32_t reg, int idx) {
    D(sl, "\n*** stlink_write_reg\n");
    sl->backend->write_reg(sl, reg, idx);
}

void stlink_read_reg(stlink_t *sl, int r_idx, reg *regp) {
    D(sl, "\n*** stlink_read_reg\n");
    DD(sl, " (%d) ***\n", r_idx);

    if (r_idx > 20 || r_idx < 0) {
        fprintf(stderr, "Error: register index must be in [0..20]\n");
        return;
    }

    sl->backend->read_reg(sl, r_idx, regp);
}

unsigned int is_core_halted(stlink_t *sl) {
    /* return non zero if core is halted */
    stlink_status(sl);
    return sl->q_buf[0] == STLINK_CORE_HALTED;
}

void stlink_step(stlink_t *sl) {
    D(sl, "\n*** stlink_step ***\n");
    sl->backend->step(sl);
}

int stlink_current_mode(stlink_t *sl) {
    D(sl, "\n*** stlink_current_mode ***\n");
    int mode = sl->backend->current_mode(sl);
    stlink_print_data(sl);
    switch (mode) {
        case STLINK_DEV_DFU_MODE:
            DD(sl, "stlink mode: dfu\n");
            return mode;
        case STLINK_DEV_DEBUG_MODE:
            DD(sl, "stlink mode: debug (jtag or swd)\n");
            return mode;
        case STLINK_DEV_MASS_MODE:
            DD(sl, "stlink mode: mass\n");
            return mode;
    }
    DD(sl, "stlink mode: unknown!\n");
    return STLINK_DEV_UNKNOWN_MODE;
}




// End of delegates....  Common code below here...

// Endianness
// http://www.ibm.com/developerworks/aix/library/au-endianc/index.html
// const int i = 1;
// #define is_bigendian() ( (*(char*)&i) == 0 )

inline unsigned int is_bigendian(void) {
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

// same as above with entrypoint.

void stlink_run_at(stlink_t *sl, stm32_addr_t addr) {
    stlink_write_reg(sl, addr, 15); /* pc register */

    stlink_run(sl);

    while (is_core_halted(sl) == 0)
        usleep(3000000);
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

void stlink_print_data(stlink_t * sl) {
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

static void unmap_file(mapped_file_t * mf) {
    munmap((void*) mf->base, mf->len);
    mf->base = (unsigned char*) MAP_FAILED;
    mf->len = 0;
}

static int check_file(stlink_t* sl, mapped_file_t* mf, stm32_addr_t addr) {
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

int write_buffer_to_sram(stlink_t *sl, flash_loader_t* fl, const uint8_t* buf, size_t size) {
    /* write the buffer right after the loader */
    memcpy(sl->q_buf, buf, size);
    stlink_write_mem8(sl, fl->buf_addr, size);
    return 0;
}

int stlink_erase_flash_page(stlink_t *sl, stm32_addr_t page) {
    /* page an addr in the page to erase */

    /* wait for ongoing op to finish */
    wait_flash_busy(sl);

    /* unlock if locked */
    unlock_flash_if(sl);

    /* set the page erase bit */
    set_flash_cr_per(sl);

    /* select the page to erase */
    write_flash_ar(sl, page);

    /* start erase operation, reset by hw with bsy bit */
    set_flash_cr_strt(sl);

    /* wait for completion */
    wait_flash_busy(sl);

    /* relock the flash */
    lock_flash(sl);

    /* todo: verify the erased page */

    return 0;
}

int stlink_erase_flash_mass(stlink_t *sl) {
    /* wait for ongoing op to finish */
    wait_flash_busy(sl);

    /* unlock if locked */
    unlock_flash_if(sl);

    /* set the mass erase bit */
    set_flash_cr_mer(sl);

    /* start erase operation, reset by hw with bsy bit */
    set_flash_cr_strt(sl);

    /* wait for completion */
    wait_flash_busy(sl);

    /* relock the flash */
    lock_flash(sl);

    /* todo: verify the erased memory */

    return 0;
}

int init_flash_loader(stlink_t *sl, flash_loader_t* fl) {
    size_t size;

    /* allocate the loader in sram */
    if (write_loader_to_sram(sl, &fl->loader_addr, &size) == -1) {
        fprintf(stderr, "write_loader_to_sram() == -1\n");
        return -1;
    }

    /* allocate a one page buffer in sram right after loader */
    fl->buf_addr = fl->loader_addr + size;

    return 0;
}

int write_loader_to_sram(stlink_t *sl, stm32_addr_t* addr, size_t* size) {
    /* from openocd, contrib/loaders/flash/stm32.s */
    static const uint8_t loader_code[] = {
        0x08, 0x4c, /* ldr	r4, STM32_FLASH_BASE */
        0x1c, 0x44, /* add	r4, r3 */
        /* write_half_word: */
        0x01, 0x23, /* movs	r3, #0x01 */
        0x23, 0x61, /* str	r3, [r4, #STM32_FLASH_CR_OFFSET] */
        0x30, 0xf8, 0x02, 0x3b, /* ldrh	r3, [r0], #0x02 */
        0x21, 0xf8, 0x02, 0x3b, /* strh	r3, [r1], #0x02 */
        /* busy: */
        0xe3, 0x68, /* ldr	r3, [r4, #STM32_FLASH_SR_OFFSET] */
        0x13, 0xf0, 0x01, 0x0f, /* tst	r3, #0x01 */
        0xfb, 0xd0, /* beq	busy */
        0x13, 0xf0, 0x14, 0x0f, /* tst	r3, #0x14 */
        0x01, 0xd1, /* bne	exit */
        0x01, 0x3a, /* subs	r2, r2, #0x01 */
        0xf0, 0xd1, /* bne	write_half_word */
        /* exit: */
        0x00, 0xbe, /* bkpt	#0x00 */
        0x00, 0x20, 0x02, 0x40, /* STM32_FLASH_BASE: .word 0x40022000 */
    };

    memcpy(sl->q_buf, loader_code, sizeof (loader_code));
    stlink_write_mem32(sl, sl->sram_base, sizeof (loader_code));

    *addr = sl->sram_base;
    *size = sizeof (loader_code);

    /* success */
    return 0;
}

int stlink_fcheck_flash(stlink_t *sl, const char* path, stm32_addr_t addr) {
    /* check the contents of path are at addr */

    int res;
    mapped_file_t mf = MAPPED_FILE_INITIALIZER;

    if (map_file(&mf, path) == -1)
        return -1;

    res = check_file(sl, &mf, addr);

    unmap_file(&mf);

    return res;
}

// The stlink_fwrite_flash should not muck with mmapped files inside itself,
// and should use this function instead. (Hell, what's the reason behind mmap
// there?!) But, as it is not actually used anywhere, nobody cares.

#define WRITE_BLOCK_SIZE 0x40

int stlink_write_flash(stlink_t *sl, stm32_addr_t addr, uint8_t* base, unsigned len) {
    size_t off;
    flash_loader_t fl;

    /* check addr range is inside the flash */
    if (addr < sl->flash_base) {
        fprintf(stderr, "addr too low\n");
        return -1;
    } else if ((addr + len) < addr) {
        fprintf(stderr, "addr overruns\n");
        return -1;
    } else if ((addr + len) > (sl->flash_base + sl->flash_size)) {
        fprintf(stderr, "addr too high\n");
        return -1;
    } else if ((addr & 1) || (len & 1)) {
        fprintf(stderr, "unaligned addr or size\n");
        return -1;
    }

    /* flash loader initialization */
    if (init_flash_loader(sl, &fl) == -1) {
        fprintf(stderr, "init_flash_loader() == -1\n");
        return -1;
    }

    /* write each page. above WRITE_BLOCK_SIZE fails? */
    for (off = 0; off < len; off += WRITE_BLOCK_SIZE) {
        /* adjust last write size */
        size_t size = WRITE_BLOCK_SIZE;
        if ((off + WRITE_BLOCK_SIZE) > len)
            size = len - off;

        if (run_flash_loader(sl, &fl, addr + off, base + off, size) == -1) {
            fprintf(stderr, "run_flash_loader(0x%zx) == -1\n", addr + off);
            return -1;
        }
    }

    for (off = 0; off < len; off += sl->flash_pgsz) {
        size_t aligned_size;

        /* adjust last page size */
        size_t cmp_size = sl->flash_pgsz;
        if ((off + sl->flash_pgsz) > len)
            cmp_size = len - off;

        aligned_size = cmp_size;
        if (aligned_size & (4 - 1))
            aligned_size = (cmp_size + 4) & ~(4 - 1);

        stlink_read_mem32(sl, addr + off, aligned_size);

        if (memcmp(sl->q_buf, base + off, cmp_size))
            return -1;
    }

    return 0;
}

int stlink_fwrite_flash(stlink_t *sl, const char* path, stm32_addr_t addr) {
    /* write the file in flash at addr */

    int error = -1;
    size_t off;
    mapped_file_t mf = MAPPED_FILE_INITIALIZER;
    flash_loader_t fl;

    if (map_file(&mf, path) == -1) {
        fprintf(stderr, "map_file() == -1\n");
        return -1;
    }

    /* check addr range is inside the flash */
    if (addr < sl->flash_base) {
        fprintf(stderr, "addr too low\n");
        goto on_error;
    } else if ((addr + mf.len) < addr) {
        fprintf(stderr, "addr overruns\n");
        goto on_error;
    } else if ((addr + mf.len) > (sl->flash_base + sl->flash_size)) {
        fprintf(stderr, "addr too high\n");
        goto on_error;
    } else if ((addr & 1) || (mf.len & 1)) {
        /* todo */
        fprintf(stderr, "unaligned addr or size\n");
        goto on_error;
    }

    /* erase each page. todo: mass erase faster? */
    for (off = 0; off < mf.len; off += sl->flash_pgsz) {
        /* addr must be an addr inside the page */
        if (stlink_erase_flash_page(sl, addr + off) == -1) {
            fprintf(stderr, "erase_flash_page(0x%zx) == -1\n", addr + off);
            goto on_error;
        }
    }

    /* flash loader initialization */
    if (init_flash_loader(sl, &fl) == -1) {
        fprintf(stderr, "init_flash_loader() == -1\n");
        goto on_error;
    }

    /* write each page. above WRITE_BLOCK_SIZE fails? */
#define WRITE_BLOCK_SIZE 0x40
    for (off = 0; off < mf.len; off += WRITE_BLOCK_SIZE) {
        /* adjust last write size */
        size_t size = WRITE_BLOCK_SIZE;
        if ((off + WRITE_BLOCK_SIZE) > mf.len)
            size = mf.len - off;

        if (run_flash_loader(sl, &fl, addr + off, mf.base + off, size) == -1) {
            fprintf(stderr, "run_flash_loader(0x%zx) == -1\n", addr + off);
            goto on_error;
        }
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

int run_flash_loader(stlink_t *sl, flash_loader_t* fl, stm32_addr_t target, const uint8_t* buf, size_t size) {
    const size_t count = size / sizeof (uint16_t);

    if (write_buffer_to_sram(sl, fl, buf, size) == -1) {
        fprintf(stderr, "write_buffer_to_sram() == -1\n");
        return -1;
    }

    /* setup core */
    stlink_write_reg(sl, fl->buf_addr, 0); /* source */
    stlink_write_reg(sl, target, 1); /* target */
    stlink_write_reg(sl, count, 2); /* count (16 bits half words) */
    stlink_write_reg(sl, 0, 3); /* flash bank 0 (input) */
    stlink_write_reg(sl, fl->loader_addr, 15); /* pc register */

    /* unlock and set programming mode */
    unlock_flash_if(sl);
    set_flash_cr_pg(sl);

    /* run loader */
    stlink_run(sl);

    while (is_core_halted(sl) == 0)
        ;

    lock_flash(sl);

    /* not all bytes have been written */
    reg rr;
    stlink_read_reg(sl, 2, &rr);
    if (rr.r[2] != 0) {
        fprintf(stderr, "write error, count == %u\n", rr.r[2]);
        return -1;
    }

    return 0;
}