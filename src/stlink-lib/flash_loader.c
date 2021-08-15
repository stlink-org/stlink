#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <stlink.h>
#include <helper.h>
#include "flash_loader.h"

#define FLASH_REGS_BANK2_OFS 0x40
#define FLASH_BANK2_START_ADDR 0x08080000

#define STM32F0_WDG_KR            0x40003000
#define STM32H7_WDG_KR            0x58004800

#define STM32F0_WDG_KR_KEY_RELOAD 0xAAAA

/* !!!
 * !!! DO NOT MODIFY FLASH LOADERS DIRECTLY!
 * !!!
 *
 * Edit assembly files in the '/flashloaders' instead. The sizes of binary
 * flash loaders must be aligned by 4 (it's written by stlink_write_mem32)
 */

/* flashloaders/stm32f0.s -- compiled with thumb2 */
static const uint8_t loader_code_stm32vl[] = {
    0x00, 0xbf, 0x00, 0xbf,
    0x09, 0x4f, 0x1f, 0x44,
    0x09, 0x4d, 0x3d, 0x44,
    0x04, 0x88, 0x0c, 0x80,
    0x02, 0x30, 0x02, 0x31,
    0x4f, 0xf0, 0x01, 0x07,
    0x2c, 0x68, 0x3c, 0x42,
    0xfc, 0xd1, 0x4f, 0xf0,
    0x14, 0x07, 0x3c, 0x42,
    0x01, 0xd1, 0x02, 0x3a,
    0xf0, 0xdc, 0x00, 0xbe,
    0x00, 0x20, 0x02, 0x40,
    0x0c, 0x00, 0x00, 0x00
};

/* flashloaders/stm32f0.s -- thumb1 only, same sequence as for STM32VL, bank ignored */
static const uint8_t loader_code_stm32f0[] = {
    0xc0, 0x46, 0xc0, 0x46,
    0x08, 0x4f, 0x1f, 0x44,
    0x08, 0x4d, 0x3d, 0x44,
    0x04, 0x88, 0x0c, 0x80,
    0x02, 0x30, 0x02, 0x31,
    0x06, 0x4f, 0x2c, 0x68,
    0x3c, 0x42, 0xfc, 0xd1,
    0x05, 0x4f, 0x3c, 0x42,
    0x01, 0xd1, 0x02, 0x3a,
    0xf2, 0xdc, 0x00, 0xbe,
    0x00, 0x20, 0x02, 0x40,
    0x0c, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00
};

static const uint8_t loader_code_stm32lx[] = {
    // flashloaders/stm32lx.s
    0x04, 0x68, 0x0c, 0x60,
    0x00, 0xf1, 0x04, 0x00,
    0x01, 0xf1, 0x04, 0x01,
    0x04, 0x3a, 0xf7, 0xdc,
    0x00, 0xbe, 0x00, 0x00
};

static const uint8_t loader_code_stm32f4[] = {
    // flashloaders/stm32f4.s
    0xdf, 0xf8, 0x24, 0xc0,
    0xdf, 0xf8, 0x24, 0xa0,
    0xe2, 0x44, 0x04, 0x68,
    0x0c, 0x60, 0x00, 0xf1,
    0x04, 0x00, 0x01, 0xf1,
    0x04, 0x01, 0xba, 0xf8,
    0x00, 0x40, 0x14, 0xf0,
    0x01, 0x0f, 0xfa, 0xd1,
    0x04, 0x3a, 0xf2, 0xdc,
    0x00, 0xbe, 0x00, 0xbf,
    0x00, 0x3c, 0x02, 0x40,
    0x0e, 0x00, 0x00, 0x00
};

