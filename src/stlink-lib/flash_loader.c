#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <stlink.h>
#include "flash_loader.h"

#define FLASH_REGS_BANK2_OFS 0x40
#define FLASH_BANK2_START_ADDR 0x08080000

/* DO NOT MODIFY SOURCECODE DIRECTLY, EDIT ASSEMBLY FILES INSTEAD */

/* flashloaders/stm32f0.s -- compiled with thumb2 */
static const uint8_t loader_code_stm32vl[] = {
    0x16, 0x4f, 0x3c, 0x68,
    0x16, 0x4f, 0x3e, 0x68,
    0x36, 0x19, 0x16, 0x4f,
    0x3d, 0x68, 0x2d, 0x19,
    0x4f, 0xf0, 0x01, 0x07,
    0x33, 0x68, 0x3b, 0x43,
    0x33, 0x60, 0x03, 0x88,
    0x0b, 0x80, 0x4f, 0xf0,
    0x02, 0x07, 0xc0, 0x19,
    0xc9, 0x19, 0x4f, 0xf0,
    0x01, 0x07, 0x2b, 0x68,
    0x3b, 0x42, 0xfa, 0xd0,
    0x4f, 0xf0, 0x04, 0x07,
    0x3b, 0x42, 0x04, 0xd1,
    0x4f, 0xf0, 0x01, 0x07,
    0xd2, 0x1b, 0x00, 0x2a,
    0xe6, 0xd1, 0x4f, 0xf0,
    0x01, 0x07, 0x33, 0x68,
    0xbb, 0x43, 0x33, 0x60,
    0x00, 0xbe, 0x00, 0xbf,
    0x00, 0x20, 0x02, 0x40,
    0x10, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x00, 0x00,
    0x50, 0x00, 0x00, 0x20,
    0x54, 0x00, 0x00, 0x20,
    0x58, 0x00, 0x00, 0x20
};

/* flashloaders/stm32f0.s -- thumb1 only, same sequence as for STM32VL, bank ignored */
static const uint8_t loader_code_stm32f0[] = {
    0xc0, 0x46, 0xc0, 0x46,
    0x13, 0x4f, 0x3c, 0x68,
    0x13, 0x4f, 0x3e, 0x68,
    0x36, 0x19, 0x13, 0x4f,
    0x3d, 0x68, 0x2d, 0x19,
    0x12, 0x4f, 0x33, 0x68,
    0x3b, 0x43, 0x33, 0x60,
    0x03, 0x88, 0x0b, 0x80,
    0x10, 0x4f, 0xc0, 0x19,
    0xc9, 0x19, 0x0e, 0x4f,
    0x2b, 0x68, 0x3b, 0x42,
    0xfb, 0xd0, 0x0e, 0x4f,
    0x3b, 0x42, 0x03, 0xd1,
    0x0a, 0x4f, 0xd2, 0x1b,
    0x00, 0x2a, 0xeb, 0xd1,
    0x08, 0x4f, 0x33, 0x68,
    0xbb, 0x43, 0x33, 0x60,
    0x00, 0xbe, 0xc0, 0x46,
    0x00, 0x20, 0x02, 0x40,
    0x10, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x00, 0x00,
    0x48, 0x00, 0x00, 0x20,
    0x4c, 0x00, 0x00, 0x20,
    0x50, 0x00, 0x00, 0x20,
    0x01, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00
};

static const uint8_t loader_code_stm32l[] = {
    // flashloaders/stm32lx.s

    0x03, 0x68, 0x0b, 0x60,
    0x4f, 0xf0, 0x04, 0x07,
    0x38, 0x44, 0x39, 0x44,
    0x4f, 0xf0, 0x01, 0x07,
    0xd2, 0x1b, 0x00, 0x2a,
    0xf4, 0xd1, 0x00, 0xbe,
};

static const uint8_t loader_code_stm32f4[] = {
    // flashloaders/stm32f4.s

    0xdf, 0xf8, 0x28, 0xc0,
    0xdf, 0xf8, 0x28, 0xa0,
    0xe2, 0x44, 0x03, 0x68,
    0x0b, 0x60, 0x00, 0xf1,
    0x04, 0x00, 0x01, 0xf1,
    0x04, 0x01, 0xba, 0xf8,
    0x00, 0x30, 0x13, 0xf0,
    0x01, 0x0f, 0xfa, 0xd0,
    0xa2, 0xf1, 0x01, 0x02,
    0x00, 0x2a, 0xf0, 0xd1,
    0x00, 0xbe, 0x00, 0xbf,
    0x00, 0x3c, 0x02, 0x40,
    0x0e, 0x00, 0x00, 0x00
};

static const uint8_t loader_code_stm32f4_lv[] = {
    // flashloaders/stm32f4lv.s
    0xdf, 0xf8, 0x2c, 0xc0,
    0xdf, 0xf8, 0x2c, 0xa0,
    0xe2, 0x44, 0x4f, 0xea,
    0x82, 0x02, 0x03, 0x78,
    0x0b, 0x70, 0x00, 0xf1,
    0x01, 0x00, 0x01, 0xf1,
    0x01, 0x01, 0xba, 0xf8,
    0x00, 0x30, 0x13, 0xf0,
    0x01, 0x0f, 0xfa, 0xd0,
    0xa2, 0xf1, 0x01, 0x02,
    0x00, 0x2a, 0xf0, 0xd1,
    0x00, 0xbe, 0x00, 0xbf,
    0x00, 0x3c, 0x02, 0x40,
    0x0e, 0x00, 0x00, 0x00
};

