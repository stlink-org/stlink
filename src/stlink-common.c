#define DEBUG_FLASH 0

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "mmap.h"

#include "stlink-common.h"
#include "uglylogging.h"

#ifndef _WIN32
#define O_BINARY 0
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

//STM32L0x flash register base and offsets
//same as 32L1 above
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
    return stlink_read_debug32(sl, FLASH_WRPR) & 0xff;
}

static inline uint32_t read_flash_wrpr(stlink_t *sl) {
    return stlink_read_debug32(sl, FLASH_WRPR);
}

static inline uint32_t read_flash_obr(stlink_t *sl) {
    return stlink_read_debug32(sl, FLASH_OBR);
}

static inline uint32_t read_flash_cr(stlink_t *sl) {
    uint32_t res;
    if ((sl->chip_id == STM32_CHIPID_F2) || (sl->chip_id == STM32_CHIPID_F4) ||(sl->chip_id == STM32_CHIPID_F4_DE) ||
            (sl->chip_id == STM32_CHIPID_F4_LP) || (sl->chip_id == STM32_CHIPID_F4_HD) || (sl->chip_id == STM32_CHIPID_F411RE))
        res = stlink_read_debug32(sl, FLASH_F4_CR);
    else
        res = stlink_read_debug32(sl, FLASH_CR);
#if DEBUG_FLASH
    fprintf(stdout, "CR:0x%x\n", res);
#endif
    return res;
}

static inline unsigned int is_flash_locked(stlink_t *sl) {
    /* return non zero for true */
    if ((sl->chip_id == STM32_CHIPID_F2) || (sl->chip_id == STM32_CHIPID_F4) || (sl->chip_id == STM32_CHIPID_F4_DE) ||
            (sl->chip_id == STM32_CHIPID_F4_LP) || (sl->chip_id == STM32_CHIPID_F4_HD) || (sl->chip_id == STM32_CHIPID_F411RE) )
        return read_flash_cr(sl) & (1 << FLASH_F4_CR_LOCK);
    else
        return read_flash_cr(sl) & (1 << FLASH_CR_LOCK);
}

