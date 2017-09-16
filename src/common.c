#define DEBUG_FLASH 0

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "stlink.h"
#include "stlink/mmap.h"
#include "stlink/logging.h"

#ifndef _WIN32
#define O_BINARY 0 //! @todo get rid of this OH MY (@xor-gate)
#endif
#ifdef _MSC_VER
#define __attribute__(x)
#endif

/* todo: stm32l15xxx flash memory, pm0062 manual */

/* stm32f FPEC flash controller interface, pm0063 manual */
// TODO - all of this needs to be abstracted out....
// STM32F05x is identical, based on RM0091 (DM00031936, Doc ID 018940 Rev 2, August 2012)
#define FLASH_REGS_ADDR 0x40022000
#define FLASH_REGS_SIZE 0x28

#define FLASH_ACR (FLASH_REGS_ADDR + 0x00)
#define FLASH_KEYR (FLASH_REGS_ADDR + 0x04)
#define FLASH_SR (FLASH_REGS_ADDR + 0x0c)
#define FLASH_CR (FLASH_REGS_ADDR + 0x10)
#define FLASH_AR (FLASH_REGS_ADDR + 0x14)
#define FLASH_OBR (FLASH_REGS_ADDR + 0x1c)
#define FLASH_WRPR (FLASH_REGS_ADDR + 0x20)

// STM32F10x_XL has two flash memory banks with separate registers to control the second bank.
#define FLASH_KEYR2 (FLASH_REGS_ADDR + 0x44)
#define FLASH_SR2 (FLASH_REGS_ADDR + 0x4c)
#define FLASH_CR2 (FLASH_REGS_ADDR + 0x50)
#define FLASH_AR2 (FLASH_REGS_ADDR + 0x54)
#define FLASH_BANK2_START_ADDR 0x08080000

// For STM32F05x, the RDPTR_KEY may be wrong, but as it is not used anywhere...
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


//32L = 32F1 same CoreID as 32F4!
#define STM32L_FLASH_REGS_ADDR ((uint32_t)0x40023c00)
#define STM32L_FLASH_ACR (STM32L_FLASH_REGS_ADDR + 0x00)
#define STM32L_FLASH_PECR (STM32L_FLASH_REGS_ADDR + 0x04)
#define STM32L_FLASH_PDKEYR (STM32L_FLASH_REGS_ADDR + 0x08)
#define STM32L_FLASH_PEKEYR (STM32L_FLASH_REGS_ADDR + 0x0c)
#define STM32L_FLASH_PRGKEYR (STM32L_FLASH_REGS_ADDR + 0x10)
#define STM32L_FLASH_OPTKEYR (STM32L_FLASH_REGS_ADDR + 0x14)
#define STM32L_FLASH_SR (STM32L_FLASH_REGS_ADDR + 0x18)
#define STM32L_FLASH_OBR (STM32L_FLASH_REGS_ADDR + 0x1c)
#define STM32L_FLASH_WRPR (STM32L_FLASH_REGS_ADDR + 0x20)
#define FLASH_L1_FPRG 10
#define FLASH_L1_PROG 3

//32L4 register base is at FLASH_REGS_ADDR (0x40022000)
#define STM32L4_FLASH_KEYR      (FLASH_REGS_ADDR + 0x08)
#define STM32L4_FLASH_SR        (FLASH_REGS_ADDR + 0x10)
#define STM32L4_FLASH_CR        (FLASH_REGS_ADDR + 0x14)
#define STM32L4_FLASH_OPTR      (FLASH_REGS_ADDR + 0x20)

#define STM32L4_FLASH_SR_BSY            16
#define STM32L4_FLASH_SR_ERRMASK        0x3f8 /* SR [9:3] */

#define STM32L4_FLASH_CR_LOCK   31      /* Lock control register */
#define STM32L4_FLASH_CR_PG     0       /* Program */
#define STM32L4_FLASH_CR_PER    1       /* Page erase */
#define STM32L4_FLASH_CR_MER1   2       /* Bank 1 erase */
#define STM32L4_FLASH_CR_MER2   15      /* Bank 2 erase */
#define STM32L4_FLASH_CR_STRT   16      /* Start command */
#define STM32L4_FLASH_CR_BKER   11      /* Bank select for page erase */
#define STM32L4_FLASH_CR_PNB    3       /* Page number (8 bits) */
// Bits requesting flash operations (useful when we want to clear them)
#define STM32L4_FLASH_CR_OPBITS                                     \
    ((1lu<<STM32L4_FLASH_CR_PG) | (1lu<<STM32L4_FLASH_CR_PER)       \
    | (1lu<<STM32L4_FLASH_CR_MER1) | (1lu<<STM32L4_FLASH_CR_MER1))
// Page is fully specified by BKER and PNB
#define STM32L4_FLASH_CR_PAGEMASK (0x1fflu << STM32L4_FLASH_CR_PNB)

#define STM32L4_FLASH_OPTR_DUALBANK     21

//STM32L0x flash register base and offsets
//same as 32L1 above
// RM0090 - DM00031020.pdf
#define STM32L0_FLASH_REGS_ADDR ((uint32_t)0x40022000)
#define FLASH_ACR_OFF     ((uint32_t) 0x00)
#define FLASH_PECR_OFF    ((uint32_t) 0x04)
#define FLASH_PDKEYR_OFF  ((uint32_t) 0x08)
#define FLASH_PEKEYR_OFF  ((uint32_t) 0x0c)
#define FLASH_PRGKEYR_OFF ((uint32_t) 0x10)
#define FLASH_OPTKEYR_OFF ((uint32_t) 0x14)
#define FLASH_SR_OFF      ((uint32_t) 0x18)
#define FLASH_OBR_OFF     ((uint32_t) 0x1c)
#define FLASH_WRPR_OFF    ((uint32_t) 0x20)



//STM32F4
#define FLASH_F4_REGS_ADDR ((uint32_t)0x40023c00)
#define FLASH_F4_KEYR (FLASH_F4_REGS_ADDR + 0x04)
#define FLASH_F4_OPT_KEYR (FLASH_F4_REGS_ADDR + 0x08)
#define FLASH_F4_SR (FLASH_F4_REGS_ADDR + 0x0c)
#define FLASH_F4_CR (FLASH_F4_REGS_ADDR + 0x10)
#define FLASH_F4_OPT_CR (FLASH_F4_REGS_ADDR + 0x14)
#define FLASH_F4_CR_STRT 16
#define FLASH_F4_CR_LOCK 31
#define FLASH_F4_CR_SER 1
#define FLASH_F4_CR_SNB 3
#define FLASH_F4_CR_SNB_MASK 0xf8
#define FLASH_F4_SR_BSY 16

#define L1_WRITE_BLOCK_SIZE 0x80
#define L0_WRITE_BLOCK_SIZE 0x40

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
        p[0] = c[pt + 0];
        p[1] = c[pt + 1];
        p[2] = c[pt + 2];
        p[3] = c[pt + 3];
    } else {
        p[0] = c[pt + 3];
        p[1] = c[pt + 2];
        p[2] = c[pt + 1];
        p[3] = c[pt + 0];
    }
    return ui;
}

static uint32_t __attribute__((unused)) read_flash_rdp(stlink_t *sl) {
    uint32_t rdp;
    stlink_read_debug32(sl, FLASH_WRPR, &rdp);
    return rdp & 0xff;
}

static inline uint32_t read_flash_cr(stlink_t *sl) {
    uint32_t reg, res;

    if (sl->flash_type == STLINK_FLASH_TYPE_F4)
        reg = FLASH_F4_CR;
    else if (sl->flash_type == STLINK_FLASH_TYPE_L4)
        reg = STM32L4_FLASH_CR;
    else
        reg = FLASH_CR;

    stlink_read_debug32(sl, reg, &res);

#if DEBUG_FLASH
    fprintf(stdout, "CR:0x%x\n", res);
#endif
    return res;
}

static inline uint32_t read_flash_cr2(stlink_t *sl) {
    uint32_t res;
    stlink_read_debug32(sl, FLASH_CR2, &res);
#if DEBUG_FLASH
    fprintf(stdout, "CR2:0x%x\n", res);
#endif
    return res;
}

static inline unsigned int is_flash_locked(stlink_t *sl) {
    /* return non zero for true */
    uint32_t cr_lock_shift, cr = read_flash_cr(sl);

    if (sl->flash_type == STLINK_FLASH_TYPE_F4)
        cr_lock_shift = FLASH_F4_CR_LOCK;
    else if (sl->flash_type == STLINK_FLASH_TYPE_L4)
        cr_lock_shift = STM32L4_FLASH_CR_LOCK;
    else
        cr_lock_shift = FLASH_CR_LOCK;

    return cr & (1 << cr_lock_shift);
}

static void unlock_flash(stlink_t *sl) {
    uint32_t key_reg;
    /* the unlock sequence consists of 2 write cycles where
       2 key values are written to the FLASH_KEYR register.
       an invalid sequence results in a definitive lock of
       the FPEC block until next reset.
       */
    if (sl->flash_type == STLINK_FLASH_TYPE_F4)
        key_reg = FLASH_F4_KEYR;
    else if (sl->flash_type == STLINK_FLASH_TYPE_L4)
        key_reg = STM32L4_FLASH_KEYR;
    else
        key_reg = FLASH_KEYR;

    stlink_write_debug32(sl, key_reg, FLASH_KEY1);
    stlink_write_debug32(sl, key_reg, FLASH_KEY2);

    if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
        stlink_write_debug32(sl, FLASH_KEYR2, FLASH_KEY1);
        stlink_write_debug32(sl, FLASH_KEYR2, FLASH_KEY2);
    }
}