static const uint8_t loader_code_stm32f4_lv[] = {
    // flashloaders/stm32f4lv.s
    0xdf, 0xf8, 0x24, 0xc0,
    0xdf, 0xf8, 0x24, 0xa0,
    0xe2, 0x44, 0x04, 0x78,
    0x0c, 0x70, 0x00, 0xf1,
    0x01, 0x00, 0x01, 0xf1,
    0x01, 0x01, 0xba, 0xf8,
    0x00, 0x40, 0x14, 0xf0,
    0x01, 0x0f, 0xfa, 0xd1,
    0x01, 0x3a, 0xf2, 0xdc,
    0x00, 0xbe, 0x00, 0xbf,
    0x00, 0x3c, 0x02, 0x40,
    0x0e, 0x00, 0x00, 0x00
};

static const uint8_t loader_code_stm32l4[] = {
    // flashloaders/stm32l4.s
    0xdf, 0xf8, 0x28, 0xc0,
    0xdf, 0xf8, 0x28, 0xa0,
    0xe2, 0x44, 0x05, 0x68,
    0x44, 0x68, 0x0d, 0x60,
    0x4c, 0x60, 0x00, 0xf1,
    0x08, 0x00, 0x01, 0xf1,
    0x08, 0x01, 0xda, 0xf8,
    0x00, 0x40, 0x14, 0xf4,
    0x80, 0x3f, 0xfa, 0xd1,
    0x08, 0x3a, 0xf0, 0xdc,
    0x00, 0xbe, 0x00, 0xbf,
    0x00, 0x20, 0x02, 0x40,
    0x10, 0x00, 0x00, 0x00
};

static const uint8_t loader_code_stm32f7[] = {
    // flashloaders/stm32f7.s
    0xdf, 0xf8, 0x28, 0xc0,
    0xdf, 0xf8, 0x28, 0xa0,
    0xe2, 0x44, 0x04, 0x68,
    0x0c, 0x60, 0x00, 0xf1,
    0x04, 0x00, 0x01, 0xf1,
    0x04, 0x01, 0xbf, 0xf3,
    0x4f, 0x8f, 0xba, 0xf8,
    0x00, 0x40, 0x14, 0xf0,
    0x01, 0x0f, 0xfa, 0xd1,
    0x04, 0x3a, 0xf0, 0xdc,
    0x00, 0xbe, 0x00, 0xbf,
    0x00, 0x3c, 0x02, 0x40,
    0x0e, 0x00, 0x00, 0x00
};

static const uint8_t loader_code_stm32f7_lv[] = {
    // flashloaders/stm32f7lv.s
    0xdf, 0xf8, 0x28, 0xc0,
    0xdf, 0xf8, 0x28, 0xa0,
    0xe2, 0x44, 0x04, 0x78,
    0x0c, 0x70, 0x00, 0xf1,
    0x01, 0x00, 0x01, 0xf1,
    0x01, 0x01, 0xbf, 0xf3,
    0x4f, 0x8f, 0xba, 0xf8,
    0x00, 0x40, 0x14, 0xf0,
    0x01, 0x0f, 0xfa, 0xd1,
    0x01, 0x3a, 0xf0, 0xdc,
    0x00, 0xbe, 0x00, 0xbf,
    0x00, 0x3c, 0x02, 0x40,
    0x0e, 0x00, 0x00, 0x00
};