static void unlock_flash(stlink_t *sl) {
    /* the unlock sequence consists of 2 write cycles where
       2 key values are written to the FLASH_KEYR register.
       an invalid sequence results in a definitive lock of
       the FPEC block until next reset.
       */
    if ((sl->chip_id == STM32_CHIPID_F2) || (sl->chip_id == STM32_CHIPID_F4) || (sl->chip_id == STM32_CHIPID_F4_DE) ||
            (sl->chip_id == STM32_CHIPID_F4_LP) || (sl->chip_id == STM32_CHIPID_F4_HD) || (sl->chip_id == STM32_CHIPID_F411RE)) {
        stlink_write_debug32(sl, FLASH_F4_KEYR, FLASH_KEY1);
        stlink_write_debug32(sl, FLASH_F4_KEYR, FLASH_KEY2);
    } else {
        stlink_write_debug32(sl, FLASH_KEYR, FLASH_KEY1);
        stlink_write_debug32(sl, FLASH_KEYR, FLASH_KEY2);
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
    if ((sl->chip_id == STM32_CHIPID_F2) || (sl->chip_id == STM32_CHIPID_F4) || (sl->chip_id == STM32_CHIPID_F4_DE) ||
            (sl->chip_id == STM32_CHIPID_F4_LP) || (sl->chip_id == STM32_CHIPID_F4_HD) || (sl->chip_id == STM32_CHIPID_F411RE)) {
        const uint32_t n = read_flash_cr(sl) | (1 << FLASH_F4_CR_LOCK);
        stlink_write_debug32(sl, FLASH_F4_CR, n);
    } else {
        /* write to 1 only. reset by hw at unlock sequence */
        const uint32_t n = read_flash_cr(sl) | (1 << FLASH_CR_LOCK);
        stlink_write_debug32(sl, FLASH_CR, n);
    }
}


static void set_flash_cr_pg(stlink_t *sl) {
    if ((sl->chip_id == STM32_CHIPID_F2) || (sl->chip_id == STM32_CHIPID_F4) || (sl->chip_id == STM32_CHIPID_F4_DE) ||
            (sl->chip_id == STM32_CHIPID_F4_LP) || (sl->chip_id == STM32_CHIPID_F4_HD) || (sl->chip_id == STM32_CHIPID_F411RE)) {
        uint32_t x = read_flash_cr(sl);
        x |= (1 << FLASH_CR_PG);
        stlink_write_debug32(sl, FLASH_F4_CR, x);
    } else {
        const uint32_t n = 1 << FLASH_CR_PG;
        stlink_write_debug32(sl, FLASH_CR, n);
    }
}

static void __attribute__((unused)) clear_flash_cr_pg(stlink_t *sl) {
    const uint32_t n = read_flash_cr(sl) & ~(1 << FLASH_CR_PG);
    if ((sl->chip_id == STM32_CHIPID_F2) || (sl->chip_id == STM32_CHIPID_F4) || (sl->chip_id == STM32_CHIPID_F4_DE) ||
            (sl->chip_id == STM32_CHIPID_F4_LP) || (sl->chip_id == STM32_CHIPID_F4_HD) || (sl->chip_id == STM32_CHIPID_F411RE))
        stlink_write_debug32(sl, FLASH_F4_CR, n);
    else
        stlink_write_debug32(sl, FLASH_CR, n);
}

static void set_flash_cr_per(stlink_t *sl) {
    const uint32_t n = 1 << FLASH_CR_PER;
    stlink_write_debug32(sl, FLASH_CR, n);
}

static void __attribute__((unused)) clear_flash_cr_per(stlink_t *sl) {
    const uint32_t n = read_flash_cr(sl) & ~(1 << FLASH_CR_PER);
    stlink_write_debug32(sl, FLASH_CR, n);
}

static void set_flash_cr_mer(stlink_t *sl) {
    if ((sl->chip_id == STM32_CHIPID_F2) || (sl->chip_id == STM32_CHIPID_F4) || (sl->chip_id == STM32_CHIPID_F4_DE) ||
            (sl->chip_id == STM32_CHIPID_F4_LP) || (sl->chip_id == STM32_CHIPID_F4_HD) || (sl->chip_id == STM32_CHIPID_F411RE))
        stlink_write_debug32(sl, FLASH_F4_CR,
                stlink_read_debug32(sl, FLASH_F4_CR) | (1 << FLASH_CR_MER));
    else
        stlink_write_debug32(sl, FLASH_CR,
                stlink_read_debug32(sl, FLASH_CR) | (1 << FLASH_CR_MER));
}

static void __attribute__((unused)) clear_flash_cr_mer(stlink_t *sl) {
    if ((sl->chip_id == STM32_CHIPID_F2) || (sl->chip_id == STM32_CHIPID_F4) || (sl->chip_id == STM32_CHIPID_F4_DE) ||
            (sl->chip_id == STM32_CHIPID_F4_LP) || (sl->chip_id == STM32_CHIPID_F4_HD) || (sl->chip_id == STM32_CHIPID_F411RE))
        stlink_write_debug32(sl, FLASH_F4_CR,
                stlink_read_debug32(sl, FLASH_F4_CR) & ~(1 << FLASH_CR_MER));
    else
        stlink_write_debug32(sl, FLASH_CR,
                stlink_read_debug32(sl, FLASH_CR) & ~(1 << FLASH_CR_MER));
}

static void set_flash_cr_strt(stlink_t *sl) {
    if ((sl->chip_id == STM32_CHIPID_F2) || (sl->chip_id == STM32_CHIPID_F4) || (sl->chip_id == STM32_CHIPID_F4_DE) ||
            (sl->chip_id == STM32_CHIPID_F4_LP) || (sl->chip_id == STM32_CHIPID_F4_HD) || (sl->chip_id == STM32_CHIPID_F411RE)) {
        uint32_t x = read_flash_cr(sl);
        x |= (1 << FLASH_F4_CR_STRT);
        stlink_write_debug32(sl, FLASH_F4_CR, x);
    } else {
        stlink_write_debug32(sl, FLASH_CR,
                stlink_read_debug32(sl, FLASH_CR) | (1 << FLASH_CR_STRT) );
    }
}

static inline uint32_t read_flash_acr(stlink_t *sl) {
    return stlink_read_debug32(sl, FLASH_ACR);
}

static inline uint32_t read_flash_sr(stlink_t *sl) {
    uint32_t res;
    if ((sl->chip_id == STM32_CHIPID_F2) || (sl->chip_id == STM32_CHIPID_F4) || (sl->chip_id == STM32_CHIPID_F4_DE) ||
            (sl->chip_id == STM32_CHIPID_F4_LP) || (sl->chip_id == STM32_CHIPID_F4_HD) || (sl->chip_id == STM32_CHIPID_F411RE))
        res = stlink_read_debug32(sl, FLASH_F4_SR);
    else
        res = stlink_read_debug32(sl, FLASH_SR);
    //fprintf(stdout, "SR:0x%x\n", *(uint32_t*) sl->q_buf);
    return res;
}

static inline unsigned int is_flash_busy(stlink_t *sl) {
    if ((sl->chip_id == STM32_CHIPID_F2) || (sl->chip_id == STM32_CHIPID_F4) || (sl->chip_id == STM32_CHIPID_F4_DE) ||
            (sl->chip_id == STM32_CHIPID_F4_LP) || (sl->chip_id == STM32_CHIPID_F4_HD) || (sl->chip_id == STM32_CHIPID_F411RE))
        return read_flash_sr(sl) & (1 << FLASH_F4_SR_BSY);
    else
        return read_flash_sr(sl) & (1 << FLASH_SR_BSY);
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

// Delegates to the backends...

void stlink_close(stlink_t *sl) {
    DLOG("*** stlink_close ***\n");
    sl->backend->close(sl);
    free(sl);
}

void stlink_exit_debug_mode(stlink_t *sl) {
    DLOG("*** stlink_exit_debug_mode ***\n");
    stlink_write_debug32(sl, DHCSR, DBGKEY);
    sl->backend->exit_debug_mode(sl);
}

void stlink_enter_swd_mode(stlink_t *sl) {
    DLOG("*** stlink_enter_swd_mode ***\n");
    sl->backend->enter_swd_mode(sl);
}

// Force the core into the debug mode -> halted state.
void stlink_force_debug(stlink_t *sl) {
    DLOG("*** stlink_force_debug_mode ***\n");
    sl->backend->force_debug(sl);
}

void stlink_exit_dfu_mode(stlink_t *sl) {
    DLOG("*** stlink_exit_dfu_mode ***\n");
    sl->backend->exit_dfu_mode(sl);
}

uint32_t stlink_core_id(stlink_t *sl) {
    DLOG("*** stlink_core_id ***\n");
    sl->backend->core_id(sl);
    if (sl->verbose > 2)
        stlink_print_data(sl);
    DLOG("core_id = 0x%08x\n", sl->core_id);
    return sl->core_id;
}

uint32_t stlink_chip_id(stlink_t *sl) {
    uint32_t chip_id = stlink_read_debug32(sl, 0xE0042000);
    if (chip_id == 0) chip_id = stlink_read_debug32(sl, 0x40015800);	//Try Corex M0 DBGMCU_IDCODE register address
    return chip_id;
}

/**
 * Cortex m3 tech ref manual, CPUID register description
 * @param sl stlink context
 * @param cpuid pointer to the result object
 */
void stlink_cpu_id(stlink_t *sl, cortex_m3_cpuid_t *cpuid) {
    uint32_t raw = stlink_read_debug32(sl, CM3_REG_CPUID);
    cpuid->implementer_id = (raw >> 24) & 0x7f;
    cpuid->variant = (raw >> 20) & 0xf;
    cpuid->part = (raw >> 4) & 0xfff;
    cpuid->revision = raw & 0xf;
    return;
}

/**
 * reads and decodes the flash parameters, as dynamically as possible
 * @param sl
 * @return 0 for success, or -1 for unsupported core type.
 */
int stlink_load_device_params(stlink_t *sl) {
    ILOG("Loading device parameters....\n");
    const chip_params_t *params = NULL;
    sl->core_id = stlink_core_id(sl);
    uint32_t chip_id = stlink_chip_id(sl);
    uint32_t flash_size;

    sl->chip_id = chip_id & 0xfff;
    /* Fix chip_id for F4 rev A errata , Read CPU ID, as CoreID is the same for F2/F4*/
    if (sl->chip_id == 0x411) {
        uint32_t cpuid = stlink_read_debug32(sl, 0xE000ED00);
        if ((cpuid  & 0xfff0) == 0xc240)
            sl->chip_id = 0x413;
    }

    for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
        if(devices[i].chip_id == sl->chip_id) {
            params = &devices[i];
            break;
        }
    }
    if (params == NULL) {
        WLOG("unknown chip id! %#x\n", chip_id);
        return -1;
    }

    // These are fixed...
    sl->flash_base = STM32_FLASH_BASE;
    sl->sram_base = STM32_SRAM_BASE;
    flash_size = stlink_read_debug32(sl,(params->flash_size_reg) & ~3);
    if (params->flash_size_reg & 2)
        flash_size = flash_size >>16;
    flash_size = flash_size & 0xffff;

    if ((sl->chip_id == STM32_CHIPID_L1_MEDIUM || sl->chip_id == STM32_CHIPID_L1_MEDIUM_PLUS) && ( flash_size == 0 )) {
        sl->flash_size = 128 * 1024;
    } else if ((sl->chip_id & 0xFFF) == STM32_CHIPID_L1_HIGH) {
        // 0 is 384k and 1 is 256k
        if ( flash_size == 0 ) {
            sl->flash_size = 384 * 1024;
        } else {
            sl->flash_size = 256 * 1024;
        }
    } else {
        sl->flash_size = flash_size * 1024;
    }
    sl->flash_pgsz = params->flash_pagesize;
    sl->sram_size = params->sram_size;
    sl->sys_base = params->bootrom_base;
    sl->sys_size = params->bootrom_size;

    ILOG("Device connected is: %s, id %#x\n", params->description, chip_id);
    // TODO make note of variable page size here.....
    ILOG("SRAM size: %#x bytes (%d KiB), Flash: %#x bytes (%d KiB) in pages of %zd bytes\n",
            sl->sram_size, sl->sram_size / 1024, sl->flash_size, sl->flash_size / 1024,
            sl->flash_pgsz);
    return 0;
}

void stlink_reset(stlink_t *sl) {
    DLOG("*** stlink_reset ***\n");
    sl->backend->reset(sl);
}

void stlink_jtag_reset(stlink_t *sl, int value) {
    DLOG("*** stlink_jtag_reset ***\n");
    sl->backend->jtag_reset(sl, value);
}

void stlink_run(stlink_t *sl) {
    DLOG("*** stlink_run ***\n");
    sl->backend->run(sl);
}

void stlink_status(stlink_t *sl) {
    DLOG("*** stlink_status ***\n");
    sl->backend->status(sl);
    stlink_core_stat(sl);
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

void stlink_version(stlink_t *sl) {
    DLOG("*** looking up stlink version\n");
    sl->backend->version(sl);
    _parse_version(sl, &sl->version);

    DLOG("st vid         = 0x%04x (expect 0x%04x)\n", sl->version.st_vid, USB_ST_VID);
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

uint32_t stlink_read_debug32(stlink_t *sl, uint32_t addr) {
    uint32_t data = sl->backend->read_debug32(sl, addr);
    DLOG("*** stlink_read_debug32 %x is %#x\n", data, addr);
    return data;
}

void stlink_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data) {
    DLOG("*** stlink_write_debug32 %x to %#x\n", data, addr);
    sl->backend->write_debug32(sl, addr, data);
}

void stlink_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    DLOG("*** stlink_write_mem32 %u bytes to %#x\n", len, addr);
    if (len % 4 != 0) {
        fprintf(stderr, "Error: Data length doesn't have a 32 bit alignment: +%d byte.\n", len % 4);
        abort();
    }
    sl->backend->write_mem32(sl, addr, len);
}

void stlink_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    DLOG("*** stlink_read_mem32 ***\n");
    if (len % 4 != 0) { // !!! never ever: fw gives just wrong values
        fprintf(stderr, "Error: Data length doesn't have a 32 bit alignment: +%d byte.\n",
                len % 4);
        abort();
    }
    sl->backend->read_mem32(sl, addr, len);
}