static const uint8_t loader_code_stm32l4[] = {
    // flashloaders/stm32l4.s
    0xdf, 0xf8, 0x2c, 0xc0,
    0xdf, 0xf8, 0x2c, 0xa0,
    0xe2, 0x44, 0x03, 0x68,
    0x44, 0x68, 0x0b, 0x60,
    0x4c, 0x60, 0x00, 0xf1,
    0x08, 0x00, 0x01, 0xf1,
    0x08, 0x01, 0xba, 0xf8,
    0x00, 0x30, 0x13, 0xf0,
    0x01, 0x0f, 0xfa, 0xd0,
    0xa2, 0xf1, 0x01, 0x02,
    0x00, 0x2a, 0xee, 0xd1,
    0x00, 0xbe, 0x00, 0xbf,
    0x00, 0x20, 0x02, 0x40,
    0x12, 0x00, 0x00, 0x00
};

static const uint8_t loader_code_stm32f7[] = {
    // flashloaders/stm32f7.s
    0xdf, 0xf8, 0x2c, 0xc0,
    0xdf, 0xf8, 0x2c, 0xa0,
    0xe2, 0x44, 0x03, 0x68,
    0x0b, 0x60, 0x00, 0xf1,
    0x04, 0x00, 0x01, 0xf1,
    0x04, 0x01, 0xbf, 0xf3,
    0x4f, 0x8f, 0xba, 0xf8,
    0x00, 0x30, 0x13, 0xf0,
    0x01, 0x0f, 0xfa, 0xd0,
    0xa2, 0xf1, 0x01, 0x02,
    0x00, 0x2a, 0xee, 0xd1,
    0x00, 0xbe, 0x00, 0xbf,
    0x00, 0x3c, 0x02, 0x40,
    0x0e, 0x00, 0x00, 0x00
};

static const uint8_t loader_code_stm32f7_lv[] = {
    // flashloaders/stm32f7lv.s
    0xdf, 0xf8, 0x30, 0xc0,
    0xdf, 0xf8, 0x30, 0xa0,
    0xe2, 0x44, 0x4f, 0xea,
    0x82, 0x02, 0x03, 0x78,
    0x0b, 0x70, 0x00, 0xf1,
    0x01, 0x00, 0x01, 0xf1,
    0x01, 0x01, 0xbf, 0xf3,
    0x4f, 0x8f, 0xba, 0xf8,
    0x00, 0x30, 0x13, 0xf0,
    0x01, 0x0f, 0xfa, 0xd0,
    0xa2, 0xf1, 0x01, 0x02,
    0x00, 0x2a, 0xee, 0xd1,
    0x00, 0xbe, 0x00, 0xbf,
    0x00, 0x3c, 0x02, 0x40,
    0x0e, 0x00, 0x00, 0x00
};