int stlink_flash_loader_init(stlink_t *sl, flash_loader_t *fl) {
    size_t size = 0;
    uint32_t dfsr, cfsr, hfsr;

    /* Interrupt masking.
     * According to DDI0419C, Table C1-7 firstly force halt */
    stlink_write_debug32(sl, STLINK_REG_DHCSR,
                           STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_DEBUGEN |
                           STLINK_REG_DHCSR_C_HALT);
    /* and only then disable interrupts */
    stlink_write_debug32(sl, STLINK_REG_DHCSR,
                           STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_DEBUGEN |
                           STLINK_REG_DHCSR_C_HALT |
                           STLINK_REG_DHCSR_C_MASKINTS);

    // allocate the loader in SRAM
    if (stlink_flash_loader_write_to_sram(sl, &fl->loader_addr, &size) == -1) {
        WLOG("Failed to write flash loader to sram!\n");
        return(-1);
    }

    // allocate a one page buffer in SRAM right after loader
    fl->buf_addr = fl->loader_addr + (uint32_t)size;
    ILOG("Successfully loaded flash loader in sram\n");

    // set address of IWDG key register for reset it
    if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
        fl->iwdg_kr = STM32H7_WDG_KR;
    } else {
        fl->iwdg_kr = STM32F0_WDG_KR;
    }

    /* Clear Fault Status Register for handling flash loader error */
    if (!stlink_read_debug32(sl, STLINK_REG_DFSR, &dfsr) && dfsr) {
        ILOG("Clear DFSR\n");
        stlink_write_debug32(sl, STLINK_REG_DFSR, dfsr);
    }
    if (!stlink_read_debug32(sl, STLINK_REG_CFSR, &cfsr) && cfsr) {
        ILOG("Clear CFSR\n");
        stlink_write_debug32(sl, STLINK_REG_CFSR, cfsr);
    }
    if (!stlink_read_debug32(sl, STLINK_REG_HFSR, &hfsr) && hfsr) {
        ILOG("Clear HFSR\n");
        stlink_write_debug32(sl, STLINK_REG_HFSR, hfsr);
    }

    return(0);
}

static int loader_v_dependent_assignment(stlink_t *sl,
                                         const uint8_t **loader_code, size_t *loader_size,
                                         const uint8_t *high_v_loader, size_t high_v_loader_size,
                                         const uint8_t *low_v_loader, size_t low_v_loader_size) {
    int retval = 0;

    if ( sl->version.stlink_v == 1) {
        printf("STLINK V1 cannot read voltage, defaulting to 32-bit writes\n");
        *loader_code = high_v_loader;
        *loader_size = high_v_loader_size;
    } else {
        int voltage = stlink_target_voltage(sl);

        if (voltage == -1) {
            retval = -1;
            printf("Failed to read Target voltage\n");
        } else   {
            if (voltage > 2700) {
                *loader_code = high_v_loader;
                *loader_size = high_v_loader_size;
            } else {
                *loader_code = low_v_loader;
                *loader_size = low_v_loader_size;
            }
        }
    }

    return(retval);
}