void stlink_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len) {
    DLOG("*** stlink_write_mem8 ***\n");
    if (len > 0x40 ) { // !!! never ever: Writing more then 0x40 bytes gives unexpected behaviour
        fprintf(stderr, "Error: Data length > 64: +%d byte.\n",
                len);
        abort();
    }
    sl->backend->write_mem8(sl, addr, len);
}

void stlink_read_all_regs(stlink_t *sl, reg *regp) {
    DLOG("*** stlink_read_all_regs ***\n");
    sl->backend->read_all_regs(sl, regp);
}

void stlink_read_all_unsupported_regs(stlink_t *sl, reg *regp) {
    DLOG("*** stlink_read_all_unsupported_regs ***\n");
    sl->backend->read_all_unsupported_regs(sl, regp);
}

void stlink_write_reg(stlink_t *sl, uint32_t reg, int idx) {
    DLOG("*** stlink_write_reg\n");
    sl->backend->write_reg(sl, reg, idx);
}

void stlink_read_reg(stlink_t *sl, int r_idx, reg *regp) {
    DLOG("*** stlink_read_reg\n");
    DLOG(" (%d) ***\n", r_idx);

    if (r_idx > 20 || r_idx < 0) {
        fprintf(stderr, "Error: register index must be in [0..20]\n");
        return;
    }

    sl->backend->read_reg(sl, r_idx, regp);
}

void stlink_read_unsupported_reg(stlink_t *sl, int r_idx, reg *regp) {
    int r_convert;

    DLOG("*** stlink_read_unsupported_reg\n");
    DLOG(" (%d) ***\n", r_idx);

    /* Convert to values used by DCRSR */
    if (r_idx >= 0x1C && r_idx <= 0x1F) { /* primask, basepri, faultmask, or control */
        r_convert = 0x14;
    } else if (r_idx == 0x40) {     /* FPSCR */
        r_convert = 0x21;
    } else if (r_idx >= 0x20 && r_idx < 0x40) {
        r_convert = 0x40 + (r_idx - 0x20);
    } else {
        fprintf(stderr, "Error: register address must be in [0x1C..0x40]\n");
        return;
    }

    sl->backend->read_unsupported_reg(sl, r_convert, regp);
}

