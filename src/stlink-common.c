#define DEBUG_FLASH 0

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
#include "uglylogging.h"

#define LOG_TAG __FILE__
#define DLOG(format, args...)         ugly_log(UDEBUG, LOG_TAG, format, ## args)
#define ILOG(format, args...)         ugly_log(UINFO, LOG_TAG, format, ## args)
#define WLOG(format, args...)         ugly_log(UWARN, LOG_TAG, format, ## args)
#define fatal(format, args...)        ugly_log(UFATAL, LOG_TAG, format, ## args)

/* todo: stm32l15xxx flash memory, pm0062 manual */

/* stm32f FPEC flash controller interface, pm0063 manual */
// TODO - all of this needs to be abstracted out....
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


//32L = 32F1 same CoreID as 32F4!
#define STM32L_FLASH_REGS_ADDR ((uint32_t)0x40023c00)
#define STM32L_FLASH_ACR (STM32L_FLASH_REGS_ADDR + 0x00)
#define STM32L_FLASH_PECR (STM32L_FLASH_REGS_ADDR + 0x04)
#define STM32L_FLASH_PDKEYR (STM32L_FLASH_REGS_ADDR + 0x08)
#define STM32L_FLASH_PEKEYR (STM32L_FLASH_REGS_ADDR + 0x0c)
#define STM32L_FLASH_PRGKEYR (STM32L_FLASH_REGS_ADDR + 0x10)
#define STM32L_FLASH_OPTKEYR (STM32L_FLASH_REGS_ADDR + 0x14)
#define STM32L_FLASH_SR (STM32L_FLASH_REGS_ADDR + 0x18)
#define STM32L_FLASH_OBR (STM32L_FLASH_REGS_ADDR + 0x0c)
#define STM32L_FLASH_WRPR (STM32L_FLASH_REGS_ADDR + 0x20)


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
#define FLASH_F4_CR_SNB_MASK 0x38
#define FLASH_F4_SR_BSY 16


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
    stlink_read_mem32(sl, FLASH_WRPR, sizeof (uint32_t));
    return read_uint32(sl->q_buf, 0) & 0xff;
}

static inline uint32_t read_flash_wrpr(stlink_t *sl) {
    stlink_read_mem32(sl, FLASH_WRPR, sizeof (uint32_t));
    return read_uint32(sl->q_buf, 0);
}

static inline uint32_t read_flash_obr(stlink_t *sl) {
    stlink_read_mem32(sl, FLASH_OBR, sizeof (uint32_t));
    return read_uint32(sl->q_buf, 0);
}

static inline uint32_t read_flash_cr(stlink_t *sl) {
	if(sl->chip_id==STM32F4_CHIP_ID)
		stlink_read_mem32(sl, FLASH_F4_CR, sizeof (uint32_t));
	else
		stlink_read_mem32(sl, FLASH_CR, sizeof (uint32_t));
#if DEBUG_FLASH
	fprintf(stdout, "CR:0x%x\n", *(uint32_t*) sl->q_buf);
#endif
	return read_uint32(sl->q_buf, 0);
}