int stlink_flash_loader_write_to_sram(stlink_t *sl, stm32_addr_t* addr, size_t* size) {
    const uint8_t* loader_code;
    size_t loader_size;

    if (sl->chip_id == STLINK_CHIPID_STM32_L1_MD ||
        sl->chip_id == STLINK_CHIPID_STM32_L1_CAT2 ||
        sl->chip_id == STLINK_CHIPID_STM32_L1_MD_PLUS ||
        sl->chip_id == STLINK_CHIPID_STM32_L1_MD_PLUS_HD ||
        sl->chip_id == STLINK_CHIPID_STM32_L152_RE ||
        sl->chip_id == STLINK_CHIPID_STM32_L011 ||
        sl->chip_id == STLINK_CHIPID_STM32_L0 ||
        sl->chip_id == STLINK_CHIPID_STM32_L0_CAT5 ||
        sl->chip_id == STLINK_CHIPID_STM32_L0_CAT2) {
        loader_code = loader_code_stm32lx;
        loader_size = sizeof(loader_code_stm32lx);
    } else if (sl->core_id == STM32VL_CORE_ID ||
               sl->chip_id == STLINK_CHIPID_STM32_F1_MD ||
               sl->chip_id == STLINK_CHIPID_STM32_F1_HD ||
               sl->chip_id == STLINK_CHIPID_STM32_F1_LD ||
               sl->chip_id == STLINK_CHIPID_STM32_F1_VL_MD_LD ||
               sl->chip_id == STLINK_CHIPID_STM32_F1_VL_HD ||
               sl->chip_id == STLINK_CHIPID_STM32_F1_XLD ||
               sl->chip_id == STLINK_CHIPID_STM32_F1_CONN ||
               sl->chip_id == STLINK_CHIPID_STM32_F3 ||
               sl->chip_id == STLINK_CHIPID_STM32_F3xx_SMALL ||
               sl->chip_id == STLINK_CHIPID_STM32_F303_HD ||
               sl->chip_id == STLINK_CHIPID_STM32_F37x ||
               sl->chip_id == STLINK_CHIPID_STM32_F334) {
        loader_code = loader_code_stm32vl;
        loader_size = sizeof(loader_code_stm32vl);
    } else if (sl->chip_id == STLINK_CHIPID_STM32_F2 ||
               sl->chip_id == STLINK_CHIPID_STM32_F4 ||
               sl->chip_id == STLINK_CHIPID_STM32_F4_DE ||
               sl->chip_id == STLINK_CHIPID_STM32_F4_LP ||
               sl->chip_id == STLINK_CHIPID_STM32_F4_HD ||
               sl->chip_id == STLINK_CHIPID_STM32_F4_DSI ||
               sl->chip_id == STLINK_CHIPID_STM32_F410 ||
               sl->chip_id == STLINK_CHIPID_STM32_F411xx ||
               sl->chip_id == STLINK_CHIPID_STM32_F412 ||
               sl->chip_id == STLINK_CHIPID_STM32_F413 ||
               sl->chip_id == STLINK_CHIPID_STM32_F446) {
        int retval;
        retval = loader_v_dependent_assignment(sl,
                                               &loader_code, &loader_size,
                                               loader_code_stm32f4, sizeof(loader_code_stm32f4),
                                               loader_code_stm32f4_lv, sizeof(loader_code_stm32f4_lv));

        if (retval == -1) { return(retval); }
    } else if (sl->core_id == STM32F7_CORE_ID ||
               sl->chip_id == STLINK_CHIPID_STM32_F7 ||
               sl->chip_id == STLINK_CHIPID_STM32_F76xxx ||
               sl->chip_id == STLINK_CHIPID_STM32_F72xxx) {
        int retval;
        retval = loader_v_dependent_assignment(sl,
                                               &loader_code, &loader_size,
                                               loader_code_stm32f7, sizeof(loader_code_stm32f7),
                                               loader_code_stm32f7_lv, sizeof(loader_code_stm32f7_lv));

        if (retval == -1) { return(retval); }
    } else if (sl->chip_id == STLINK_CHIPID_STM32_F0 ||
               sl->chip_id == STLINK_CHIPID_STM32_F04 ||
               sl->chip_id == STLINK_CHIPID_STM32_F0_CAN ||
               sl->chip_id == STLINK_CHIPID_STM32_F0xx_SMALL ||
               sl->chip_id == STLINK_CHIPID_STM32_F09x) {
        loader_code = loader_code_stm32f0;
        loader_size = sizeof(loader_code_stm32f0);
    } else if ((sl->chip_id == STLINK_CHIPID_STM32_L4) ||
               (sl->chip_id == STLINK_CHIPID_STM32_L41x_L42x) ||
               (sl->chip_id == STLINK_CHIPID_STM32_L43x_L44x) ||
               (sl->chip_id == STLINK_CHIPID_STM32_L45x_L46x) ||
               (sl->chip_id == STLINK_CHIPID_STM32_L4Rx) ||
               (sl->chip_id == STLINK_CHIPID_STM32_L496x_L4A6x)) {
        loader_code = loader_code_stm32l4;
        loader_size = sizeof(loader_code_stm32l4);
    } else {
        ELOG("unknown coreid, not sure what flash loader to use, aborting! coreid: %x, chipid: %x\n",
            sl->core_id, sl->chip_id);
        return(-1);
    }

    memcpy(sl->q_buf, loader_code, loader_size);
    int ret = stlink_write_mem32(sl, sl->sram_base, (uint16_t)loader_size);

    if (ret) { return(ret); }

    *addr = sl->sram_base;
    *size = loader_size;

    return(0); // success
}