static int unlock_flash_if(stlink_t *sl) {
    /* unlock flash if already locked */

    if (is_flash_locked(sl)) {
        unlock_flash(sl);
        if (is_flash_locked(sl)) {
            WLOG("Failed to unlock flash!\n");
            return -1;
        }
    }
    DLOG("Successfully unlocked flash\n");
    return 0;
}

static void lock_flash(stlink_t *sl) {
    uint32_t cr_lock_shift, cr_reg, n;

    if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
        cr_reg = FLASH_F4_CR;
        cr_lock_shift = FLASH_F4_CR_LOCK;
    } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
        cr_reg = STM32L4_FLASH_CR;
        cr_lock_shift = STM32L4_FLASH_CR_LOCK;
    } else {
        cr_reg = FLASH_CR;
        cr_lock_shift = FLASH_CR_LOCK;
    }

    n = read_flash_cr(sl) | (1 << cr_lock_shift);
    stlink_write_debug32(sl, cr_reg, n);

    if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
        n = read_flash_cr2(sl) | (1 << cr_lock_shift);
        stlink_write_debug32(sl, FLASH_CR2, n);
    }
}


static void set_flash_cr_pg(stlink_t *sl) {
    uint32_t cr_reg, x;

    x = read_flash_cr(sl);

    if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
        cr_reg = FLASH_F4_CR;
        x |= 1 << FLASH_CR_PG;
    } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
        cr_reg = STM32L4_FLASH_CR;
        x &= ~STM32L4_FLASH_CR_OPBITS;
        x |= 1 << STM32L4_FLASH_CR_PG;
    } else {
        cr_reg = FLASH_CR;
        x = 1 << FLASH_CR_PG;
    }

    stlink_write_debug32(sl, cr_reg, x);
}

static void __attribute__((unused)) clear_flash_cr_pg(stlink_t *sl) {
    uint32_t cr_reg, n;

    if (sl->flash_type == STLINK_FLASH_TYPE_F4)
        cr_reg = FLASH_F4_CR;
    else if (sl->flash_type == STLINK_FLASH_TYPE_L4)
        cr_reg = STM32L4_FLASH_CR;
    else
        cr_reg = FLASH_CR;

    n = read_flash_cr(sl) & ~(1 << FLASH_CR_PG);
    stlink_write_debug32(sl, cr_reg, n);
}

static void set_flash_cr_per(stlink_t *sl) {
    const uint32_t n = 1 << FLASH_CR_PER;
    stlink_write_debug32(sl, FLASH_CR, n);
}

static void set_flash_cr2_per(stlink_t *sl) {
    const uint32_t n = 1 << FLASH_CR_PER;
    stlink_write_debug32(sl, FLASH_CR2, n);
}

static void __attribute__((unused)) clear_flash_cr_per(stlink_t *sl) {
    const uint32_t n = read_flash_cr(sl) & ~(1 << FLASH_CR_PER);
    stlink_write_debug32(sl, FLASH_CR, n);
}

static void set_flash_cr_mer(stlink_t *sl, bool v) {
    uint32_t val, cr_reg, cr_mer, cr_pg;

    if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
        cr_reg = FLASH_F4_CR;
        cr_mer = 1 << FLASH_CR_MER;
        cr_pg = 1 << FLASH_CR_PG;
    } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
        cr_reg = STM32L4_FLASH_CR;
        cr_mer = (1 << STM32L4_FLASH_CR_MER1) | (1 << STM32L4_FLASH_CR_MER2);
        cr_pg = 1 << STM32L4_FLASH_CR_PG;
    } else {
        cr_reg = FLASH_CR;
        cr_mer = 1 << FLASH_CR_MER;
        cr_pg = 1 << FLASH_CR_PG;
    }

    stlink_read_debug32(sl, cr_reg, &val);
    if (val & cr_pg) {
        /* STM32F030 will drop MER bit if PG was set */
        val &= ~cr_pg;
        stlink_write_debug32(sl, cr_reg, val);
    }

    if(v)
        val |= cr_mer;
    else
        val &= ~cr_mer;
    stlink_write_debug32(sl, cr_reg, val);
}

static void __attribute__((unused)) clear_flash_cr_mer(stlink_t *sl) {
    uint32_t val, cr_reg, cr_mer;

    if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
        cr_reg = FLASH_F4_CR;
        cr_mer = 1 << FLASH_CR_MER;
    } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
        cr_reg = STM32L4_FLASH_CR;
        cr_mer = (1 << STM32L4_FLASH_CR_MER1) | (1 << STM32L4_FLASH_CR_MER2);
    } else {
        cr_reg = FLASH_CR;
        cr_mer = 1 << FLASH_CR_MER;
    }

    stlink_read_debug32(sl, cr_reg, &val);
    val &= ~cr_mer;
    stlink_write_debug32(sl, cr_reg, val);
}

static void set_flash_cr_strt(stlink_t *sl) {
    uint32_t val, cr_reg, cr_strt;

    if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
        cr_reg = FLASH_F4_CR;
        cr_strt = 1 << FLASH_F4_CR_STRT;
    } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
        cr_reg = STM32L4_FLASH_CR;
        cr_strt = 1 << STM32L4_FLASH_CR_STRT;
    } else {
        cr_reg = FLASH_CR;
        cr_strt = 1 << FLASH_CR_STRT;
    }

    stlink_read_debug32(sl, cr_reg, &val);
    val |= cr_strt;
    stlink_write_debug32(sl, cr_reg, val);
}

static void set_flash_cr2_strt(stlink_t *sl) {
    uint32_t val;

    stlink_read_debug32(sl, FLASH_CR2, &val);
    val |= 1 << FLASH_CR_STRT;
    stlink_write_debug32(sl, FLASH_CR2, val);
}

static inline uint32_t read_flash_sr(stlink_t *sl) {
    uint32_t res, sr_reg;

    if (sl->flash_type == STLINK_FLASH_TYPE_F4)
        sr_reg = FLASH_F4_SR;
    else if (sl->flash_type == STLINK_FLASH_TYPE_L4)
        sr_reg = STM32L4_FLASH_SR;
    else
        sr_reg = FLASH_SR;

    stlink_read_debug32(sl, sr_reg, &res);

    return res;
}

static inline uint32_t read_flash_sr2(stlink_t *sl) {
    uint32_t res;
    stlink_read_debug32(sl, FLASH_SR2, &res);
    return res;
}

static inline unsigned int is_flash_busy(stlink_t *sl) {
    uint32_t sr_busy_shift;
    unsigned int res;

    if (sl->flash_type == STLINK_FLASH_TYPE_F4)
        sr_busy_shift = FLASH_F4_SR_BSY;
    else if (sl->flash_type == STLINK_FLASH_TYPE_L4)
        sr_busy_shift = STM32L4_FLASH_SR_BSY;
    else
        sr_busy_shift = FLASH_SR_BSY;

    res = read_flash_sr(sl) & (1 << sr_busy_shift);

    if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
        res |= read_flash_sr2(sl) & (1 << sr_busy_shift);
    }

    return res;
}

static void wait_flash_busy(stlink_t *sl) {
    /* todo: add some delays here */
    while (is_flash_busy(sl))
        ;
}

static void wait_flash_busy_progress(stlink_t *sl) {
    int i = 0;
    fprintf(stdout, "Mass erasing");
    fflush(stdout);
    while (is_flash_busy(sl)) {
        usleep(10000);
        i++;
        if (i % 100 == 0) {
            fprintf(stdout, ".");
            fflush(stdout);
        }
    }
    fprintf(stdout, "\n");
}

static inline unsigned int is_flash_eop(stlink_t *sl) {
    return read_flash_sr(sl) & (1 << FLASH_SR_EOP);
}

static void __attribute__((unused)) clear_flash_sr_eop(stlink_t *sl) {
    const uint32_t n = read_flash_sr(sl) & ~(1 << FLASH_SR_EOP);
    stlink_write_debug32(sl, FLASH_SR, n);
}

static void __attribute__((unused)) wait_flash_eop(stlink_t *sl) {
    /* todo: add some delays here */
    while (is_flash_eop(sl) == 0)
        ;
}

static inline void write_flash_ar(stlink_t *sl, uint32_t n) {
    stlink_write_debug32(sl, FLASH_AR, n);
}

static inline void write_flash_ar2(stlink_t *sl, uint32_t n) {
    stlink_write_debug32(sl, FLASH_AR2, n);
}

static inline void write_flash_cr_psiz(stlink_t *sl, uint32_t n) {
    uint32_t x = read_flash_cr(sl);
    x &= ~(0x03 << 8);
    x |= (n << 8);
#if DEBUG_FLASH
    fprintf(stdout, "PSIZ:0x%x 0x%x\n", x, n);
#endif
    stlink_write_debug32(sl, FLASH_F4_CR, x);
}


static inline void write_flash_cr_snb(stlink_t *sl, uint32_t n) {
    uint32_t x = read_flash_cr(sl);
    x &= ~FLASH_F4_CR_SNB_MASK;
    x |= (n << FLASH_F4_CR_SNB);
    x |= (1 << FLASH_F4_CR_SER);
#if DEBUG_FLASH
    fprintf(stdout, "SNB:0x%x 0x%x\n", x, n);
#endif
    stlink_write_debug32(sl, FLASH_F4_CR, x);
}