static inline unsigned int is_flash_locked(stlink_t *sl) {
    /* return non zero for true */
	if(sl->chip_id==STM32F4_CHIP_ID)
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
    if(sl->chip_id==STM32F4_CHIP_ID) {
        write_uint32(sl->q_buf, FLASH_KEY1);
    	stlink_write_mem32(sl, FLASH_F4_KEYR, sizeof (uint32_t));
		write_uint32(sl->q_buf, FLASH_KEY2);
		stlink_write_mem32(sl, FLASH_F4_KEYR, sizeof (uint32_t));
    }
	else {
        write_uint32(sl->q_buf, FLASH_KEY1);
    	stlink_write_mem32(sl, FLASH_KEYR, sizeof (uint32_t));
		write_uint32(sl->q_buf, FLASH_KEY2);
		stlink_write_mem32(sl, FLASH_KEYR, sizeof (uint32_t));
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
    ILOG("Successfully unlocked flash\n");
    return 0;
}

static void lock_flash(stlink_t *sl) {
    if(sl->chip_id==STM32F4_CHIP_ID) {
    	const uint32_t n = read_flash_cr(sl) | (1 << FLASH_F4_CR_LOCK);
        write_uint32(sl->q_buf, n);
    	stlink_write_mem32(sl, FLASH_F4_CR, sizeof (uint32_t));
    }
    else {
        /* write to 1 only. reset by hw at unlock sequence */
        const uint32_t n = read_flash_cr(sl) | (1 << FLASH_CR_LOCK);
        write_uint32(sl->q_buf, n);
        stlink_write_mem32(sl, FLASH_CR, sizeof (uint32_t));
    }
}


static void set_flash_cr_pg(stlink_t *sl) {
    if(sl->chip_id==STM32F4_CHIP_ID) {
		uint32_t x = read_flash_cr(sl);
		x |= (1 << FLASH_CR_PG);
		write_uint32(sl->q_buf, x);
    	stlink_write_mem32(sl, FLASH_F4_CR, sizeof (uint32_t));
    }
    else {
        const uint32_t n = 1 << FLASH_CR_PG;
        write_uint32(sl->q_buf, n);
        stlink_write_mem32(sl, FLASH_CR, sizeof (uint32_t));
    }
}

static void __attribute__((unused)) clear_flash_cr_pg(stlink_t *sl) {
    const uint32_t n = read_flash_cr(sl) & ~(1 << FLASH_CR_PG);
    write_uint32(sl->q_buf, n);
    if(sl->chip_id==STM32F4_CHIP_ID)
    	stlink_write_mem32(sl, FLASH_F4_CR, sizeof (uint32_t));
    else
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
	if(sl->chip_id == STM32F4_CHIP_ID)
	{
		uint32_t x = read_flash_cr(sl);
		x |= (1 << FLASH_F4_CR_STRT);
		write_uint32(sl->q_buf, x);
		stlink_write_mem32(sl, FLASH_F4_CR, sizeof (uint32_t));
	}
	else {
		/* assume come on the flash_cr_per path */
	    const uint32_t n = (1 << FLASH_CR_PER) | (1 << FLASH_CR_STRT);
	    write_uint32(sl->q_buf, n);
	    stlink_write_mem32(sl, FLASH_CR, sizeof (uint32_t));
	}
}

static inline uint32_t read_flash_acr(stlink_t *sl) {
    stlink_read_mem32(sl, FLASH_ACR, sizeof (uint32_t));
    return read_uint32(sl->q_buf, 0);
}

static inline uint32_t read_flash_sr(stlink_t *sl) {
	if(sl->chip_id==STM32F4_CHIP_ID)
		stlink_read_mem32(sl, FLASH_F4_SR, sizeof (uint32_t));
	else
		stlink_read_mem32(sl, FLASH_SR, sizeof (uint32_t));
    //fprintf(stdout, "SR:0x%x\n", *(uint32_t*) sl->q_buf);
    return read_uint32(sl->q_buf, 0);
}

static inline unsigned int is_flash_busy(stlink_t *sl) {
	if(sl->chip_id==STM32F4_CHIP_ID)
		return read_flash_sr(sl) & (1 << FLASH_F4_SR_BSY);
	else
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

static inline void write_flash_cr_psiz(stlink_t *sl, uint32_t n) {
    uint32_t x = read_flash_cr(sl);
    x &= ~(0x03 << 8);
    x |= (n << 8);
#if DEBUG_FLASH
    fprintf(stdout, "PSIZ:0x%x 0x%x\n", x, n);
#endif
    write_uint32(sl->q_buf, x);
    stlink_write_mem32(sl, FLASH_F4_CR, sizeof (uint32_t));
}


static inline void write_flash_cr_snb(stlink_t *sl, uint32_t n) {
    uint32_t x = read_flash_cr(sl);
    x &= ~FLASH_F4_CR_SNB_MASK;
    x |= (n << FLASH_F4_CR_SNB);
    x |= (1 << FLASH_F4_CR_SER);
#if DEBUG_FLASH
    fprintf(stdout, "SNB:0x%x 0x%x\n", x, n);
#endif
    write_uint32(sl->q_buf, x);
    stlink_write_mem32(sl, FLASH_F4_CR, sizeof (uint32_t));
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
    DLOG("*** stlink_close ***\n");
    sl->backend->close(sl);
    free(sl);
}

void stlink_exit_debug_mode(stlink_t *sl) {
    DLOG("*** stlink_exit_debug_mode ***\n");
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
    stlink_read_mem32(sl, 0xE0042000, 4);
    uint32_t chip_id = sl->q_buf[0] | (sl->q_buf[1] << 8) | (sl->q_buf[2] << 16) |
            (sl->q_buf[3] << 24);
    return chip_id;
}

/**
 * Cortex m3 tech ref manual, CPUID register description
 * @param sl stlink context
 * @param cpuid pointer to the result object
 */
void stlink_cpu_id(stlink_t *sl, cortex_m3_cpuid_t *cpuid) {
    stlink_read_mem32(sl, CM3_REG_CPUID, 4);
    uint32_t raw = read_uint32(sl->q_buf, 0);
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
    
    /* Fix chip_id for F4 rev A errata */
    if (((chip_id & 0xFFF) == 0x411) && (sl->core_id == CORE_M4_R0)) {
      chip_id = 0x413;
    }

    sl->chip_id = chip_id;
	for(size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
		if(devices[i].chip_id == (chip_id & 0xFFF)) {
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
    
    // read flash size from hardware, if possible...
    if ((chip_id & 0xFFF) == STM32_CHIPID_F2) {
        sl->flash_size = 0; // FIXME - need to work this out some other way, just set to max possible?
    } else if ((chip_id & 0xFFF) == STM32_CHIPID_F4) {
		sl->flash_size = 0x100000;			//todo: RM0090 error; size register same address as unique ID
    } else {
        stlink_read_mem32(sl, params->flash_size_reg, 4);
        uint32_t flash_size = sl->q_buf[0] | (sl->q_buf[1] << 8);
        sl->flash_size = flash_size * 1024;
    }
    sl->flash_pgsz = params->flash_pagesize;
    sl->sram_size = params->sram_size;
    sl->sys_base = params->bootrom_base;
    sl->sys_size = params->bootrom_size;
    
    ILOG("Device connected is: %s\n", params->description);
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

void stlink_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    DLOG("*** stlink_write_mem32 %u bytes to %#x\n", len, addr);
    if (len % 4 != 0) {
        fprintf(stderr, "Error: Data length doesn't have a 32 bit alignment: +%d byte.\n", len % 4);
        return;
    }
    sl->backend->write_mem32(sl, addr, len);
}

void stlink_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    DLOG("*** stlink_read_mem32 ***\n");
    if (len % 4 != 0) { // !!! never ever: fw gives just wrong values
        fprintf(stderr, "Error: Data length doesn't have a 32 bit alignment: +%d byte.\n",
                len % 4);
        return;
    }
    sl->backend->read_mem32(sl, addr, len);
}

void stlink_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len) {
    DLOG("*** stlink_write_mem8 ***\n");
    sl->backend->write_mem8(sl, addr, len);
}

void stlink_read_all_regs(stlink_t *sl, reg *regp) {
    DLOG("*** stlink_read_all_regs ***\n");
    sl->backend->read_all_regs(sl, regp);
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
	size_t rounded_size;
        if ((off + read_size) > size)
	  read_size = size - off;

        /* round size if needed */
	rounded_size = read_size;
        if (rounded_size & 3)
	  rounded_size = (rounded_size + 4) & ~(3);

        stlink_read_mem32(sl, addr + off, rounded_size);

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
	if(sl->chip_id == STM32F4_CHIP_ID) {
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
  ILOG("Erasing flash page at addr: %#x\n", flashaddr);
  if (sl->chip_id == STM32F4_CHIP_ID)
  {
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
  }
  else if (sl->core_id == STM32L_CORE_ID)
  {

    uint32_t val;

    /* disable pecr protection */
    write_uint32(sl->q_buf, 0x89abcdef);
    stlink_write_mem32(sl, STM32L_FLASH_PEKEYR, sizeof(uint32_t));
    write_uint32(sl->q_buf, 0x02030405);
    stlink_write_mem32(sl, STM32L_FLASH_PEKEYR, sizeof(uint32_t));

    /* check pecr.pelock is cleared */
    stlink_read_mem32(sl, STM32L_FLASH_PECR, sizeof(uint32_t));
    val = read_uint32(sl->q_buf, 0);
    if (val & (1 << 0))
    {
      WLOG("pecr.pelock not clear (%#x)\n", val);
      return -1;
    }

    /* unlock program memory */
    write_uint32(sl->q_buf, 0x8c9daebf);
    stlink_write_mem32(sl, STM32L_FLASH_PRGKEYR, sizeof(uint32_t));
    write_uint32(sl->q_buf, 0x13141516);
    stlink_write_mem32(sl, STM32L_FLASH_PRGKEYR, sizeof(uint32_t));

    /* check pecr.prglock is cleared */
    stlink_read_mem32(sl, STM32L_FLASH_PECR, sizeof(uint32_t));
    val = read_uint32(sl->q_buf, 0);
    if (val & (1 << 1))
    {
      WLOG("pecr.prglock not clear (%#x)\n", val);
      return -1;
    }

    /* unused: unlock the option byte block */
#if 0
    write_uint32(sl->q_buf, 0xfbead9c8);
    stlink_write_mem32(sl, STM32L_FLASH_OPTKEYR, sizeof(uint32_t));
    write_uint32(sl->q_buf, 0x24252627);
    stlink_write_mem32(sl, STM32L_FLASH_OPTKEYR, sizeof(uint32_t));

    /* check pecr.optlock is cleared */
    stlink_read_mem32(sl, STM32L_FLASH_PECR, sizeof(uint32_t));
    val = read_uint32(sl->q_buf, 0);
    if (val & (1 << 2))
    {
      fprintf(stderr, "pecr.prglock not clear\n");
      return -1;
    }
#endif

    /* set pecr.{erase,prog} */
    val |= (1 << 9) | (1 << 3);
    write_uint32(sl->q_buf, val);
    stlink_write_mem32(sl, STM32L_FLASH_PECR, sizeof(uint32_t));

#if 0 /* fix_to_be_confirmed */

    /* wait for sr.busy to be cleared
       MP: Test shows that busy bit is not set here. Perhaps, PM0062 is
       wrong and we do not need to wait here for clearing the busy bit.
       TEXANE: ok, if experience says so and it works for you, we comment
       it. If someone has a problem, please drop an email.
     */
    while (1)
    {
      stlink_read_mem32(sl, STM32L_FLASH_SR, sizeof(uint32_t));
      if ((read_uint32(sl->q_buf, 0) & (1 << 0)) == 0) break ;
    }

#endif /* fix_to_be_confirmed */

    /* write 0 to the first word of the page to be erased */
    memset(sl->q_buf, 0, sizeof(uint32_t));
    stlink_write_mem32(sl, flashaddr, sizeof(uint32_t));

    /* MP: It is better to wait for clearing the busy bit after issuing
    page erase command, even though PM0062 recommends to wait before it.
    Test shows that a few iterations is performed in the following loop
    before busy bit is cleared.*/
    while (1)
    {
      stlink_read_mem32(sl, STM32L_FLASH_SR, sizeof(uint32_t));
      if ((read_uint32(sl->q_buf, 0) & (1 << 0)) == 0) break;
    }

    /* reset lock bits */
    stlink_read_mem32(sl, STM32L_FLASH_PECR, sizeof(uint32_t));
    val = read_uint32(sl->q_buf, 0) | (1 << 0) | (1 << 1) | (1 << 2);
    write_uint32(sl->q_buf, val);
    stlink_write_mem32(sl, STM32L_FLASH_PECR, sizeof(uint32_t));
  }
  else if (sl->core_id == STM32VL_CORE_ID)
  {
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
  }

  else {
    WLOG("unknown coreid: %x\n", sl->core_id);
    return -1;
  }

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

    const uint8_t* loader_code;
    size_t loader_size;

    if (sl->core_id == STM32L_CORE_ID) /* stm32l */
    {
      loader_code = loader_code_stm32l;
      loader_size = sizeof(loader_code_stm32l);
    }
    else if (sl->core_id == STM32VL_CORE_ID)
    {
      loader_code = loader_code_stm32vl;
      loader_size = sizeof(loader_code_stm32vl);
    }
    else
    {
      WLOG("unknown coreid, not sure what flash loader to use, aborting!: %x\n", sl->core_id);
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
    if ((sl->chip_id & 0xFFF) == STM32_CHIPID_F4) {
        DLOG("(FIXME)Skipping verification for F4, not enough ram (yet)\n");
        return 0;
    }
    ILOG("Starting verification of write complete\n");
    for (off = 0; off < length; off += sl->flash_pgsz) {
        size_t aligned_size;

        /* adjust last page size */
        size_t cmp_size = sl->flash_pgsz;
        if ((off + sl->flash_pgsz) > length)
            cmp_size = length - off;

        aligned_size = cmp_size;
        if (aligned_size & (4 - 1))
            aligned_size = (cmp_size + 4) & ~(4 - 1);

        stlink_read_mem32(sl, address + off, aligned_size);

        if (memcmp(sl->q_buf, data + off, cmp_size)) {
            WLOG("Verification of flash failed at offset: %zd\n", off);
            return -1;
        }
    }
    ILOG("Flash written and verified! jolly good!\n");
    return 0;

}

int stlink_write_flash(stlink_t *sl, stm32_addr_t addr, uint8_t* base, unsigned len) {
    size_t off;
    flash_loader_t fl;
    ILOG("Attempting to write %d (%#x) bytes to stm32 address: %u (%#x)\n",
        len, len, addr, addr);
    /* check addr range is inside the flash */
    stlink_calculate_pagesize(sl, addr);
    if (addr < sl->flash_base) {
        WLOG("addr too low %#x < %#x\n", addr, sl->flash_base);
        return -1;
    } else if ((addr + len) < addr) {
        WLOG("addr overruns\n");
        return -1;
    } else if ((addr + len) > (sl->flash_base + sl->flash_size)) {
        WLOG("addr too high\n");
        return -1;
    } else if ((addr & 1) || (len & 1)) {
        WLOG("unaligned addr or size\n");
        return -1;
    } else if (addr & (sl->flash_pgsz - 1)) {
        WLOG("addr not a multiple of pagesize, not supported\n");
        return -1;
    }

    // Make sure we've loaded the context with the chip details
    stlink_core_id(sl);
    /* erase each page */
    int page_count = 0;
    for (off = 0; off < len; off += stlink_calculate_pagesize(sl, addr + off)) {
        /* addr must be an addr inside the page */
        if (stlink_erase_flash_page(sl, addr + off) == -1) {
            WLOG("Failed to erase_flash_page(%#zx) == -1\n", addr + off);
            return -1;
        }
        page_count++;
    }
    ILOG("Finished erasing %d pages of %d (%#x) bytes\n", 
        page_count, sl->flash_pgsz, sl->flash_pgsz);

    if (sl->chip_id == STM32F4_CHIP_ID) {
    	/* todo: check write operation */

    	/* First unlock the cr */
    	unlock_flash_if(sl);

    	/* set parallelisim to 32 bit*/
    	write_flash_cr_psiz(sl, 2);

    	/* set programming mode */
    	set_flash_cr_pg(sl);

#define PROGRESS_CHUNK_SIZE 0x1000
    	/* write a word in program memory */
    	for (off = 0; off < len; off += sizeof(uint32_t)) {
    		if (sl->verbose >= 1) {
    			if ((off & (PROGRESS_CHUNK_SIZE - 1)) == 0) {
    				/* show progress. writing procedure is slow
					   and previous errors are misleading */
    				const uint32_t pgnum = (off / PROGRESS_CHUNK_SIZE)+1;
    				const uint32_t pgcount = len / PROGRESS_CHUNK_SIZE;
    				fprintf(stdout, "Writing %ukB chunk %u out of %u\n", PROGRESS_CHUNK_SIZE/1024, pgnum, pgcount);
    			}
    		}

    		memcpy(sl->q_buf, (const void*)(base + off), sizeof(uint32_t));
    		stlink_write_mem32(sl, addr + off, sizeof(uint32_t));

    		/* wait for sr.busy to be cleared */
    	    wait_flash_busy(sl);

    	}
    	/* Relock flash */
    	lock_flash(sl);

#if 0 /* todo: debug mode */
	fprintf(stdout, "Final CR:0x%x\n", read_flash_cr(sl));
#endif



    }	//STM32F4END

    else if (sl->core_id == STM32L_CORE_ID)    {
    	/* use fast word write. todo: half page. */
    	uint32_t val;

#if 0 /* todo: check write operation */

    	uint32_t nwrites = sl->flash_pgsz;

    	redo_write:

#endif /* todo: check write operation */

    	/* disable pecr protection */
    	write_uint32(sl->q_buf, 0x89abcdef);
    	stlink_write_mem32(sl, STM32L_FLASH_PEKEYR, sizeof(uint32_t));
    	write_uint32(sl->q_buf, 0x02030405);
    	stlink_write_mem32(sl, STM32L_FLASH_PEKEYR, sizeof(uint32_t));

    	/* check pecr.pelock is cleared */
    	stlink_read_mem32(sl, STM32L_FLASH_PECR, sizeof(uint32_t));
    	val = read_uint32(sl->q_buf, 0);
    	if (val & (1 << 0)) {
    		fprintf(stderr, "pecr.pelock not clear\n");
    		return -1;
    	}

    	/* unlock program memory */
    	write_uint32(sl->q_buf, 0x8c9daebf);
    	stlink_write_mem32(sl, STM32L_FLASH_PRGKEYR, sizeof(uint32_t));
    	write_uint32(sl->q_buf, 0x13141516);
    	stlink_write_mem32(sl, STM32L_FLASH_PRGKEYR, sizeof(uint32_t));

    	/* check pecr.prglock is cleared */
    	stlink_read_mem32(sl, STM32L_FLASH_PECR, sizeof(uint32_t));
    	val = read_uint32(sl->q_buf, 0);
    	if (val & (1 << 1)) {
    		fprintf(stderr, "pecr.prglock not clear\n");
    		return -1;
    	}

    	/* write a word in program memory */
    	for (off = 0; off < len; off += sizeof(uint32_t)) {
    		if (sl->verbose >= 1) {
    			if ((off & (sl->flash_pgsz - 1)) == 0) {
    				/* show progress. writing procedure is slow
				   and previous errors are misleading */
    				const uint32_t pgnum = off / sl->flash_pgsz;
    				const uint32_t pgcount = len / sl->flash_pgsz;
    				fprintf(stdout, "%u pages written out of %u\n", pgnum, pgcount);
    			}
    		}

    		memcpy(sl->q_buf, (const void*)(base + off), sizeof(uint32_t));
    		stlink_write_mem32(sl, addr + off, sizeof(uint32_t));

    		/* wait for sr.busy to be cleared */
    		while (1) {
    			stlink_read_mem32(sl, STM32L_FLASH_SR, sizeof(uint32_t));
    			if ((read_uint32(sl->q_buf, 0) & (1 << 0)) == 0) break ;
    		}

#if 0 /* todo: check redo write operation */

    		/* check written bytes. todo: should be on a per page basis. */
    		stlink_read_mem32(sl, addr + off, sizeof(uint32_t));
    		if (memcmp(sl->q_buf, base + off, sizeof(uint32_t))) {
    			/* re erase the page and redo the write operation */
    			uint32_t page;
    			uint32_t val;

    			/* fail if successive write count too low */
    			if (nwrites < sl->flash_pgsz) {
    				fprintf(stderr, "writes operation failure count too high, aborting\n");
    				return -1;
    			}

    			nwrites = 0;

    			/* assume addr aligned */
    			if (off % sl->flash_pgsz) off &= ~(sl->flash_pgsz - 1);
    			page = addr + off;

    			fprintf(stderr, "invalid write @0x%x(0x%x): 0x%x != 0x%x. retrying.\n",
    					page, addr + off, read_uint32(base + off, 0), read_uint32(sl->q_buf, 0));

    			/* reset lock bits */
    			stlink_read_mem32(sl, STM32L_FLASH_PECR, sizeof(uint32_t));
    			val = read_uint32(sl->q_buf, 0) | (1 << 0) | (1 << 1) | (1 << 2);
    			write_uint32(sl->q_buf, val);
    			stlink_write_mem32(sl, STM32L_FLASH_PECR, sizeof(uint32_t));

    			stlink_erase_flash_page(sl, page);

    			goto redo_write;
    		}

    		/* increment successive writes counter */
    		++nwrites;

#endif /* todo: check redo write operation */
    	}
    	/* reset lock bits */
    	stlink_read_mem32(sl, STM32L_FLASH_PECR, sizeof(uint32_t));
    	val = read_uint32(sl->q_buf, 0) | (1 << 0) | (1 << 1) | (1 << 2);
    	write_uint32(sl->q_buf, val);
    	stlink_write_mem32(sl, STM32L_FLASH_PECR, sizeof(uint32_t));
    } else if (sl->core_id == STM32VL_CORE_ID) {
        ILOG("Starting Flash write for VL core id\n");
        /* flash loader initialization */
        if (init_flash_loader(sl, &fl) == -1) {
            WLOG("init_flash_loader() == -1\n");
            return -1;
        }

        /* write each page. above WRITE_BLOCK_SIZE fails? */
#define WRITE_BLOCK_SIZE 0x40
        int write_block_count = 0;
        for (off = 0; off < len; off += WRITE_BLOCK_SIZE) {
            ILOG("Writing flash block %d of size %d (%#x)\n", write_block_count,
                WRITE_BLOCK_SIZE, WRITE_BLOCK_SIZE);
            /* adjust last write size */
            size_t size = WRITE_BLOCK_SIZE;
            if ((off + WRITE_BLOCK_SIZE) > len) size = len - off;

            /* unlock and set programming mode */
            unlock_flash_if(sl);
            set_flash_cr_pg(sl);
            //DLOG("Finished setting flash cr pg, running loader!\n");
            if (run_flash_loader(sl, &fl, addr + off, base + off, size) == -1) {
                WLOG("run_flash_loader(%#zx) failed! == -1\n", addr + off);
                return -1;
            }
            lock_flash(sl);
            DLOG("Finished writing block %d\n", write_block_count++);
        }
    } else {
        WLOG("unknown coreid, not sure how to write: %x\n", sl->core_id);
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
    mapped_file_t mf = MAPPED_FILE_INITIALIZER;
    if (map_file(&mf, path) == -1) {
        WLOG("map_file() == -1\n");
        return -1;
    }
    err = stlink_write_flash(sl, addr, mf.base, mf.len);
    unmap_file(&mf);
    return err;
}

int run_flash_loader(stlink_t *sl, flash_loader_t* fl, stm32_addr_t target, const uint8_t* buf, size_t size) {

    reg rr;
    DLOG("Running flash loader, write address:%#x, size: %zd\n", target, size);
    // FIXME This can never return -1
    if (write_buffer_to_sram(sl, fl, buf, size) == -1) {
        // IMPOSSIBLE!
        WLOG("write_buffer_to_sram() == -1\n");
        return -1;
    }

    if (sl->core_id == STM32L_CORE_ID) {

      size_t count = size / sizeof(uint32_t);
      if (size % sizeof(uint32_t)) ++count;

      /* setup core */
      stlink_write_reg(sl, target, 0); /* target */
      stlink_write_reg(sl, fl->buf_addr, 1); /* source */
      stlink_write_reg(sl, count, 2); /* count (32 bits words) */
      stlink_write_reg(sl, 0, 3); /* output count */
      stlink_write_reg(sl, fl->loader_addr, 15); /* pc register */

    } else if (sl->core_id == STM32VL_CORE_ID) {

      size_t count = size / sizeof(uint16_t);
      if (size % sizeof(uint16_t)) ++count;

      /* setup core */
      stlink_write_reg(sl, fl->buf_addr, 0); /* source */
      stlink_write_reg(sl, target, 1); /* target */
      stlink_write_reg(sl, count, 2); /* count (16 bits half words) */
      stlink_write_reg(sl, 0, 3); /* flash bank 0 (input) */
      stlink_write_reg(sl, fl->loader_addr, 15); /* pc register */

    } else {
      fprintf(stderr, "unknown coreid: 0x%x\n", sl->core_id);
      return -1;
    }

    /* run loader */
    stlink_run(sl);

    /* wait until done (reaches breakpoint) */
    while (is_core_halted(sl) == 0) ;

    /* check written byte count */
    if (sl->core_id == STM32L_CORE_ID) {

      size_t count = size / sizeof(uint32_t);
      if (size % sizeof(uint32_t)) ++count;

      stlink_read_reg(sl, 3, &rr);
      if (rr.r[3] != count) {
        fprintf(stderr, "write error, count == %u\n", rr.r[3]);
        return -1;
      }

    } else if (sl->core_id == STM32VL_CORE_ID) {

      stlink_read_reg(sl, 2, &rr);
      if (rr.r[2] != 0) {
        fprintf(stderr, "write error, count == %u\n", rr.r[2]);
        return -1;
      }

    } else {

      fprintf(stderr, "unknown coreid: 0x%x\n", sl->core_id);
      return -1;

    }

    return 0;
}