void stlink_write_unsupported_reg(stlink_t *sl, uint32_t val, int r_idx, reg *regp) {
    int r_convert;

    DLOG("*** stlink_write_unsupported_reg\n");
    DLOG(" (%d) ***\n", r_idx);

    /* Convert to values used by DCRSR */
    if (r_idx >= 0x1C && r_idx <= 0x1F) { /* primask, basepri, faultmask, or control */
        r_convert = r_idx;  /* The backend function handles this */
    } else if (r_idx == 0x40) {     /* FPSCR */
        r_convert = 0x21;
    } else if (r_idx >= 0x20 && r_idx < 0x40) {
        r_convert = 0x40 + (r_idx - 0x20);
    } else {
        fprintf(stderr, "Error: register address must be in [0x1C..0x40]\n");
        return;
    }

    sl->backend->write_unsupported_reg(sl, val, r_convert, regp);
}

unsigned int is_core_halted(stlink_t *sl) {
    /* return non zero if core is halted */
    stlink_status(sl);
    return sl->q_buf[0] == STLINK_CORE_HALTED;
}

void stlink_step(stlink_t *sl) {
    DLOG("*** stlink_step ***\n");
    sl->backend->step(sl);
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

inline unsigned int is_bigendian(void) {
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

    while (is_core_halted(sl) == 0)
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
    /* set stack*/
    stlink_write_reg(sl, stlink_read_debug32(sl, addr    ),13);
    /* Set PC to the reset routine*/
    stlink_write_reg(sl, stlink_read_debug32(sl, addr + 4),15);
    stlink_run(sl);

on_error:
    unmap_file(&mf);
    return error;
}

int stlink_fread(stlink_t* sl, const char* path, stm32_addr_t addr, size_t size) {
    /* read size bytes from addr to file */

    int error = -1;
    size_t off;
    int num_empty = 0;
    unsigned char erased_pattern = (sl->chip_id == STM32_CHIPID_L1_MEDIUM  || sl->chip_id == STM32_CHIPID_L1_MEDIUM_PLUS
            || sl->chip_id == STM32_CHIPID_L1_HIGH || sl->chip_id == STM32_CHIPID_L152_RE) ? 0:0xff;

    const int fd = open(path, O_RDWR | O_TRUNC | O_CREAT, 00700);
    if (fd == -1) {
        fprintf(stderr, "open(%s) == -1\n", path);
        return -1;
    }

    if (size <1)
        size = sl->flash_size;

    if (size > sl->flash_size)
        size = sl->flash_size;

    /* do the copy by 1k blocks */
    for (off = 0; off < size; off += 1024) {
        size_t read_size = 1024;
        size_t rounded_size;
        size_t index;
        if ((off + read_size) > size)
            read_size = size - off;

        /* round size if needed */
        rounded_size = read_size;
        if (rounded_size & 3)
            rounded_size = (rounded_size + 4) & ~(3);

        stlink_read_mem32(sl, addr + off, rounded_size);

        for(index = 0; index < read_size; index ++) {
            if (sl->q_buf[index] == erased_pattern)
                num_empty ++;
            else
                num_empty = 0;
        }
        if (write(fd, sl->q_buf, read_size) != (ssize_t) read_size) {
            fprintf(stderr, "write() != read_size\n");
            goto on_error;
        }
    }

    /* Ignore NULL Bytes at end of file */
    if (!ftruncate(fd, size - num_empty)) {
        error = -1;
    }

    /* success */
    error = 0;

on_error:
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
        stlink_write_mem8(sl, (fl->buf_addr)+chunk, rem);
    }
    return 0;
}

uint32_t calculate_F4_sectornum(uint32_t flashaddr){
    flashaddr &= ~STM32_FLASH_BASE;	//Page now holding the actual flash address
    if (flashaddr<0x4000) return (0);
    else if(flashaddr<0x8000) return(1);
    else if(flashaddr<0xc000) return(2);
    else if(flashaddr<0x10000) return(3);
    else if(flashaddr<0x20000) return(4);
    else return(flashaddr/0x20000)+4;

}

uint32_t stlink_calculate_pagesize(stlink_t *sl, uint32_t flashaddr){
    if ((sl->chip_id == STM32_CHIPID_F2) || (sl->chip_id == STM32_CHIPID_F4) || (sl->chip_id == STM32_CHIPID_F4_DE) ||
            (sl->chip_id == STM32_CHIPID_F4_LP) || (sl->chip_id == STM32_CHIPID_F4_HD) || (sl->chip_id == STM32_CHIPID_F411RE)) {
        uint32_t sector=calculate_F4_sectornum(flashaddr);
        if (sector<4) sl->flash_pgsz=0x4000;
        else if(sector<5) sl->flash_pgsz=0x10000;
        else sl->flash_pgsz=0x20000;
    }
    return (sl->flash_pgsz);
}

/**
 * Erase a page of flash, assumes sl is fully populated with things like chip/core ids
 * @param sl stlink context
 * @param flashaddr an address in the flash page to erase
 * @return 0 on success -ve on failure
 */