static inline void write_flash_cr_bker_pnb(stlink_t *sl, uint32_t n) {
    stlink_write_debug32(sl, STM32L4_FLASH_SR, 0xFFFFFFFF & ~(1<<STM32L4_FLASH_SR_BSY));
    uint32_t x = read_flash_cr(sl);
    x &=~ STM32L4_FLASH_CR_OPBITS;
    x &=~ STM32L4_FLASH_CR_PAGEMASK;
    x &= ~(1<<STM32L4_FLASH_CR_MER1);
    x &= ~(1<<STM32L4_FLASH_CR_MER2);
    x |= (n << STM32L4_FLASH_CR_PNB);
    x |= (1lu << STM32L4_FLASH_CR_PER);
#if DEBUG_FLASH
    fprintf(stdout, "BKER:PNB:0x%x 0x%x\n", x, n);
#endif
    stlink_write_debug32(sl, STM32L4_FLASH_CR, x);
}

// Delegates to the backends...

void stlink_close(stlink_t *sl) {
    DLOG("*** stlink_close ***\n");
    if (!sl)
         return;
    sl->backend->close(sl);
    free(sl);
}

int stlink_exit_debug_mode(stlink_t *sl) {
    int ret;

    DLOG("*** stlink_exit_debug_mode ***\n");
    ret = stlink_write_debug32(sl, STLINK_REG_DHCSR, STLINK_REG_DHCSR_DBGKEY);
    if (ret == -1)
        return ret;

    return sl->backend->exit_debug_mode(sl);
}

int stlink_enter_swd_mode(stlink_t *sl) {
    DLOG("*** stlink_enter_swd_mode ***\n");
    return sl->backend->enter_swd_mode(sl);
}

// Force the core into the debug mode -> halted state.
int stlink_force_debug(stlink_t *sl) {
    DLOG("*** stlink_force_debug_mode ***\n");
    return sl->backend->force_debug(sl);
}

int stlink_exit_dfu_mode(stlink_t *sl) {
    DLOG("*** stlink_exit_dfu_mode ***\n");
    return sl->backend->exit_dfu_mode(sl);
}

int stlink_core_id(stlink_t *sl) {
    int ret;

    DLOG("*** stlink_core_id ***\n");
    ret = sl->backend->core_id(sl);
    if (ret == -1) {
        ELOG("Failed to read core_id\n");
        return ret;
    }
    if (sl->verbose > 2)
        stlink_print_data(sl);
    DLOG("core_id = 0x%08x\n", sl->core_id);
    return ret;
}

int stlink_chip_id(stlink_t *sl, uint32_t *chip_id) {
    int ret;

    ret = stlink_read_debug32(sl, 0xE0042000, chip_id);
    if (ret == -1)
        return ret;

    if (*chip_id == 0)
        ret = stlink_read_debug32(sl, 0x40015800, chip_id);    //Try Corex M0 DBGMCU_IDCODE register address

    return ret;
}

/**
 * Cortex m3 tech ref manual, CPUID register description
 * @param sl stlink context
 * @param cpuid pointer to the result object
 */
int stlink_cpu_id(stlink_t *sl, cortex_m3_cpuid_t *cpuid) {
    uint32_t raw;

    if (stlink_read_debug32(sl, STLINK_REG_CM3_CPUID, &raw))
        return -1;

    cpuid->implementer_id = (raw >> 24) & 0x7f;
    cpuid->variant = (raw >> 20) & 0xf;
    cpuid->part = (raw >> 4) & 0xfff;
    cpuid->revision = raw & 0xf;
    return 0;
}

/**
 * reads and decodes the flash parameters, as dynamically as possible
 * @param sl
 * @return 0 for success, or -1 for unsupported core type.
 */
int stlink_load_device_params(stlink_t *sl) {
    ILOG("Loading device parameters....\n");
    const struct stlink_chipid_params *params = NULL;
    stlink_core_id(sl);
    uint32_t chip_id;
    uint32_t flash_size;

    stlink_chip_id(sl, &chip_id);
    sl->chip_id = chip_id & 0xfff;
    /* Fix chip_id for F4 rev A errata , Read CPU ID, as CoreID is the same for F2/F4*/
    if (sl->chip_id == 0x411) {
        uint32_t cpuid;
        stlink_read_debug32(sl, 0xE000ED00, &cpuid);
        if ((cpuid  & 0xfff0) == 0xc240)
            sl->chip_id = 0x413;
    }

    params = stlink_chipid_get_params(sl->chip_id);
    if (params == NULL) {
        WLOG("unknown chip id! %#x\n", chip_id);
        return -1;
    }

    if (params->flash_type == STLINK_FLASH_TYPE_UNKNOWN) {
        WLOG("Invalid flash type, please check device declaration\n");
        return -1;
    }

    // These are fixed...
    sl->flash_base = STM32_FLASH_BASE;
    sl->sram_base = STM32_SRAM_BASE;
    stlink_read_debug32(sl,(params->flash_size_reg) & ~3, &flash_size);
    if (params->flash_size_reg & 2)
        flash_size = flash_size >>16;
    flash_size = flash_size & 0xffff;

    if ((sl->chip_id == STLINK_CHIPID_STM32_L1_MEDIUM || sl->chip_id == STLINK_CHIPID_STM32_L1_MEDIUM_PLUS) && ( flash_size == 0 )) {
        sl->flash_size = 128 * 1024;
    } else if (sl->chip_id == STLINK_CHIPID_STM32_L1_CAT2) {
        sl->flash_size = (flash_size & 0xff) * 1024;
    } else if ((sl->chip_id & 0xFFF) == STLINK_CHIPID_STM32_L1_HIGH) {
        // 0 is 384k and 1 is 256k
        if ( flash_size == 0 ) {
            sl->flash_size = 384 * 1024;
        } else {
            sl->flash_size = 256 * 1024;
        }
    } else {
        sl->flash_size = flash_size * 1024;
    }
    sl->flash_type = params->flash_type;
    sl->flash_pgsz = params->flash_pagesize;
    sl->sram_size = params->sram_size;
    sl->sys_base = params->bootrom_base;
    sl->sys_size = params->bootrom_size;

    //medium and low devices have the same chipid. ram size depends on flash size.
    //STM32F100xx datasheet Doc ID 16455 Table 2
    if(sl->chip_id == STLINK_CHIPID_STM32_F1_VL_MEDIUM_LOW && sl->flash_size < 64 * 1024){
        sl->sram_size = 0x1000;
    }

    ILOG("Device connected is: %s, id %#x\n", params->description, chip_id);
    // TODO make note of variable page size here.....
    ILOG("SRAM size: %#x bytes (%d KiB), Flash: %#x bytes (%d KiB) in pages of %u bytes\n",
            sl->sram_size, sl->sram_size / 1024, sl->flash_size, sl->flash_size / 1024,
	 (unsigned int)sl->flash_pgsz);
    return 0;
}

int stlink_reset(stlink_t *sl) {
    DLOG("*** stlink_reset ***\n");
    return sl->backend->reset(sl);
}

int stlink_jtag_reset(stlink_t *sl, int value) {
    DLOG("*** stlink_jtag_reset ***\n");
    return sl->backend->jtag_reset(sl, value);
}

int stlink_run(stlink_t *sl) {
    DLOG("*** stlink_run ***\n");
    return sl->backend->run(sl);
}

int stlink_set_swdclk(stlink_t *sl, uint16_t divisor) {
    DLOG("*** set_swdclk ***\n");
    return sl->backend->set_swdclk(sl, divisor);
}

int stlink_status(stlink_t *sl) {
    int ret;

    DLOG("*** stlink_status ***\n");
    ret = sl->backend->status(sl);
    stlink_core_stat(sl);

    return ret;
}

/**
 * Decode the version bits, originally from -sg, verified with usb
 * @param sl stlink context, assumed to contain valid data in the buffer
 * @param slv output parsed version object
 */
void _parse_version(stlink_t *sl, stlink_version_t *slv) {
    uint32_t b0 = sl->q_buf[0]; //lsb
    uint32_t b1 = sl->q_buf[1];
    uint32_t b2 = sl->q_buf[2];
    uint32_t b3 = sl->q_buf[3];
    uint32_t b4 = sl->q_buf[4];
    uint32_t b5 = sl->q_buf[5]; //msb

    // b0 b1                       || b2 b3  | b4 b5
    // 4b        | 6b     | 6b     || 2B     | 2B
    // stlink_v  | jtag_v | swim_v || st_vid | stlink_pid

    slv->stlink_v = (b0 & 0xf0) >> 4;
    slv->jtag_v = ((b0 & 0x0f) << 2) | ((b1 & 0xc0) >> 6);
    slv->swim_v = b1 & 0x3f;
    slv->st_vid = (b3 << 8) | b2;
    slv->stlink_pid = (b5 << 8) | b4;
    return;
}

int stlink_version(stlink_t *sl) {
    DLOG("*** looking up stlink version\n");
    if (sl->backend->version(sl))
        return -1;

    _parse_version(sl, &sl->version);

    DLOG("st vid         = 0x%04x (expect 0x%04x)\n", sl->version.st_vid, STLINK_USB_VID_ST);
    DLOG("stlink pid     = 0x%04x\n", sl->version.stlink_pid);
    DLOG("stlink version = 0x%x\n", sl->version.stlink_v);
    DLOG("jtag version   = 0x%x\n", sl->version.jtag_v);
    DLOG("swim version   = 0x%x\n", sl->version.swim_v);
    if (sl->version.jtag_v == 0) {
        DLOG("    notice: the firmware doesn't support a jtag/swd interface\n");
    }
    if (sl->version.swim_v == 0) {
        DLOG("    notice: the firmware doesn't support a swim interface\n");
    }

    return 0;
}

int stlink_target_voltage(stlink_t *sl) {
    int voltage = -1;
    DLOG("*** reading target voltage\n");
    if (sl->backend->target_voltage != NULL) {
        voltage = sl->backend->target_voltage(sl);
        if (voltage != -1) {
            DLOG("target voltage = %ldmV\n", voltage);
        } else {
            DLOG("error reading target voltage\n");
        }
    } else {
        DLOG("reading voltage not supported by backend\n");
    }
    return voltage;
}