int stlink_flash_loader_init(stlink_t *sl, flash_loader_t *fl) {
    size_t size = 0;

    // allocate the loader in SRAM
    if (stlink_flash_loader_write_to_sram(sl, &fl->loader_addr, &size) == -1) {
        WLOG("Failed to write flash loader to sram!\n");
        return(-1);
    }

    // allocate a one page buffer in SRAM right after loader
    fl->buf_addr = fl->loader_addr + (uint32_t)size;
    ILOG("Successfully loaded flash loader in sram\n");

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

    if (sl->chip_id == STLINK_CHIPID_STM32_L1_MEDIUM ||
        sl->chip_id == STLINK_CHIPID_STM32_L1_CAT2 ||
        sl->chip_id == STLINK_CHIPID_STM32_L1_MEDIUM_PLUS ||
        sl->chip_id == STLINK_CHIPID_STM32_L1_HIGH ||
        sl->chip_id == STLINK_CHIPID_STM32_L152_RE ||
        sl->chip_id == STLINK_CHIPID_STM32_L011 ||
        sl->chip_id == STLINK_CHIPID_STM32_L0 ||
        sl->chip_id == STLINK_CHIPID_STM32_L0_CAT5 ||
        sl->chip_id == STLINK_CHIPID_STM32_L0_CAT2) { // STM32l
        loader_code = loader_code_stm32l;
        loader_size = sizeof(loader_code_stm32l);
    } else if (sl->core_id == STM32VL_CORE_ID ||
               sl->chip_id == STLINK_CHIPID_STM32_F1_MEDIUM ||
               sl->chip_id == STLINK_CHIPID_STM32_F3 ||
               sl->chip_id == STLINK_CHIPID_STM32_F3_SMALL ||
               sl->chip_id == STLINK_CHIPID_STM32_F303_HIGH ||
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
               sl->chip_id == STLINK_CHIPID_STM32_F411RE ||
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
               sl->chip_id == STLINK_CHIPID_STM32_F7XXXX ||
               sl->chip_id == STLINK_CHIPID_STM32_F72XXX)                                          {
        int retval;
        retval = loader_v_dependent_assignment(sl,
                                               &loader_code, &loader_size,
                                               loader_code_stm32f7, sizeof(loader_code_stm32f7),
                                               loader_code_stm32f7_lv, sizeof(loader_code_stm32f7_lv));

        if (retval == -1) { return(retval); }
    } else if (sl->chip_id == STLINK_CHIPID_STM32_F0 ||
               sl->chip_id == STLINK_CHIPID_STM32_F04 ||
               sl->chip_id == STLINK_CHIPID_STM32_F0_CAN ||
               sl->chip_id == STLINK_CHIPID_STM32_F0_SMALL ||
               sl->chip_id == STLINK_CHIPID_STM32_F09X) {
        loader_code = loader_code_stm32f0;
        loader_size = sizeof(loader_code_stm32f0);
    } else if ((sl->chip_id == STLINK_CHIPID_STM32_L4) ||
               (sl->chip_id == STLINK_CHIPID_STM32_L41X) ||
               (sl->chip_id == STLINK_CHIPID_STM32_L43X) ||
               (sl->chip_id == STLINK_CHIPID_STM32_L46X) ||
               (sl->chip_id == STLINK_CHIPID_STM32_L4RX) ||
               (sl->chip_id == STLINK_CHIPID_STM32_L496X)) {
        loader_code = loader_code_stm32l4;
        loader_size = sizeof(loader_code_stm32l4);
    } else {
        ELOG("unknown coreid, not sure what flash loader to use, aborting! coreid: %x, chipid: %x\n",
            sl->core_id, sl->chip_id);
        return(-1);
    }

    memcpy(sl->q_buf, loader_code, loader_size);
    int ret = stlink_write_mem32(sl, sl->sram_base, loader_size);

    if (ret) { return(ret); }

    *addr = sl->sram_base;
    *size = loader_size;

    return(0); // success
}

int stlink_flash_loader_run(stlink_t *sl, flash_loader_t* fl, stm32_addr_t target, const uint8_t* buf, size_t size) {
    struct stlink_reg rr;
    int i = 0;
    size_t count = 0;
    uint32_t flash_base = 0;

    DLOG("Running flash loader, write address:%#x, size: %u\n", target, (unsigned int)size);

    // TODO: This can never return -1
    if (write_buffer_to_sram(sl, fl, buf, size) == -1) {
        // IMPOSSIBLE!
        ELOG("write_buffer_to_sram() == -1\n");
        return(-1);
    }

    if ((sl->flash_type == STLINK_FLASH_TYPE_F0) ||
        (sl->flash_type == STLINK_FLASH_TYPE_F1_XL)) {
        count = size / sizeof(uint16_t);

        if (size % sizeof(uint16_t)) { ++count; }
    } else if (sl->flash_type == STLINK_FLASH_TYPE_F4 ||
               sl->flash_type == STLINK_FLASH_TYPE_L0) {
        count = size / sizeof(uint32_t);

        if (size % sizeof(uint32_t)) { ++count; }
    } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
        count = size / sizeof(uint64_t);

        if (size % sizeof(uint64_t)) { ++count; }
    }

    if ((sl->flash_type == STLINK_FLASH_TYPE_F1_XL) && (target >= FLASH_BANK2_START_ADDR)) {
        flash_base = FLASH_REGS_BANK2_OFS;
    }

    /* Setup core */
    stlink_write_reg(sl, fl->buf_addr, 0);     // source
    stlink_write_reg(sl, target, 1);           // target
    stlink_write_reg(sl, (uint32_t)count, 2);  // count
    stlink_write_reg(sl, flash_base, 3);       // flash register base
                                               // only used on VL/F1_XL, but harmless for others
    stlink_write_reg(sl, fl->loader_addr, 15); // pc register

    /* Run loader */
    stlink_run(sl);

/* This piece of code used to try to spin for .1 second by waiting doing 10000 rounds of 10 µs.
 * But because this usually runs on Unix-like OSes, the 10 µs get rounded up to the "tick"
 * (actually almost two ticks) of the system. 1 ms. Thus, the ten thousand attempts, when
 * "something goes wrong" that requires the error message "flash loader run error" would wait
 * for something like 20 seconds before coming up with the error.
 * By increasing the sleep-per-round to the same order-of-magnitude as the tick-rounding that
 * the OS uses, the wait until the error message is reduced to the same order of magnitude
 * as what was intended. -- REW.
 */
#define WAIT_ROUNDS 30

    // wait until done (reaches breakpoint)
    for (i = 0; i < WAIT_ROUNDS; i++) {
        usleep(10000);

        if (stlink_is_core_halted(sl)) { break; }
    }

    if (i >= WAIT_ROUNDS) {
        ELOG("flash loader run error\n");
        return(-1);
    }

    // check written byte count
    stlink_read_reg(sl, 2, &rr);

    if (rr.r[2] != 0) {
        ELOG("write error, count == %u\n", rr.r[2]);
        return(-1);
    }

    return(0);
}