int stlink_erase_flash_page(stlink_t *sl, stm32_addr_t flashaddr)
{
    if ((sl->chip_id == STM32_CHIPID_F2) || (sl->chip_id == STM32_CHIPID_F4) ||  (sl->chip_id == STM32_CHIPID_F4_DE) ||
            (sl->chip_id == STM32_CHIPID_F4_LP) || (sl->chip_id == STM32_CHIPID_F4_HD) || (sl->chip_id == STM32_CHIPID_F411RE)) {
        /* wait for ongoing op to finish */
        wait_flash_busy(sl);

        /* unlock if locked */
        unlock_flash_if(sl);

        /* select the page to erase */
        // calculate the actual page from the address
        uint32_t sector=calculate_F4_sectornum(flashaddr);

        fprintf(stderr, "EraseFlash - Sector:0x%x Size:0x%x\n", sector, stlink_calculate_pagesize(sl, flashaddr));
        write_flash_cr_snb(sl, sector);

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
    } else if (sl->chip_id == STM32_CHIPID_L1_MEDIUM || sl->chip_id == STM32_CHIPID_L1_MEDIUM_PLUS
            || sl->chip_id == STM32_CHIPID_L1_HIGH || sl->chip_id == STM32_CHIPID_L152_RE
            || sl->chip_id == STM32_CHIPID_L0) {

        uint32_t val;
        uint32_t flash_regs_base;
        if (sl->chip_id == STM32_CHIPID_L0) {
            flash_regs_base = STM32L0_FLASH_REGS_ADDR;
        } else {
            flash_regs_base = STM32L_FLASH_REGS_ADDR;
        }

        /* check if the locks are set */
        val = stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF);
        if((val & (1<<0))||(val & (1<<1))) {
            /* disable pecr protection */
            stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF, 0x89abcdef);
            stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF, 0x02030405);

            /* check pecr.pelock is cleared */
            val = stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF);
            if (val & (1 << 0)) {
                WLOG("pecr.pelock not clear (%#x)\n", val);
                return -1;
            }

            /* unlock program memory */
            stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, 0x8c9daebf);
            stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, 0x13141516);

            /* check pecr.prglock is cleared */
            val = stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF);
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
        while ((stlink_read_debug32(sl, STM32L_FLASH_SR) & (1 << 0)) != 0)
            ;

#endif /* fix_to_be_confirmed */

        /* write 0 to the first word of the page to be erased */
        stlink_write_debug32(sl, flashaddr, 0);

        /* MP: It is better to wait for clearing the busy bit after issuing
           page erase command, even though PM0062 recommends to wait before it.
           Test shows that a few iterations is performed in the following loop
           before busy bit is cleared.*/
        while ((stlink_read_debug32(sl, flash_regs_base + FLASH_SR_OFF) & (1 << 0)) != 0)
            ;

        /* reset lock bits */
        val = stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF)
            | (1 << 0) | (1 << 1) | (1 << 2);
        stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
    } else if (sl->core_id == STM32VL_CORE_ID 
            || sl->core_id == STM32F0_CORE_ID 
            || sl->chip_id == STM32_CHIPID_F3 
            || sl->chip_id == STM32_CHIPID_F37x
            || sl->chip_id == STM32_CHIPID_F334)  {
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
    } else {
        WLOG("unknown coreid %x, page erase failed\n", sl->core_id);
        return -1;
    }

    /* todo: verify the erased page */

    return 0;
}

int stlink_erase_flash_mass(stlink_t *sl) {
    if (sl->chip_id == STM32_CHIPID_L1_MEDIUM || sl->chip_id == STM32_CHIPID_L1_MEDIUM_PLUS 
            || sl->chip_id == STM32_CHIPID_L1_HIGH || sl->chip_id == STM32_CHIPID_L152_RE
            || sl->chip_id == STM32_CHIPID_L0) {
        /* erase each page */
        int i = 0, num_pages = sl->flash_size/sl->flash_pgsz;
        for (i = 0; i < num_pages; i++) {
            /* addr must be an addr inside the page */
            stm32_addr_t addr = sl->flash_base + i * sl->flash_pgsz;
            if (stlink_erase_flash_page(sl, addr) == -1) {
                WLOG("Failed to erase_flash_page(%#zx) == -1\n", addr);
                return -1;
            }
            fprintf(stdout,"\rFlash page at %5d/%5d erased", i, num_pages);
            fflush(stdout);
        }
        fprintf(stdout, "\n");
    } else {
        /* wait for ongoing op to finish */
        wait_flash_busy(sl);

        /* unlock if locked */
        unlock_flash_if(sl);

        /* set the mass erase bit */
        set_flash_cr_mer(sl);

        /* start erase operation, reset by hw with bsy bit */
        set_flash_cr_strt(sl);

        /* wait for completion */
        wait_flash_busy_progress(sl);

        /* relock the flash */
        lock_flash(sl);

        /* todo: verify the erased memory */
    }
    return 0;
}

int init_flash_loader(stlink_t *sl, flash_loader_t* fl) {
    size_t size;

    /* allocate the loader in sram */
    if (write_loader_to_sram(sl, &fl->loader_addr, &size) == -1) {
        WLOG("Failed to write flash loader to sram!\n");
        return -1;
    }

    /* allocate a one page buffer in sram right after loader */
    fl->buf_addr = fl->loader_addr + size;
    ILOG("Successfully loaded flash loader in sram\n");
    return 0;
}