int stlink_read_debug32(stlink_t *sl, uint32_t addr, uint32_t *data) {
    int ret;

    ret = sl->backend->read_debug32(sl, addr, data);
    if (!ret)
	    DLOG("*** stlink_read_debug32 %x is %#x\n", *data, addr);

	return ret;
}

int stlink_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data) {
    DLOG("*** stlink_write_debug32 %x to %#x\n", data, addr);
    return sl->backend->write_debug32(sl, addr, data);
}

int stlink_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    DLOG("*** stlink_write_mem32 %u bytes to %#x\n", len, addr);
    if (len % 4 != 0) {
        fprintf(stderr, "Error: Data length doesn't have a 32 bit alignment: +%d byte.\n", len % 4);
        abort();
    }
    return sl->backend->write_mem32(sl, addr, len);
}

int stlink_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    DLOG("*** stlink_read_mem32 ***\n");
    if (len % 4 != 0) { // !!! never ever: fw gives just wrong values
        fprintf(stderr, "Error: Data length doesn't have a 32 bit alignment: +%d byte.\n",
                len % 4);
        abort();
    }
    return sl->backend->read_mem32(sl, addr, len);
}

int stlink_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len) {
    DLOG("*** stlink_write_mem8 ***\n");
    if (len > 0x40 ) { // !!! never ever: Writing more then 0x40 bytes gives unexpected behaviour
        fprintf(stderr, "Error: Data length > 64: +%d byte.\n",
                len);
        abort();
    }
    return sl->backend->write_mem8(sl, addr, len);
}

int stlink_read_all_regs(stlink_t *sl, struct stlink_reg *regp) {
    DLOG("*** stlink_read_all_regs ***\n");
    return sl->backend->read_all_regs(sl, regp);
}

int stlink_read_all_unsupported_regs(stlink_t *sl, struct stlink_reg *regp) {
    DLOG("*** stlink_read_all_unsupported_regs ***\n");
    return sl->backend->read_all_unsupported_regs(sl, regp);
}

int stlink_write_reg(stlink_t *sl, uint32_t reg, int idx) {
    DLOG("*** stlink_write_reg\n");
    return sl->backend->write_reg(sl, reg, idx);
}

int stlink_read_reg(stlink_t *sl, int r_idx, struct stlink_reg *regp) {
    DLOG("*** stlink_read_reg\n");
    DLOG(" (%d) ***\n", r_idx);

    if (r_idx > 20 || r_idx < 0) {
        fprintf(stderr, "Error: register index must be in [0..20]\n");
        return -1;
    }

    return sl->backend->read_reg(sl, r_idx, regp);
}

int stlink_read_unsupported_reg(stlink_t *sl, int r_idx, struct stlink_reg *regp) {
    int r_convert;

    DLOG("*** stlink_read_unsupported_reg\n");
    DLOG(" (%d) ***\n", r_idx);

    /* Convert to values used by STLINK_REG_DCRSR */
    if (r_idx >= 0x1C && r_idx <= 0x1F) { /* primask, basepri, faultmask, or control */
        r_convert = 0x14;
    } else if (r_idx == 0x40) {     /* FPSCR */
        r_convert = 0x21;
    } else if (r_idx >= 0x20 && r_idx < 0x40) {
        r_convert = 0x40 + (r_idx - 0x20);
    } else {
        fprintf(stderr, "Error: register address must be in [0x1C..0x40]\n");
        return -1;
    }

    return sl->backend->read_unsupported_reg(sl, r_convert, regp);
}

int stlink_write_unsupported_reg(stlink_t *sl, uint32_t val, int r_idx, struct stlink_reg *regp) {
    int r_convert;

    DLOG("*** stlink_write_unsupported_reg\n");
    DLOG(" (%d) ***\n", r_idx);

    /* Convert to values used by STLINK_REG_DCRSR */
    if (r_idx >= 0x1C && r_idx <= 0x1F) { /* primask, basepri, faultmask, or control */
        r_convert = r_idx;  /* The backend function handles this */
    } else if (r_idx == 0x40) {     /* FPSCR */
        r_convert = 0x21;
    } else if (r_idx >= 0x20 && r_idx < 0x40) {
        r_convert = 0x40 + (r_idx - 0x20);
    } else {
        fprintf(stderr, "Error: register address must be in [0x1C..0x40]\n");
        return -1;
    }

    return sl->backend->write_unsupported_reg(sl, val, r_convert, regp);
}

bool stlink_is_core_halted(stlink_t *sl)
{
	bool ret = false;

	stlink_status(sl);
	if (sl->q_buf[0] == STLINK_CORE_HALTED)
		ret = true;

	return ret;
}

int stlink_step(stlink_t *sl) {
    DLOG("*** stlink_step ***\n");
    return sl->backend->step(sl);
}

int stlink_current_mode(stlink_t *sl) {
    int mode = sl->backend->current_mode(sl);
    switch (mode) {
    case STLINK_DEV_DFU_MODE:
        DLOG("stlink current mode: dfu\n");
        return mode;
    case STLINK_DEV_DEBUG_MODE:
        DLOG("stlink current mode: debug (jtag or swd)\n");
        return mode;
    case STLINK_DEV_MASS_MODE:
        DLOG("stlink current mode: mass\n");
        return mode;
    }
    DLOG("stlink mode: unknown!\n");
    return STLINK_DEV_UNKNOWN_MODE;
}




// End of delegates....  Common code below here...

// Endianness
// http://www.ibm.com/developerworks/aix/library/au-endianc/index.html
// const int i = 1;
// #define is_bigendian() ( (*(char*)&i) == 0 )

unsigned int is_bigendian(void) {
    static volatile const unsigned int i = 1;
    return *(volatile const char*) &i == 0;
}

uint16_t read_uint16(const unsigned char *c, const int pt) {
    uint32_t ui;
    char *p = (char *) &ui;

    if (!is_bigendian()) { // le -> le (don't swap)
        p[0] = c[pt + 0];
        p[1] = c[pt + 1];
    } else {
        p[0] = c[pt + 1];
        p[1] = c[pt + 0];
    }
    return ui;
}

// same as above with entrypoint.

void stlink_run_at(stlink_t *sl, stm32_addr_t addr) {
    stlink_write_reg(sl, addr, 15); /* pc register */

    stlink_run(sl);

    while (stlink_is_core_halted(sl))
        usleep(3000000);
}

void stlink_core_stat(stlink_t *sl) {
    if (sl->q_len <= 0)
        return;

    switch (sl->q_buf[0]) {
    case STLINK_CORE_RUNNING:
        sl->core_stat = STLINK_CORE_RUNNING;
        DLOG("  core status: running\n");
        return;
    case STLINK_CORE_HALTED:
        sl->core_stat = STLINK_CORE_HALTED;
        DLOG("  core status: halted\n");
        return;
    default:
        sl->core_stat = STLINK_CORE_STAT_UNKNOWN;
        fprintf(stderr, "  core status: unknown\n");
    }
}

void stlink_print_data(stlink_t * sl) {
    if (sl->q_len <= 0 || sl->verbose < UDEBUG)
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

    const int fd = open(path, O_RDONLY | O_BINARY);
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

/* Limit the block size to compare to 0x1800
   Anything larger will stall the STLINK2
   Maybe STLINK V1 needs smaller value!*/
static int check_file(stlink_t* sl, mapped_file_t* mf, stm32_addr_t addr) {
    size_t off;
    size_t n_cmp = sl->flash_pgsz;
    if ( n_cmp > 0x1800)
        n_cmp = 0x1800;

    for (off = 0; off < mf->len; off += n_cmp) {
        size_t aligned_size;

        /* adjust last page size */
        size_t cmp_size = n_cmp;
        if ((off + n_cmp) > mf->len)
            cmp_size = mf->len - off;

        aligned_size = cmp_size;
        if (aligned_size & (4 - 1))
            aligned_size = (cmp_size + 4) & ~(4 - 1);

        stlink_read_mem32(sl, addr + (uint32_t) off, aligned_size);

        if (memcmp(sl->q_buf, mf->base + off, cmp_size))
            return -1;
    }

    return 0;
}

static void stlink_fwrite_finalize(stlink_t *sl, stm32_addr_t addr) {
    unsigned int val;
    /* set stack*/
    stlink_read_debug32(sl, addr, &val);
    stlink_write_reg(sl, val, 13);
    /* Set PC to the reset routine*/
    stlink_read_debug32(sl, addr + 4, &val);
    stlink_write_reg(sl, val, 15);
    stlink_run(sl);
}

int stlink_mwrite_sram(stlink_t * sl, uint8_t* data, uint32_t length, stm32_addr_t addr) {
    /* write the file in sram at addr */

    int error = -1;
    size_t off;
    size_t len;

    /* check addr range is inside the sram */
    if (addr < sl->sram_base) {
        fprintf(stderr, "addr too low\n");
        goto on_error;
    } else if ((addr + length) < addr) {
        fprintf(stderr, "addr overruns\n");
        goto on_error;
    } else if ((addr + length) > (sl->sram_base + sl->sram_size)) {
        fprintf(stderr, "addr too high\n");
        goto on_error;
    } else if (addr & 3) {
        /* todo */
        fprintf(stderr, "unaligned addr\n");
        goto on_error;
    }

    len = length;

    if(len & 3) {
      len -= len & 3;
    }

    /* do the copy by 1k blocks */
    for (off = 0; off < len; off += 1024) {
        size_t size = 1024;
        if ((off + size) > len)
            size = len - off;

        memcpy(sl->q_buf, data + off, size);

        /* round size if needed */
        if (size & 3)
            size += 2;

        stlink_write_mem32(sl, addr + (uint32_t) off, size);
    }

    if(length > len) {
        memcpy(sl->q_buf, data + len, length - len);
        stlink_write_mem8(sl, addr + (uint32_t) len, length - len);
    }

    /* success */
    error = 0;
    stlink_fwrite_finalize(sl, addr);

on_error:
    return error;
}

int stlink_fwrite_sram(stlink_t * sl, const char* path, stm32_addr_t addr) {
    /* write the file in sram at addr */

    int error = -1;
    size_t off;
    size_t len;
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
    } else if (addr & 3) {
        /* todo */
        fprintf(stderr, "unaligned addr\n");
        goto on_error;
    }

    len = mf.len;

    if(len & 3) {
      len -= len & 3;
    }

    /* do the copy by 1k blocks */
    for (off = 0; off < len; off += 1024) {
        size_t size = 1024;
        if ((off + size) > len)
            size = len - off;

        memcpy(sl->q_buf, mf.base + off, size);

        /* round size if needed */
        if (size & 3)
            size += 2;

        stlink_write_mem32(sl, addr + (uint32_t) off, size);
    }

    if(mf.len > len) {
        memcpy(sl->q_buf, mf.base + len, mf.len - len);
        stlink_write_mem8(sl, addr + (uint32_t) len, mf.len - len);
    }

    /* check the file ha been written */
    if (check_file(sl, &mf, addr) == -1) {
        fprintf(stderr, "check_file() == -1\n");
        goto on_error;
    }

    /* success */
    error = 0;
    stlink_fwrite_finalize(sl, addr);

on_error:
    unmap_file(&mf);
    return error;
}