int stlink_flash_loader_run(stlink_t *sl, flash_loader_t* fl, stm32_addr_t target, const uint8_t* buf, size_t size) {
    struct stlink_reg rr;
    unsigned timeout;
    uint32_t flash_base = 0;
    uint32_t dhcsr, dfsr, cfsr, hfsr;

    DLOG("Running flash loader, write address:%#x, size: %u\n", target, (unsigned int)size);

    if (write_buffer_to_sram(sl, fl, buf, size) == -1) {
        ELOG("write_buffer_to_sram() == -1\n");
        return(-1);
    }

    if ((sl->flash_type == STLINK_FLASH_TYPE_F1_XL) && (target >= FLASH_BANK2_START_ADDR)) {
        flash_base = FLASH_REGS_BANK2_OFS;
    }

    /* Setup core */
    stlink_write_reg(sl, fl->buf_addr, 0);     // source
    stlink_write_reg(sl, target, 1);           // target
    stlink_write_reg(sl, (uint32_t)size, 2);   // count
    stlink_write_reg(sl, flash_base, 3);       // flash register base
                                               // only used on VL/F1_XL, but harmless for others
    stlink_write_reg(sl, fl->loader_addr, 15); // pc register

    /* Reset IWDG */
    if (fl->iwdg_kr) {
        stlink_write_debug32(sl, fl->iwdg_kr, STM32F0_WDG_KR_KEY_RELOAD);
    }

    /* Run loader */
    stlink_run(sl, RUN_FLASH_LOADER);

/* This piece of code used to try to spin for .1 second by waiting doing 10000 rounds of 10 µs.
 * But because this usually runs on Unix-like OSes, the 10 µs get rounded up to the "tick"
 * (actually almost two ticks) of the system. 1 ms. Thus, the ten thousand attempts, when
 * "something goes wrong" that requires the error message "flash loader run error" would wait
 * for something like 20 seconds before coming up with the error.
 * By increasing the sleep-per-round to the same order-of-magnitude as the tick-rounding that
 * the OS uses, the wait until the error message is reduced to the same order of magnitude
 * as what was intended. -- REW.
 */

    // wait until done (reaches breakpoint)
    timeout = time_ms() + 500;
    while (time_ms() < timeout) {
        usleep(10000);

        if (stlink_is_core_halted(sl)) {
            timeout = 0;
            break;
        }
    }

    if (timeout) {
        ELOG("Flash loader run error\n");
        goto error;
    }

    // check written byte count
    stlink_read_reg(sl, 2, &rr);

    /* The chunk size for loading is not rounded. The flash loader
     * subtracts the size of the written block (1-8 bytes) from
     * the remaining size each time. A negative value may mean that
     * several bytes garbage has been written due to the unaligned
     * firmware size.
     */
    if ((int32_t)rr.r[2] > 0 || (int32_t)rr.r[2] < -7) {
        ELOG("Write error\n");
        goto error;
    }

    return(0);

error:
    dhcsr = dfsr = cfsr = hfsr = 0;
    stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr);
    stlink_read_debug32(sl, STLINK_REG_DFSR, &dfsr);
    stlink_read_debug32(sl, STLINK_REG_CFSR, &cfsr);
    stlink_read_debug32(sl, STLINK_REG_HFSR, &hfsr);
    stlink_read_all_regs(sl, &rr);

    WLOG("Loader state: R2 0x%X R15 0x%X\n", rr.r[2], rr.r[15]);
    if (dhcsr != 0x3000B || dfsr || cfsr || hfsr) {
        WLOG("MCU state: DHCSR 0x%X DFSR 0x%X CFSR 0x%X HFSR 0x%X\n",
            dhcsr, dfsr, cfsr, hfsr);
    }

    return(-1);
}