int write_loader_to_sram(stlink_t *sl, stm32_addr_t* addr, size_t* size) {
    /* from openocd, contrib/loaders/flash/stm32.s */
    static const uint8_t loader_code_stm32vl[] = {
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

    /* flashloaders/stm32f0.s -- thumb1 only, same sequence as for STM32VL, bank ignored */
    static const uint8_t loader_code_stm32f0[] = {
#if 1
        /*
         * These two NOPs here are a safety precaution, added by Pekka Nikander
         * while debugging the STM32F05x support.  They may not be needed, but
         * there were strange problems with simpler programs, like a program
         * that had just a breakpoint or a program that first moved zero to register r2
         * and then had a breakpoint.  So, it appears safest to have these two nops.
         *
         * Feel free to remove them, if you dare, but then please do test the result
         * rigorously.  Also, if you remove these, it may be a good idea first to
         * #if 0 them out, with a comment when these were taken out, and to remove
         * these only a few months later...  But YMMV.
         */
        0x00, 0x30, //     nop     /* add r0,#0 */
        0x00, 0x30, //     nop     /* add r0,#0 */
#endif
        0x0A, 0x4C, //     ldr     r4, STM32_FLASH_BASE
        0x01, 0x25, //     mov     r5, #1            /*  FLASH_CR_PG, FLASH_SR_BUSY */
        0x04, 0x26, //     mov     r6, #4            /*  PGERR  */
        // write_half_word:
        0x23, 0x69, //     ldr     r3, [r4, #16]     /*  FLASH->CR   */
        0x2B, 0x43, //     orr     r3, r5
        0x23, 0x61, //     str     r3, [r4, #16]     /*  FLASH->CR |= FLASH_CR_PG */
        0x03, 0x88, //     ldrh    r3, [r0]          /*  r3 = *sram */
        0x0B, 0x80, //     strh    r3, [r1]          /*  *flash = r3 */
        // busy:
        0xE3, 0x68, //     ldr	   r3, [r4, #12]     /*  FLASH->SR  */
        0x2B, 0x42, //     tst	   r3, r5            /*  FLASH_SR_BUSY  */
        0xFC, 0xD0, //     beq	   busy

        0x33, 0x42, //     tst	   r3, r6            /*  PGERR  */
        0x04, 0xD1, //     bne	   exit

        0x02, 0x30, //     add     r0, r0, #2        /*  sram += 2  */
        0x02, 0x31, //     add     r1, r1, #2        /*  flash += 2  */
        0x01, 0x3A, //     sub     r2, r2, #0x01     /*  count--  */
        0x00, 0x2A, //     cmp     r2, #0
        0xF0, 0xD1, //     bne	   write_half_word
        // exit:
        0x23, 0x69, //     ldr     r3, [r4, #16]     /*  FLASH->CR  */
        0xAB, 0x43, //     bic     r3, r5
        0x23, 0x61, //     str     r3, [r4, #16]     /*  FLASH->CR &= ~FLASH_CR_PG  */
        0x00, 0xBE, //     bkpt	#0x00
        0x00, 0x20, 0x02, 0x40, /* STM32_FLASH_BASE: .word 0x40022000 */
    };

    static const uint8_t loader_code_stm32l[] = {

        /* openocd.git/contrib/loaders/flash/stm32lx.S
           r0, input, dest addr
           r1, input, source addr
           r2, input, word count
           r3, output, word count
           */

        0x00, 0x23,
        0x04, 0xe0,

        0x51, 0xf8, 0x04, 0xcb,
        0x40, 0xf8, 0x04, 0xcb,
        0x01, 0x33,

        0x93, 0x42,
        0xf8, 0xd3,
        0x00, 0xbe
    };

    static const uint8_t loader_code_stm32l0[] = {

        /*
           r0, input, dest addr
           r1, input, source addr
           r2, input, word count
           r3, output, word count
         */

        0x00, 0x23,
        0x04, 0xe0,

        0x0c, 0x68,
        0x04, 0x66,
        0x01, 0x33,
        0x04, 0x31,
        0x04, 0x30,

        0x93, 0x42,
        0xf8, 0xd3,
        0x00, 0xbe
    };

    static const uint8_t loader_code_stm32f4[] = {
        // flashloaders/stm32f4.s

        0x07, 0x4b,

        0x62, 0xb1,
        0x04, 0x68,
        0x0c, 0x60,

        0xdc, 0x89,
        0x14, 0xf0, 0x01, 0x0f,
        0xfb, 0xd1,
        0x00, 0xf1, 0x04, 0x00,
        0x01, 0xf1, 0x04, 0x01,
        0xa2, 0xf1, 0x01, 0x02,
        0xf1, 0xe7,

        0x00, 0xbe,

        0x00, 0x3c, 0x02, 0x40,
    };

    const uint8_t* loader_code;
    size_t loader_size;

    if (sl->chip_id == STM32_CHIPID_L1_MEDIUM || sl->chip_id == STM32_CHIPID_L1_MEDIUM_PLUS
            || sl->chip_id == STM32_CHIPID_L1_HIGH || sl->chip_id == STM32_CHIPID_L152_RE) { /* stm32l */
        loader_code = loader_code_stm32l;
        loader_size = sizeof(loader_code_stm32l);
    } else if (sl->core_id == STM32VL_CORE_ID || sl->chip_id == STM32_CHIPID_F3  ||
                sl->chip_id == STM32_CHIPID_F37x || sl->chip_id == STM32_CHIPID_F334) {
        loader_code = loader_code_stm32vl;
        loader_size = sizeof(loader_code_stm32vl);
    } else if (sl->chip_id == STM32_CHIPID_F2 || sl->chip_id == STM32_CHIPID_F4 || (sl->chip_id == STM32_CHIPID_F4_DE) ||
            sl->chip_id == STM32_CHIPID_F4_LP || sl->chip_id == STM32_CHIPID_F4_HD || (sl->chip_id == STM32_CHIPID_F411RE)){
        loader_code = loader_code_stm32f4;
        loader_size = sizeof(loader_code_stm32f4);
    } else if (sl->chip_id == STM32_CHIPID_F0 || sl->chip_id == STM32_CHIPID_F0_CAN || sl->chip_id == STM32_CHIPID_F0_SMALL) {
        loader_code = loader_code_stm32f0;
        loader_size = sizeof(loader_code_stm32f0);
    } else if (sl->chip_id == STM32_CHIPID_L0) {
		loader_code = loader_code_stm32l0;
		loader_size = sizeof(loader_code_stm32l0);
	} else {
        ELOG("unknown coreid, not sure what flash loader to use, aborting!: %x\n", sl->core_id);
        return -1;
    }

    memcpy(sl->q_buf, loader_code, loader_size);
    stlink_write_mem32(sl, sl->sram_base, loader_size);

    *addr = sl->sram_base;
    *size = loader_size;

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

        stlink_read_mem32(sl, address + off, aligned_size);

        if (memcmp(sl->q_buf, data + off, cmp_size)) {
            ELOG("Verification of flash failed at offset: %zd\n", off);
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

    if (sl->chip_id == STM32_CHIPID_L0) {
        flash_regs_base = STM32L0_FLASH_REGS_ADDR;
    } else {
        flash_regs_base = STM32L_FLASH_REGS_ADDR;
    }

    ILOG("Starting Half page flash write for STM32L core id\n");
    /* flash loader initialization */
    if (init_flash_loader(sl, &fl) == -1) {
        WLOG("init_flash_loader() == -1\n");
        return -1;
    }
    /* Unlock already done */
    val = stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF);
    val |= (1 << FLASH_L1_FPRG);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);

    val |= (1 << FLASH_L1_PROG);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
    while ((stlink_read_debug32(sl, flash_regs_base + FLASH_SR_OFF) & (1 << 0)) != 0) {}

    for (count = 0; count  < num_half_pages; count ++) {
        if (run_flash_loader(sl, &fl, addr + count * pagesize, base + count * pagesize, pagesize) == -1) {
            WLOG("l1_run_flash_loader(%#zx) failed! == -1\n", addr + count * pagesize);
            val = stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF);
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
        while ((stlink_read_debug32(sl, flash_regs_base + FLASH_SR_OFF) & (1 << 0)) != 0) {
        }
    }
    val = stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF);
    val &= ~(1 << FLASH_L1_PROG);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
    val = stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF);
    val &= ~(1 << FLASH_L1_FPRG);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);

    return 0;
}