typedef bool (*save_block_fn)(void* arg, uint8_t* block, ssize_t len);

static int stlink_read(stlink_t* sl, stm32_addr_t addr, size_t size, save_block_fn fn, void* fn_arg) {

    int error = -1;

    if (size <1)
        size = sl->flash_size;

    if (size > sl->flash_size)
        size = sl->flash_size;

    size_t cmp_size = (sl->flash_pgsz > 0x1800)? 0x1800:sl->flash_pgsz;
    for (size_t off = 0; off < size; off += cmp_size) {
        size_t aligned_size;

        /* adjust last page size */
        if ((off + cmp_size) > size)
            cmp_size = size - off;

        aligned_size = cmp_size;
        if (aligned_size & (4 - 1))
            aligned_size = (cmp_size + 4) & ~(4 - 1);

        stlink_read_mem32(sl, addr + (uint32_t) off, aligned_size);

        if (!fn(fn_arg, sl->q_buf, aligned_size)) {
            goto on_error;
        }
    }

    /* success */
    error = 0;

on_error:
    return error;
}

struct stlink_fread_worker_arg {
    int fd;
};

static bool stlink_fread_worker(void* arg, uint8_t* block, ssize_t len) {
    struct stlink_fread_worker_arg* the_arg = (struct stlink_fread_worker_arg*)arg;
    if (write(the_arg->fd, block, len) != len) {
        fprintf(stderr, "write() != aligned_size\n");
        return false;
    }
    else {
        return true;
    }
}

struct stlink_fread_ihex_worker_arg {
    FILE* file;
    uint32_t addr;
    uint32_t lba;
    uint8_t buf[16];
    uint8_t buf_pos;
};

static bool stlink_fread_ihex_newsegment(struct stlink_fread_ihex_worker_arg* the_arg) {
    uint32_t addr = the_arg->addr;
    uint8_t sum = 2 + 4 + (uint8_t)((addr & 0xFF000000) >> 24) + (uint8_t)((addr & 0x00FF0000) >> 16);
    if(17 != fprintf(the_arg->file, ":02000004%04X%02X\r\n", (addr & 0xFFFF0000) >> 16, (uint8_t)(0x100 - sum)))
        return false;

    the_arg->lba = (addr & 0xFFFF0000);

    return true;
}

static bool stlink_fread_ihex_writeline(struct stlink_fread_ihex_worker_arg* the_arg) {
    uint8_t count = the_arg->buf_pos;
    if(count == 0) return true;

    uint32_t addr = the_arg->addr;

    if(the_arg->lba != (addr & 0xFFFF0000)) { // segment changed
        if(!stlink_fread_ihex_newsegment(the_arg)) return false;
    }

    uint8_t sum = count + (uint8_t)((addr & 0x0000FF00) >> 8) + (uint8_t)(addr & 0x000000FF);
    if(9 != fprintf(the_arg->file, ":%02X%04X00", count, (addr & 0x0000FFFF)))
        return false;

    for(uint8_t i = 0; i < count; ++i) {
        uint8_t b = the_arg->buf[i];
        sum += b;
        if(2 != fprintf(the_arg->file, "%02X", b))
            return false;
    }

    if(4 != fprintf(the_arg->file, "%02X\r\n", (uint8_t)(0x100 - sum)))
        return false;

    the_arg->addr += count;
    the_arg->buf_pos = 0;

    return true;
}

static bool stlink_fread_ihex_init(struct stlink_fread_ihex_worker_arg* the_arg, int fd, stm32_addr_t addr) {
    the_arg->file    = fdopen(fd, "w");
    the_arg->addr    = addr;
    the_arg->lba     = 0;
    the_arg->buf_pos = 0;

    return (the_arg->file != NULL);
}

static bool stlink_fread_ihex_worker(void* arg, uint8_t* block, ssize_t len) {
    struct stlink_fread_ihex_worker_arg* the_arg = (struct stlink_fread_ihex_worker_arg*)arg;

    for(ssize_t i = 0; i < len; ++i) {
        if(the_arg->buf_pos == sizeof(the_arg->buf)) { // line is full
            if(!stlink_fread_ihex_writeline(the_arg)) return false;
        }

        the_arg->buf[the_arg->buf_pos++] = block[i];
    }

    return true;
}

static bool stlink_fread_ihex_finalize(struct stlink_fread_ihex_worker_arg* the_arg) {
    if(!stlink_fread_ihex_writeline(the_arg)) return false;

    // FIXME do we need the Start Linear Address?

    if(13 != fprintf(the_arg->file, ":00000001FF\r\n")) // EoF
        return false;

    return (0 == fclose(the_arg->file));
}

int stlink_fread(stlink_t* sl, const char* path, bool is_ihex, stm32_addr_t addr, size_t size) {
    /* read size bytes from addr to file */

    int error;

    int fd = open(path, O_RDWR | O_TRUNC | O_CREAT, 00700);
    if (fd == -1) {
        fprintf(stderr, "open(%s) == -1\n", path);
        return -1;
    }

    if(is_ihex) {
        struct stlink_fread_ihex_worker_arg arg;
        if(stlink_fread_ihex_init(&arg, fd, addr)) {
            error = stlink_read(sl, addr, size, &stlink_fread_ihex_worker, &arg);
            if(!stlink_fread_ihex_finalize(&arg))
                error = -1;
        }
        else {
            error = -1;
        }
    }
    else {
        struct stlink_fread_worker_arg arg = { fd };
        error = stlink_read(sl, addr, size, &stlink_fread_worker, &arg);
    }

    close(fd);

    return error;
}

int write_buffer_to_sram(stlink_t *sl, flash_loader_t* fl, const uint8_t* buf, size_t size) {
    /* write the buffer right after the loader */
    size_t chunk = size & ~0x3;
    size_t rem   = size & 0x3;
    if (chunk) {
        memcpy(sl->q_buf, buf, chunk);
        stlink_write_mem32(sl, fl->buf_addr, chunk);
    }
    if (rem) {
        memcpy(sl->q_buf, buf+chunk, rem);
        stlink_write_mem8(sl, (fl->buf_addr) + (uint32_t) chunk, rem);
    }
    return 0;
}

uint32_t calculate_F4_sectornum(uint32_t flashaddr){
    uint32_t offset = 0;
    flashaddr &= ~STM32_FLASH_BASE;	//Page now holding the actual flash address
    if (flashaddr >= 0x100000) {
        offset = 12;
        flashaddr -= 0x100000;
    }
    if (flashaddr<0x4000) return (offset + 0);
    else if(flashaddr<0x8000) return(offset + 1);
    else if(flashaddr<0xc000) return(offset + 2);
    else if(flashaddr<0x10000) return(offset + 3);
    else if(flashaddr<0x20000) return(offset + 4);
    else return offset + (flashaddr/0x20000) +4;

}

uint32_t calculate_F7_sectornum(uint32_t flashaddr){
    flashaddr &= ~STM32_FLASH_BASE;	//Page now holding the actual flash address
	if(flashaddr<0x20000) return(flashaddr/0x8000);
    else if(flashaddr<0x40000) return(4);
    else return(flashaddr/0x40000) +4;

}

// Returns BKER:PNB for the given page address
uint32_t calculate_L4_page(stlink_t *sl, uint32_t flashaddr) {
    uint32_t bker = 0;
    uint32_t flashopt;
    stlink_read_debug32(sl, STM32L4_FLASH_OPTR, &flashopt);
    flashaddr -= STM32_FLASH_BASE;
    if (flashopt & (1lu << STM32L4_FLASH_OPTR_DUALBANK)) {
        uint32_t banksize = (uint32_t) sl->flash_size / 2;
        if (flashaddr >= banksize) {
            flashaddr -= banksize;
            bker = 0x100;
        }
    }
    // For 1MB chips without the dual-bank option set, the page address will
    // overflow into the BKER bit, which gives us the correct bank:page value.
    return bker | flashaddr/sl->flash_pgsz;
}

uint32_t stlink_calculate_pagesize(stlink_t *sl, uint32_t flashaddr){
    if ((sl->chip_id == STLINK_CHIPID_STM32_F2) || (sl->chip_id == STLINK_CHIPID_STM32_F4) || (sl->chip_id == STLINK_CHIPID_STM32_F4_DE) ||
            (sl->chip_id == STLINK_CHIPID_STM32_F4_LP) || (sl->chip_id == STLINK_CHIPID_STM32_F4_HD) || (sl->chip_id == STLINK_CHIPID_STM32_F411RE) ||
            (sl->chip_id == STLINK_CHIPID_STM32_F446) || (sl->chip_id == STLINK_CHIPID_STM32_F4_DSI)) {
        uint32_t sector=calculate_F4_sectornum(flashaddr);
        if (sector>= 12) {
            sector -= 12;
        }
        if (sector<4) sl->flash_pgsz=0x4000;
        else if(sector<5) sl->flash_pgsz=0x10000;
        else sl->flash_pgsz=0x20000;
    }
    else if (sl->chip_id == STLINK_CHIPID_STM32_F7 || sl->chip_id == STLINK_CHIPID_STM32_F7XXXX) {
        uint32_t sector=calculate_F7_sectornum(flashaddr);
        if (sector<4) sl->flash_pgsz=0x8000;
        else if(sector<5) sl->flash_pgsz=0x20000;
        else sl->flash_pgsz=0x40000;
    }
    return (uint32_t) sl->flash_pgsz;
}

/**
 * Erase a page of flash, assumes sl is fully populated with things like chip/core ids
 * @param sl stlink context
 * @param flashaddr an address in the flash page to erase
 * @return 0 on success -ve on failure
 */
int stlink_erase_flash_page(stlink_t *sl, stm32_addr_t flashaddr)
{
    if (sl->flash_type == STLINK_FLASH_TYPE_F4 || sl->flash_type == STLINK_FLASH_TYPE_L4) {
        /* wait for ongoing op to finish */
        wait_flash_busy(sl);

        /* unlock if locked */
        unlock_flash_if(sl);

        /* select the page to erase */
        if ((sl->chip_id == STLINK_CHIPID_STM32_L4) ||
            (sl->chip_id == STLINK_CHIPID_STM32_L43X) ||
            (sl->chip_id == STLINK_CHIPID_STM32_L46X) ||
            (sl->chip_id == STLINK_CHIPID_STM32_L496X)) {
            // calculate the actual bank+page from the address
            uint32_t page = calculate_L4_page(sl, flashaddr);

            fprintf(stderr, "EraseFlash - Page:0x%x Size:0x%x ", page, stlink_calculate_pagesize(sl, flashaddr));

            write_flash_cr_bker_pnb(sl, page);
        } else if (sl->chip_id == STLINK_CHIPID_STM32_F7 || sl->chip_id == STLINK_CHIPID_STM32_F7XXXX) {
            // calculate the actual page from the address
            uint32_t sector=calculate_F7_sectornum(flashaddr);

            fprintf(stderr, "EraseFlash - Sector:0x%x Size:0x%x ", sector, stlink_calculate_pagesize(sl, flashaddr));

            write_flash_cr_snb(sl, sector);
        } else {
            // calculate the actual page from the address
            uint32_t sector=calculate_F4_sectornum(flashaddr);

            fprintf(stderr, "EraseFlash - Sector:0x%x Size:0x%x ", sector, stlink_calculate_pagesize(sl, flashaddr));

            //the SNB values for flash sectors in the second bank do not directly follow the values for the first bank on 2mb devices...
            if (sector >= 12) sector += 4;

            write_flash_cr_snb(sl, sector);
        }

        /* start erase operation */
        set_flash_cr_strt(sl);

        /* wait for completion */
        wait_flash_busy(sl);

        /* relock the flash */
        //todo: fails to program if this is in
        lock_flash(sl);
#if DEBUG_FLASH
        fprintf(stdout, "Erase Final CR:0x%x\n", read_flash_cr(sl));
#endif
    } else if (sl->flash_type == STLINK_FLASH_TYPE_L0) {

        uint32_t val;
        uint32_t flash_regs_base;
        if (sl->chip_id == STLINK_CHIPID_STM32_L0 || sl->chip_id == STLINK_CHIPID_STM32_L0_CAT5 || sl->chip_id == STLINK_CHIPID_STM32_L0_CAT2 || sl->chip_id == STLINK_CHIPID_STM32_L011) {
            flash_regs_base = STM32L0_FLASH_REGS_ADDR;
        } else {
            flash_regs_base = STM32L_FLASH_REGS_ADDR;
        }

        /* check if the locks are set */
        stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
        if((val & (1<<0))||(val & (1<<1))) {
            /* disable pecr protection */
            stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF, 0x89abcdef);
            stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF, 0x02030405);

            /* check pecr.pelock is cleared */
            stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
            if (val & (1 << 0)) {
                WLOG("pecr.pelock not clear (%#x)\n", val);
                return -1;
            }

            /* unlock program memory */
            stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, 0x8c9daebf);
            stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, 0x13141516);

            /* check pecr.prglock is cleared */
            stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
            if (val & (1 << 1)) {
                WLOG("pecr.prglock not clear (%#x)\n", val);
                return -1;
            }
        }

        /* set pecr.{erase,prog} */
        val |= (1 << 9) | (1 << 3);
        stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
#if 0 /* fix_to_be_confirmed */

        /* wait for sr.busy to be cleared
         * MP: Test shows that busy bit is not set here. Perhaps, PM0062 is
         * wrong and we do not need to wait here for clearing the busy bit.
         * TEXANE: ok, if experience says so and it works for you, we comment
         * it. If someone has a problem, please drop an email.
         */
        do {
            stlink_read_debug32(sl, STM32L_FLASH_SR, &val)
        } while((val & (1 << 0)) != 0);

#endif /* fix_to_be_confirmed */

        /* write 0 to the first word of the page to be erased */
        stlink_write_debug32(sl, flashaddr, 0);

        /* MP: It is better to wait for clearing the busy bit after issuing
           page erase command, even though PM0062 recommends to wait before it.
           Test shows that a few iterations is performed in the following loop
           before busy bit is cleared.*/
        do {
            stlink_read_debug32(sl, flash_regs_base + FLASH_SR_OFF, &val);
        } while ((val & (1 << 0)) != 0);

        /* reset lock bits */
        stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
        val |= (1 << 0) | (1 << 1) | (1 << 2);
        stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
    } else if ((sl->flash_type == STLINK_FLASH_TYPE_F0) || ((sl->flash_type == STLINK_FLASH_TYPE_F1_XL) && (flashaddr < FLASH_BANK2_START_ADDR))) {
        /* wait for ongoing op to finish */
        wait_flash_busy(sl);

        /* unlock if locked */
        unlock_flash_if(sl);

        /* set the page erase bit */
        set_flash_cr_per(sl);

        /* select the page to erase */
        write_flash_ar(sl, flashaddr);

        /* start erase operation, reset by hw with bsy bit */
        set_flash_cr_strt(sl);

        /* wait for completion */
        wait_flash_busy(sl);

        /* relock the flash */
        lock_flash(sl);
    } else if ((sl->flash_type == STLINK_FLASH_TYPE_F1_XL) && (flashaddr >= FLASH_BANK2_START_ADDR)) {
        /* wait for ongoing op to finish */
        wait_flash_busy(sl);

        /* unlock if locked */
        unlock_flash_if(sl);

        /* set the page erase bit */
        set_flash_cr2_per(sl);

        /* select the page to erase */
        write_flash_ar2(sl, flashaddr);

        /* start erase operation, reset by hw with bsy bit */
        set_flash_cr2_strt(sl);

        /* wait for completion */
        wait_flash_busy(sl);

        /* relock the flash */
        lock_flash(sl);
    } else {
        WLOG("unknown coreid %x, page erase failed\n", sl->core_id);
        return -1;
    }

    /* todo: verify the erased page */

    return 0;
}