int stlink_write_flash(stlink_t *sl, stm32_addr_t addr, uint8_t* base, uint32_t len) {
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
    for (off = 0; off < len; off += stlink_calculate_pagesize(sl, addr + off)) {
        /* addr must be an addr inside the page */
        if (stlink_erase_flash_page(sl, addr + off) == -1) {
            ELOG("Failed to erase_flash_page(%#zx) == -1\n", addr + off);
            return -1;
        }
        fprintf(stdout,"\rFlash page at addr: 0x%08lx erased",
                (unsigned long)addr + off);
        fflush(stdout);
        page_count++;
    }
    fprintf(stdout,"\n");
    ILOG("Finished erasing %d pages of %d (%#x) bytes\n",
            page_count, sl->flash_pgsz, sl->flash_pgsz);

    if ((sl->chip_id == STM32_CHIPID_F2) ||
        (sl->chip_id == STM32_CHIPID_F4) ||
        (sl->chip_id == STM32_CHIPID_F4_DE) ||
        (sl->chip_id == STM32_CHIPID_F4_LP) ||
        (sl->chip_id == STM32_CHIPID_F4_HD) ||
        (sl->chip_id == STM32_CHIPID_F411RE)) {
        /* todo: check write operation */

        ILOG("Starting Flash write for F2/F4\n");
        /* flash loader initialization */
        if (init_flash_loader(sl, &fl) == -1) {
            ELOG("init_flash_loader() == -1\n");
            return -1;
        }

        /* First unlock the cr */
        unlock_flash_if(sl);

        /* TODO: Check that Voltage range is 2.7 - 3.6 V */
        /* set parallelisim to 32 bit*/
        write_flash_cr_psiz(sl, 2);

        /* set programming mode */
        set_flash_cr_pg(sl);

        for(off = 0; off < len;) {
            size_t size = len - off > 0x8000 ? 0x8000 : len - off;

            printf("size: %zu\n", size);

            if (run_flash_loader(sl, &fl, addr + off, base + off, size) == -1) {
                ELOG("run_flash_loader(%#zx) failed! == -1\n", addr + off);
                return -1;
            }

            off += size;
        }

        /* Relock flash */
        lock_flash(sl);

    }	//STM32F4END

    else if (sl->chip_id == STM32_CHIPID_L1_MEDIUM || sl->chip_id == STM32_CHIPID_L1_MEDIUM_PLUS
            || sl->chip_id == STM32_CHIPID_L1_HIGH || sl->chip_id == STM32_CHIPID_L152_RE
            || sl->chip_id == STM32_CHIPID_L0) {
        /* use fast word write. todo: half page. */
        uint32_t val;
        uint32_t flash_regs_base;
        uint32_t pagesize;

        if (sl->chip_id == STM32_CHIPID_L0) {
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
        val = stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF);
        if (val & (1 << 0)) {
            fprintf(stderr, "pecr.pelock not clear\n");
            return -1;
        }

        /* unlock program memory */
        stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, 0x8c9daebf);
        stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, 0x13141516);

        /* check pecr.prglock is cleared */
        val = stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF);
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
                fprintf(stdout, "\r%3zd/%3zd pages written",
                        off/sl->flash_pgsz, len/sl->flash_pgsz);
                fflush(stdout);
            }

            write_uint32((unsigned char*) &data, *(uint32_t*) (base + off));
            stlink_write_debug32(sl, addr + off, data);

            /* wait for sr.busy to be cleared */
            while ((stlink_read_debug32(sl, flash_regs_base + FLASH_SR_OFF) & (1 << 0)) != 0)
                ;

            /* todo: check redo write operation */

        }
        fprintf(stdout, "\n");
        /* reset lock bits */
        val = stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF)
            | (1 << 0) | (1 << 1) | (1 << 2);
        stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
    } else if (sl->core_id == STM32VL_CORE_ID ||
                sl->core_id == STM32F0_CORE_ID ||
                sl->chip_id == STM32_CHIPID_F3  ||
                sl->chip_id == STM32_CHIPID_F334 ||
                sl->chip_id == STM32_CHIPID_F37x) {
        ILOG("Starting Flash write for VL/F0/F3 core id\n");
        /* flash loader initialization */
        if (init_flash_loader(sl, &fl) == -1) {
            ELOG("init_flash_loader() == -1\n");
            return -1;
        }

        int write_block_count = 0;
        for (off = 0; off < len; off += sl->flash_pgsz) {
            /* adjust last write size */
            size_t size = sl->flash_pgsz;
            if ((off + sl->flash_pgsz) > len) size = len - off;

            /* unlock and set programming mode */
            unlock_flash_if(sl);
            set_flash_cr_pg(sl);
            //DLOG("Finished setting flash cr pg, running loader!\n");
            if (run_flash_loader(sl, &fl, addr + off, base + off, size) == -1) {
                ELOG("run_flash_loader(%#zx) failed! == -1\n", addr + off);
                return -1;
            }
            lock_flash(sl);
            if (sl->verbose >= 1) {
                /* show progress. writing procedure is slow
                   and previous errors are misleading */
                fprintf(stdout, "\r%3u/%lu pages written", write_block_count++, (unsigned long)len/sl->flash_pgsz);
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
    unsigned int num_empty = 0, index;
    unsigned char erased_pattern =(sl->chip_id == STM32_CHIPID_L1_MEDIUM || sl->chip_id == STM32_CHIPID_L1_MEDIUM_PLUS
            || sl->chip_id == STM32_CHIPID_L1_HIGH || sl->chip_id == STM32_CHIPID_L152_RE )?0:0xff;
    mapped_file_t mf = MAPPED_FILE_INITIALIZER;
    if (map_file(&mf, path) == -1) {
        ELOG("map_file() == -1\n");
        return -1;
    }
    for(index = 0; index < mf.len; index ++) {
        if (mf.base[index] == erased_pattern)
            num_empty ++;
        else
            num_empty = 0;
    }
    /* Round down to words */
    num_empty -= (num_empty & 3);
    if(num_empty != 0) {
        ILOG("Ignoring %d bytes of 0x%02x at end of file\n", num_empty, erased_pattern);
        mf.len -= num_empty;
    }
    err = stlink_write_flash(sl, addr, mf.base, mf.len);
    /* set stack*/
    stlink_write_reg(sl, stlink_read_debug32(sl, addr    ),13);
    /* Set PC to the reset routine*/
    stlink_write_reg(sl, stlink_read_debug32(sl, addr + 4),15);
    stlink_run(sl);
    unmap_file(&mf);
    return err;
}

int run_flash_loader(stlink_t *sl, flash_loader_t* fl, stm32_addr_t target, const uint8_t* buf, size_t size) {

    reg rr;
    int i = 0;
    DLOG("Running flash loader, write address:%#x, size: %zd\n", target, size);
    // FIXME This can never return -1
    if (write_buffer_to_sram(sl, fl, buf, size) == -1) {
        // IMPOSSIBLE!
        ELOG("write_buffer_to_sram() == -1\n");
        return -1;
    }

    if (sl->chip_id == STM32_CHIPID_L1_MEDIUM  || sl->chip_id == STM32_CHIPID_L1_MEDIUM_PLUS
            || sl->chip_id == STM32_CHIPID_L1_HIGH || sl->chip_id == STM32_CHIPID_L152_RE
            || sl->chip_id == STM32_CHIPID_L0) {

        size_t count = size / sizeof(uint32_t);
        if (size % sizeof(uint32_t)) ++count;

        /* setup core */
        stlink_write_reg(sl, target, 0); /* target */
        stlink_write_reg(sl, fl->buf_addr, 1); /* source */
        stlink_write_reg(sl, count, 2); /* count (32 bits words) */
        stlink_write_reg(sl, fl->loader_addr, 15); /* pc register */

    } else if (sl->core_id == STM32VL_CORE_ID ||
                sl->core_id == STM32F0_CORE_ID ||
                sl->chip_id == STM32_CHIPID_F3 ||
                sl->chip_id == STM32_CHIPID_F37x ||
                sl->chip_id == STM32_CHIPID_F334) {

        size_t count = size / sizeof(uint16_t);
        if (size % sizeof(uint16_t)) ++count;

        /* setup core */
        stlink_write_reg(sl, fl->buf_addr, 0); /* source */
        stlink_write_reg(sl, target, 1); /* target */
        stlink_write_reg(sl, count, 2); /* count (16 bits half words) */
        stlink_write_reg(sl, 0, 3); /* flash bank 0 (input) */
        stlink_write_reg(sl, fl->loader_addr, 15); /* pc register */

    } else if (sl->chip_id == STM32_CHIPID_F2 || sl->chip_id == STM32_CHIPID_F4 || (sl->chip_id == STM32_CHIPID_F4_DE) ||
            sl->chip_id == STM32_CHIPID_F4_LP || sl->chip_id == STM32_CHIPID_F4_HD || (sl->chip_id == STM32_CHIPID_F411RE)) {

        size_t count = size / sizeof(uint32_t);
        if (size % sizeof(uint32_t)) ++count;

        /* setup core */
        stlink_write_reg(sl, fl->buf_addr, 0); /* source */
        stlink_write_reg(sl, target, 1); /* target */
        stlink_write_reg(sl, count, 2); /* count (32 bits words) */
        stlink_write_reg(sl, fl->loader_addr, 15); /* pc register */

    } else {
        fprintf(stderr, "unknown coreid 0x%x, don't know what flash loader to use\n", sl->core_id);
        return -1;
    }

    /* run loader */
    stlink_run(sl);

#define WAIT_ROUNDS 10000
    /* wait until done (reaches breakpoint) */
    for (i = 0; i < WAIT_ROUNDS; i++) {
        usleep(10);
        if (is_core_halted(sl))
            break;
    }

    if (i >= WAIT_ROUNDS) {
        ELOG("flash loader run error\n");
        return -1;
    }

    /* check written byte count */
    if (sl->chip_id == STM32_CHIPID_L1_MEDIUM || sl->chip_id == STM32_CHIPID_L1_MEDIUM_PLUS
            || sl->chip_id == STM32_CHIPID_L1_HIGH || sl->chip_id == STM32_CHIPID_L152_RE
            || sl->chip_id == STM32_CHIPID_L0) {

        size_t count = size / sizeof(uint32_t);
        if (size % sizeof(uint32_t)) ++count;

        stlink_read_reg(sl, 3, &rr);
        if (rr.r[3] != count) {
            fprintf(stderr, "write error, count == %u\n", rr.r[3]);
            return -1;
        }

    } else if (sl->core_id == STM32VL_CORE_ID ||
                sl->core_id == STM32F0_CORE_ID ||
                sl->chip_id == STM32_CHIPID_F3 ||
                sl->chip_id == STM32_CHIPID_F37x ||
                sl->chip_id == STM32_CHIPID_F334) {

        stlink_read_reg(sl, 2, &rr);
        if (rr.r[2] != 0) {
            fprintf(stderr, "write error, count == %u\n", rr.r[2]);
            return -1;
        }

    } else if (sl->chip_id == STM32_CHIPID_F2 || sl->chip_id == STM32_CHIPID_F4 || (sl->chip_id == STM32_CHIPID_F4_DE) ||
            sl->chip_id == STM32_CHIPID_F4_LP || sl->chip_id == STM32_CHIPID_F4_HD || (sl->chip_id == STM32_CHIPID_F411RE)) {

        stlink_read_reg(sl, 2, &rr);
        if (rr.r[2] != 0) {
            fprintf(stderr, "write error, count == %u\n", rr.r[2]);
            return -1;
        }

    } else {

        fprintf(stderr, "unknown coreid 0x%x, can't check written byte count\n", sl->core_id);
        return -1;

    }

    return 0;
}