int stlink_erase_flash_mass(stlink_t *sl) {
    if (sl->flash_type == STLINK_FLASH_TYPE_L0) {
        /* erase each page */
        int i = 0, num_pages = (int) sl->flash_size/sl->flash_pgsz;
        for (i = 0; i < num_pages; i++) {
            /* addr must be an addr inside the page */
            stm32_addr_t addr = (stm32_addr_t) sl->flash_base + i * (stm32_addr_t) sl->flash_pgsz;
            if (stlink_erase_flash_page(sl, addr) == -1) {
                WLOG("Failed to erase_flash_page(%#zx) == -1\n", addr);
                return -1;
            }
            fprintf(stdout,"-> Flash page at %5d/%5d erased\n", i, num_pages);
            fflush(stdout);
        }
        fprintf(stdout, "\n");
    } else {
        /* wait for ongoing op to finish */
        wait_flash_busy(sl);

        /* unlock if locked */
        unlock_flash_if(sl);

        /* set the mass erase bit */
        set_flash_cr_mer(sl,1);

        /* start erase operation, reset by hw with bsy bit */
        set_flash_cr_strt(sl);

        /* wait for completion */
        wait_flash_busy_progress(sl);

        /* relock the flash */
        lock_flash(sl);

        /* reset the mass erase bit */
        set_flash_cr_mer(sl,0);

        /* todo: verify the erased memory */
    }
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

/**
 * Verify addr..addr+len is binary identical to base...base+len
 * @param sl stlink context
 * @param address stm device address
 * @param data host side buffer to check against
 * @param length how much
 * @return 0 for success, -ve for failure
 */
int stlink_verify_write_flash(stlink_t *sl, stm32_addr_t address, uint8_t *data, unsigned length) {
    size_t off;
    size_t cmp_size = (sl->flash_pgsz > 0x1800)? 0x1800:sl->flash_pgsz;
    ILOG("Starting verification of write complete\n");
    for (off = 0; off < length; off += cmp_size) {
        size_t aligned_size;

        /* adjust last page size */
        if ((off + cmp_size) > length)
            cmp_size = length - off;

        aligned_size = cmp_size;
        if (aligned_size & (4 - 1))
            aligned_size = (cmp_size + 4) & ~(4 - 1);

        stlink_read_mem32(sl, address + (uint32_t) off, aligned_size);

        if (memcmp(sl->q_buf, data + off, cmp_size)) {
	  ELOG("Verification of flash failed at offset: %u\n", (unsigned int)off);
            return -1;
        }
    }
    ILOG("Flash written and verified! jolly good!\n");
    return 0;

}

int stm32l1_write_half_pages(stlink_t *sl, stm32_addr_t addr, uint8_t* base, uint32_t len, uint32_t pagesize)
{
    unsigned int count;
    unsigned int num_half_pages = len / pagesize;
    uint32_t val;
    uint32_t flash_regs_base;
    flash_loader_t fl;

    if (sl->chip_id == STLINK_CHIPID_STM32_L0 || sl->chip_id == STLINK_CHIPID_STM32_L0_CAT5 || sl->chip_id == STLINK_CHIPID_STM32_L0_CAT2 || sl->chip_id == STLINK_CHIPID_STM32_L011) {
        flash_regs_base = STM32L0_FLASH_REGS_ADDR;
    } else {
        flash_regs_base = STM32L_FLASH_REGS_ADDR;
    }

    ILOG("Starting Half page flash write for STM32L core id\n");
    /* flash loader initialization */
    if (stlink_flash_loader_init(sl, &fl) == -1) {
        WLOG("stlink_flash_loader_init() == -1\n");
        return -1;
    }
    /* Unlock already done */
    stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
    val |= (1 << FLASH_L1_FPRG);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);

    val |= (1 << FLASH_L1_PROG);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
    do {
        stlink_read_debug32(sl, flash_regs_base + FLASH_SR_OFF, &val);
    } while ((val & (1 << 0)) != 0);

    for (count = 0; count  < num_half_pages; count ++) {
        if (stlink_flash_loader_run(sl, &fl, addr + count * pagesize, base + count * pagesize, pagesize) == -1) {
            WLOG("l1_stlink_flash_loader_run(%#zx) failed! == -1\n", addr + count * pagesize);
            stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
            val &= ~((1 << FLASH_L1_FPRG) |(1 << FLASH_L1_PROG));
            stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
            return -1;
        }
        /* wait for sr.busy to be cleared */
        if (sl->verbose >= 1) {
            /* show progress. writing procedure is slow
               and previous errors are misleading */
            fprintf(stdout, "\r%3u/%u halfpages written", count + 1, num_half_pages);
            fflush(stdout);
        }
        do {
            stlink_read_debug32(sl, flash_regs_base + FLASH_SR_OFF, &val);
        } while ((val & (1 << 0)) != 0);
    }
    stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
    val &= ~(1 << FLASH_L1_PROG);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
    stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
    val &= ~(1 << FLASH_L1_FPRG);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);

    return 0;
}

int stlink_write_flash(stlink_t *sl, stm32_addr_t addr, uint8_t* base, uint32_t len, uint8_t eraseonly) {
    size_t off;
    flash_loader_t fl;
    ILOG("Attempting to write %d (%#x) bytes to stm32 address: %u (%#x)\n",
            len, len, addr, addr);
    /* check addr range is inside the flash */
    stlink_calculate_pagesize(sl, addr);
    if (addr < sl->flash_base) {
        ELOG("addr too low %#x < %#x\n", addr, sl->flash_base);
        return -1;
    } else if ((addr + len) < addr) {
        ELOG("addr overruns\n");
        return -1;
    } else if ((addr + len) > (sl->flash_base + sl->flash_size)) {
        ELOG("addr too high\n");
        return -1;
    } else if (addr & 1) {
        ELOG("unaligned addr 0x%x\n", addr);
        return -1;
    } else if (len & 1) {
        WLOG("unaligned len 0x%x -- padding with zero\n", len);
        len += 1;
    } else if (addr & (sl->flash_pgsz - 1)) {
        ELOG("addr not a multiple of pagesize, not supported\n");
        return -1;
    }

    // Make sure we've loaded the context with the chip details
    stlink_core_id(sl);
    /* erase each page */
    int page_count = 0;
    for (off = 0; off < len; off += stlink_calculate_pagesize(sl, addr + (uint32_t) off)) {
        /* addr must be an addr inside the page */
        if (stlink_erase_flash_page(sl, addr + (uint32_t) off) == -1) {
            ELOG("Failed to erase_flash_page(%#zx) == -1\n", addr + off);
            return -1;
        }
        fprintf(stdout,"\rFlash page at addr: 0x%08lx erased",
                (unsigned long)(addr + off));
        fflush(stdout);
        page_count++;
    }
    fprintf(stdout,"\n");
    ILOG("Finished erasing %d pages of %d (%#x) bytes\n",
            page_count, sl->flash_pgsz, sl->flash_pgsz);

    if (eraseonly)
        return 0;

    if ((sl->flash_type == STLINK_FLASH_TYPE_F4) || (sl->flash_type == STLINK_FLASH_TYPE_L4)) {
        /* todo: check write operation */

        ILOG("Starting Flash write for F2/F4/L4\n");
        /* flash loader initialization */
        if (stlink_flash_loader_init(sl, &fl) == -1) {
            ELOG("stlink_flash_loader_init() == -1\n");
            return -1;
        }

        /* First unlock the cr */
        unlock_flash_if(sl);

        /* TODO: Check that Voltage range is 2.7 - 3.6 V */
        if ((sl->chip_id != STLINK_CHIPID_STM32_L4) &&
            (sl->chip_id != STLINK_CHIPID_STM32_L43X) &&
            (sl->chip_id != STLINK_CHIPID_STM32_L46X) &&
            (sl->chip_id != STLINK_CHIPID_STM32_L496X)) {

            if( sl->version.stlink_v == 1 ) {
                printf("STLINK V1 cannot read voltage, defaulting to 32-bit writes on F4 devices\n");
                write_flash_cr_psiz(sl, 2);
            }
            else {
                /* set parallelisim to 32 bit*/
                int voltage = stlink_target_voltage(sl);
                if (voltage == -1) {
                    printf("Failed to read Target voltage\n");
                    return voltage;
                } else if (voltage > 2700) {
                    printf("enabling 32-bit flash writes\n");
                    write_flash_cr_psiz(sl, 2);
                } else {
                    printf("Target voltage (%d mV) too low for 32-bit flash, using 8-bit flash writes\n", voltage);
                    write_flash_cr_psiz(sl, 0);
                }
            }
        } else {
            /* L4 does not have a byte-write mode */
            int voltage = stlink_target_voltage(sl);
            if (voltage == -1) {
                printf("Failed to read Target voltage\n");
                return voltage;
            } else if (voltage < 1710) {
                printf("Target voltage (%d mV) too low for flash writes!\n", voltage);
                return -1;
            }
        }

        /* set programming mode */
        set_flash_cr_pg(sl);

		size_t buf_size = (sl->sram_size > 0x8000) ? 0x8000 : 0x4000;
        for(off = 0; off < len;) {
            size_t size = len - off > buf_size ? buf_size : len - off;

            printf("size: %u\n", (unsigned int)size);

            if (stlink_flash_loader_run(sl, &fl, addr + (uint32_t) off, base + off, size) == -1) {
                ELOG("stlink_flash_loader_run(%#zx) failed! == -1\n", addr + off);
                return -1;
            }

            off += size;
        }

        /* Relock flash */
        lock_flash(sl);

    }	//STM32F4END

    else if (sl->flash_type == STLINK_FLASH_TYPE_L0) {
        /* use fast word write. todo: half page. */
        uint32_t val;
        uint32_t flash_regs_base;
        uint32_t pagesize;

        if (sl->chip_id == STLINK_CHIPID_STM32_L0 || sl->chip_id == STLINK_CHIPID_STM32_L0_CAT5 || sl->chip_id == STLINK_CHIPID_STM32_L0_CAT2 || sl->chip_id == STLINK_CHIPID_STM32_L011) {
            flash_regs_base = STM32L0_FLASH_REGS_ADDR;
            pagesize = L0_WRITE_BLOCK_SIZE;
        } else {
            flash_regs_base = STM32L_FLASH_REGS_ADDR;
            pagesize = L1_WRITE_BLOCK_SIZE;
        }

        /* todo: check write operation */

        /* disable pecr protection */
        stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF, 0x89abcdef);
        stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF, 0x02030405);

        /* check pecr.pelock is cleared */
        stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
        if (val & (1 << 0)) {
            fprintf(stderr, "pecr.pelock not clear\n");
            return -1;
        }

        /* unlock program memory */
        stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, 0x8c9daebf);
        stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, 0x13141516);

        /* check pecr.prglock is cleared */
        stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
        if (val & (1 << 1)) {
            fprintf(stderr, "pecr.prglock not clear\n");
            return -1;
        }
        off = 0;
        if (len > pagesize) {
            if (stm32l1_write_half_pages(sl, addr, base, len, pagesize) == -1) {
                /* This may happen on a blank device! */
                WLOG("\nwrite_half_pages failed == -1\n");
            } else {
                off = (len / pagesize)*pagesize;
            }
        }

        /* write remainingword in program memory */
        for ( ; off < len; off += sizeof(uint32_t)) {
            uint32_t data;
            if (off > 254)
                fprintf(stdout, "\r");

            if ((off % sl->flash_pgsz) > (sl->flash_pgsz -5)) {
                fprintf(stdout, "\r%3u/%3u pages written",
                        (unsigned int)(off/sl->flash_pgsz),
			(unsigned int)(len/sl->flash_pgsz));
                fflush(stdout);
            }

            write_uint32((unsigned char*) &data, *(uint32_t*) (base + off));
            stlink_write_debug32(sl, addr + (uint32_t) off, data);

            /* wait for sr.busy to be cleared */
            do {
                stlink_read_debug32(sl, flash_regs_base + FLASH_SR_OFF, &val);
            } while ((val & (1 << 0)) != 0);

            /* todo: check redo write operation */

        }
        fprintf(stdout, "\n");
        /* reset lock bits */
        stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
        val |= (1 << 0) | (1 << 1) | (1 << 2);
        stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
    } else if ((sl->flash_type == STLINK_FLASH_TYPE_F0) || (sl->flash_type == STLINK_FLASH_TYPE_F1_XL)) {
        ILOG("Starting Flash write for VL/F0/F3/F1_XL core id\n");
        /* flash loader initialization */
        if (stlink_flash_loader_init(sl, &fl) == -1) {
            ELOG("stlink_flash_loader_init() == -1\n");
            return -1;
        }

        int write_block_count = 0;
        for (off = 0; off < len; off += sl->flash_pgsz) {
            /* adjust last write size */
            size_t size = sl->flash_pgsz;
            if ((off + sl->flash_pgsz) > len) size = len - off;

            /* unlock and set programming mode */
            unlock_flash_if(sl);
            if (sl->flash_type != STLINK_FLASH_TYPE_F1_XL) {
                set_flash_cr_pg(sl);
            }
            DLOG("Finished unlocking flash, running loader!\n");
            if (stlink_flash_loader_run(sl, &fl, addr + (uint32_t) off, base + off, size) == -1) {
                ELOG("stlink_flash_loader_run(%#zx) failed! == -1\n", addr + off);
                return -1;
            }
            lock_flash(sl);
            if (sl->verbose >= 1) {
                /* show progress. writing procedure is slow
                   and previous errors are misleading */
                fprintf(stdout, "\r%3u/%lu pages written", ++write_block_count, (unsigned long)((len+sl->flash_pgsz-1)/sl->flash_pgsz));
                fflush(stdout);
            }
        }
        fprintf(stdout, "\n");
    } else {
        ELOG("unknown coreid, not sure how to write: %x\n", sl->core_id);
        return -1;
    }

    return stlink_verify_write_flash(sl, addr, base, len);
}

// note: length not checked
static uint8_t stlink_parse_hex(const char* hex) {
    uint8_t d[2];
    for(int i = 0; i < 2; ++i) {
        char c = *(hex + i);
        if(c >= '0' && c <= '9') d[i] = c - '0';
        else if(c >= 'A' && c <= 'F') d[i] = c - 'A' + 10;
        else if(c >= 'a' && c <= 'f') d[i] = c - 'a' + 10;
        else return 0; // error
    }
    return (d[0] << 4) | (d[1]);
}

int stlink_parse_ihex(const char* path, uint8_t erased_pattern, uint8_t * * mem, size_t * size, uint32_t * begin) {
    int res = 0;
    *begin = UINT32_MAX;
    uint8_t* data = NULL;
    uint32_t end = 0;
    bool eof_found = false;

    for(int scan = 0; (res == 0) && (scan < 2); ++scan) { // parse file two times - first to find memory range, second - to fill it
        if(scan == 1) {
            if(!eof_found) {
                ELOG("No EoF recond\n");
                res = -1;
                break;
            }

            if(*begin >= end) {
                ELOG("No data found in file\n");
                res = -1;
                break;
            }

            *size = (end - *begin) + 1;
            data = calloc(*size, 1); // use calloc to get NULL if out of memory
            if(!data) {
                ELOG("Cannot allocate %d bytes\n", *size);
                res = -1;
                break;
            }

            memset(data, erased_pattern, *size);
        }

        FILE* file = fopen(path, "r");
        if(!file) {
            ELOG("Cannot open file\n");
            res = -1;
            break;
        }

        uint32_t lba = 0;

        char line[1 + 5*2 + 255*2 + 2];
        while(fgets(line, sizeof(line), file)) {
            if(line[0] == '\n' || line[0] == '\r') continue; // skip empty lines
            if(line[0] != ':') { // no marker - wrong file format
                ELOG("Wrong file format - no marker\n");
                res = -1;
                break;
            }

            size_t l = strlen(line);
            while(l > 0 && (line[l-1] == '\n' || line[l-1] == '\r')) --l; // trim EoL
            if((l < 11) || (l == (sizeof(line)-1))) { // line too short or long - wrong file format
                ELOG("Wrong file format - wrong line length\n");
                res = -1;
                break;
            }

            // check sum
            uint8_t chksum = 0;
            for(size_t i = 1; i < l; i += 2) {
                chksum += stlink_parse_hex(line + i);
            }
            if(chksum != 0) {
                ELOG("Wrong file format - checksum mismatch\n");
                res = -1;
                break;
            }

            uint8_t reclen = stlink_parse_hex(line + 1);
            if(((uint32_t)reclen + 5)*2 + 1 != l) {
                ELOG("Wrong file format - record length mismatch\n");
                res = -1;
                break;
            }

            uint16_t offset  = ((uint16_t)stlink_parse_hex(line + 3) << 8) | ((uint16_t)stlink_parse_hex(line + 5));
            uint8_t  rectype = stlink_parse_hex(line + 7);

            switch(rectype) {
                case 0: // data
                    if(scan == 0) {
                        uint32_t b = lba + offset;
                        uint32_t e = b + reclen - 1;
                        if(b < *begin) *begin = b;
                        if(e > end) end = e;
                    }
                    else {
                        for(uint8_t i = 0; i < reclen; ++i) {
                            uint8_t b = stlink_parse_hex(line + 9 + i*2);
                            uint32_t addr = lba + offset + i;
                            if(addr >= *begin && addr <= end) {
                                data[addr - *begin] = b;
                            }
                        }
                    }
                    break;

                case 1: // EoF
                    eof_found = true;
                    break;

                case 2: // Extended Segment Address, unexpected
                    res = -1;
                    break;

                case 3: // Start Segment Address, unexpected
                    res = -1;
                    break;

                case 4: // Extended Linear Address
                    if(reclen == 2) {
                        lba = ((uint32_t)stlink_parse_hex(line + 9) << 24) | ((uint32_t)stlink_parse_hex(line + 11) << 16);
                    }
                    else {
                        ELOG("Wrong file format - wrong LBA length\n");
                        res = -1;
                    }
                    break;

                case 5: // Start Linear Address - expected, but ignore
                    break;

                default:
                    ELOG("Wrong file format - unexpected record type %d\n", rectype);
                    res = -1;
            }
            if(res != 0) break;
        }

        fclose(file);
    }

    if(res == 0) {
        *mem = data;
    }
    else {
        free(data);
    }

    return res;
}

uint8_t stlink_get_erased_pattern(stlink_t *sl) {
    if (sl->flash_type == STLINK_FLASH_TYPE_L0)
        return 0x00;
    else
        return 0xff;
}

int stlink_mwrite_flash(stlink_t *sl, uint8_t* data, uint32_t length, stm32_addr_t addr) {
    /* write the block in flash at addr */
    int err;
    unsigned int num_empty, idx;
    uint8_t erased_pattern = stlink_get_erased_pattern(sl);

    idx = (unsigned int)length;
    for(num_empty = 0; num_empty != length; ++num_empty) {
        if (data[--idx] != erased_pattern) {
            break;
        }
    }
    /* Round down to words */
    num_empty -= (num_empty & 3);
    if(num_empty != 0) {
        ILOG("Ignoring %d bytes of 0x%02x at end of file\n", num_empty, erased_pattern);
    }
    err = stlink_write_flash(sl, addr, data, (num_empty == length) ? (uint32_t) length : (uint32_t) length - num_empty, num_empty == length);
    stlink_fwrite_finalize(sl, addr);
    return err;
}

/**
 * Write the given binary file into flash at address "addr"
 * @param sl
 * @param path readable file path, should be binary image
 * @param addr where to start writing
 * @return 0 on success, -ve on failure.
 */
int stlink_fwrite_flash(stlink_t *sl, const char* path, stm32_addr_t addr) {
    /* write the file in flash at addr */
    int err;
    unsigned int num_empty, idx;
    uint8_t erased_pattern = stlink_get_erased_pattern(sl);
    mapped_file_t mf = MAPPED_FILE_INITIALIZER;

    if (map_file(&mf, path) == -1) {
        ELOG("map_file() == -1\n");
        return -1;
    }

    idx = (unsigned int) mf.len;
    for(num_empty = 0; num_empty != mf.len; ++num_empty) {
        if (mf.base[--idx] != erased_pattern) {
            break;
        }
    }
    /* Round down to words */
    num_empty -= (num_empty & 3);
    if(num_empty != 0) {
        ILOG("Ignoring %d bytes of 0x%02x at end of file\n", num_empty, erased_pattern);
    }
    err = stlink_write_flash(sl, addr, mf.base, (num_empty == mf.len) ? (uint32_t) mf.len : (uint32_t) mf.len - num_empty, num_empty == mf.len);
    stlink_fwrite_finalize(sl, addr);
    unmap_file(&mf);
    return err;
}
