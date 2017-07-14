#include "stlink.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FLASH_REGS_BANK2_OFS 0x40
#define FLASH_BANK2_START_ADDR 0x08080000

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
        // flashloaders/stm32lx.s

        0x04, 0xe0, //     b test_done          ; Go to compare
        // write_word:
        0x04, 0x68, //     ldr      r4, [r0]    ; Load one word from address in r0
        0x0c, 0x60, //     str      r4, [r1]    ; Store the word to address in r1
        0x04, 0x30, //     adds     r0, #4      ; Increment r0
        0x04, 0x31, //     adds     r1, #4      ; Increment r1
        0x01, 0x3a, //     subs     r2, #1      ; Decrement r2
        // test_done:
        0x00, 0x2a, //     cmp      r2, #0      ; Compare r2 to 0
        0xf8, 0xd8, //     bhi      write_word  ; Loop if above 0
        0x00, 0xbe, //     bkpt     #0x00       ; Set breakpoint to exit
        0x00, 0x00
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

    static const uint8_t loader_code_stm32f4_lv[] = {
        // flashloaders/stm32f4lv.s
        0x92, 0x00,

        0x08, 0x4b,
        0x62, 0xb1,
        0x04, 0x78,
        0x0c, 0x70,

        0xdc, 0x89,
        0x14, 0xf0, 0x01, 0x0f,
        0xfb, 0xd1,
        0x00, 0xf1, 0x01, 0x00,
        0x01, 0xf1, 0x01, 0x01,
        0xa2, 0xf1, 0x01, 0x02,
        0xf1, 0xe7,

        0x00, 0xbe,
        0x00, 0xbf,

        0x00, 0x3c, 0x02, 0x40,
    };

    static const uint8_t loader_code_stm32l4[] = {
        // flashloaders/stm32l4.s
        0x08, 0x4b,             // start: ldr   r3, [pc, #32] ; <flash_base>
        0x72, 0xb1,             // next:  cbz   r2, <done>
        0x04, 0x68,             //        ldr   r4, [r0, #0]
        0x45, 0x68,             //        ldr   r5, [r0, #4]
        0x0c, 0x60,             //        str   r4, [r1, #0]
        0x4d, 0x60,             //        str   r5, [r1, #4]
        0x5c, 0x8a,             // wait:  ldrh  r4, [r3, #18]
        0x14, 0xf0, 0x01, 0x0f, //        tst.w r4, #1
        0xfb, 0xd1,             //        bne.n <wait>
        0x00, 0xf1, 0x08, 0x00, //        add.w r0, r0, #8
        0x01, 0xf1, 0x08, 0x01, //        add.w r1, r1, #8
        0xa2, 0xf1, 0x01, 0x02, //        sub.w r2, r2, #1
        0xef, 0xe7,             //        b.n   <next>
        0x00, 0xbe,             // done:  bkpt  0x0000
        0x00, 0x20, 0x02, 0x40  // flash_base:  .word 0x40022000
    };

	static const uint8_t loader_code_stm32f7[] = {
        // flashloaders/stm32f7.s
        0x08, 0x4b,
        0x72, 0xb1,
        0x04, 0x68,
        0x0c, 0x60,
        0xbf, 0xf3, 0x4f, 0x8f,        // DSB Memory barrier for in order flash write
        0xdc, 0x89,
        0x14, 0xf0, 0x01, 0x0f,
        0xfb, 0xd1,
        0x00, 0xf1, 0x04, 0x00,
        0x01, 0xf1, 0x04, 0x01,
        0xa2, 0xf1, 0x01, 0x02,
        0xef, 0xe7,
        0x00, 0xbe,                   //     bkpt	#0x00
        0x00, 0x3c, 0x02, 0x40,
    };

    static const uint8_t loader_code_stm32f7_lv[] = {
        // flashloaders/stm32f7lv.s
        0x92, 0x00,             //      lsls    r2, r2, #2
        0x09, 0x4b,             //      ldr r3, [pc, #36]   ; (0x20000028 <flash_base>)
                                // next:
        0x72, 0xb1,             //      cbz     r2, 24 <done>
        0x04, 0x78,             //      ldrb    r4, [r0, #0]
        0x0c, 0x70,             //      strb    r4, [r1, #0]
        0xbf, 0xf3, 0x4f, 0x8f, //      dsb sy
                                // wait:
        0xdc, 0x89,             //      ldrh    r4, [r3, #14]
        0x14, 0xf0, 0x01, 0x0f, //      tst.w   r4, #1
        0xfb, 0xd1,             //      bne.n   e <wait>
        0x00, 0xf1, 0x01, 0x00, //      add     r0, r0, #1
        0x01, 0xf1, 0x01, 0x01, //      add     r1, r1, #1
        0xa2, 0xf1, 0x01, 0x02, //      sub     r2, r2, #1
        0xef, 0xe7,             //      b       next
                                // done:
        0x00, 0xbe,             //      bkpt
        0x00, 0xbf,             //      nop
                                // flash_base:
        0x00, 0x3c, 0x02, 0x40  //      .word   0x40023c00
    };



int stlink_flash_loader_init(stlink_t *sl, flash_loader_t *fl)
{
	size_t size;

	/* allocate the loader in sram */
	if (stlink_flash_loader_write_to_sram(sl, &fl->loader_addr, &size) == -1) {
		WLOG("Failed to write flash loader to sram!\n");
		return -1;
	}

	/* allocate a one page buffer in sram right after loader */
	fl->buf_addr = fl->loader_addr + (uint32_t) size;
	ILOG("Successfully loaded flash loader in sram\n");

	return 0;
}

static int loader_v_dependent_assignment(stlink_t *sl, 
                                         const uint8_t **loader_code, size_t *loader_size,
                                         const uint8_t *high_v_loader, size_t high_v_loader_size,
                                         const uint8_t *low_v_loader, size_t low_v_loader_size)
{
    int retval = 0;

    if( sl->version.stlink_v == 1 ) {
        printf("STLINK V1 cannot read voltage, defaulting to 32-bit writes\n");
        *loader_code = high_v_loader;
        *loader_size = high_v_loader_size;
    }
    else {
        int voltage = stlink_target_voltage(sl);
        if (voltage == -1) {
            retval = -1;
            printf("Failed to read Target voltage\n");
        } 
        else {
            if (voltage > 2700) {
                *loader_code = high_v_loader;
                *loader_size = high_v_loader_size;
            } else {
                *loader_code = low_v_loader;
                *loader_size = low_v_loader_size;
            }
        }
    }
    return retval;
}

int stlink_flash_loader_write_to_sram(stlink_t *sl, stm32_addr_t* addr, size_t* size)
{
    const uint8_t* loader_code;
    size_t loader_size;

    if (sl->chip_id == STLINK_CHIPID_STM32_L1_MEDIUM || sl->chip_id == STLINK_CHIPID_STM32_L1_CAT2
            || sl->chip_id == STLINK_CHIPID_STM32_L1_MEDIUM_PLUS || sl->chip_id == STLINK_CHIPID_STM32_L1_HIGH
            || sl->chip_id == STLINK_CHIPID_STM32_L152_RE
            || sl->chip_id == STLINK_CHIPID_STM32_L0 || sl->chip_id == STLINK_CHIPID_STM32_L0_CAT5 || sl->chip_id == STLINK_CHIPID_STM32_L0_CAT2) { /* stm32l */
        loader_code = loader_code_stm32l;
        loader_size = sizeof(loader_code_stm32l);
    } else if (sl->core_id == STM32VL_CORE_ID
            || sl->chip_id == STLINK_CHIPID_STM32_F3
            || sl->chip_id == STLINK_CHIPID_STM32_F3_SMALL
            || sl->chip_id == STLINK_CHIPID_STM32_F303_HIGH
            || sl->chip_id == STLINK_CHIPID_STM32_F37x
            || sl->chip_id == STLINK_CHIPID_STM32_F334) {
        loader_code = loader_code_stm32vl;
        loader_size = sizeof(loader_code_stm32vl);
    } else if (sl->chip_id == STLINK_CHIPID_STM32_F2      ||
		sl->chip_id == STLINK_CHIPID_STM32_F4     ||
		sl->chip_id == STLINK_CHIPID_STM32_F4_DE  ||
		sl->chip_id == STLINK_CHIPID_STM32_F4_LP  ||
		sl->chip_id == STLINK_CHIPID_STM32_F4_HD  ||
		sl->chip_id == STLINK_CHIPID_STM32_F4_DSI ||
		sl->chip_id == STLINK_CHIPID_STM32_F410   ||
		sl->chip_id == STLINK_CHIPID_STM32_F411RE ||
		sl->chip_id == STLINK_CHIPID_STM32_F412   ||
		sl->chip_id == STLINK_CHIPID_STM32_F413   ||
		sl->chip_id == STLINK_CHIPID_STM32_F446
		) {
        int retval;
        retval = loader_v_dependent_assignment(sl,
                                               &loader_code, &loader_size,
                                               loader_code_stm32f4, sizeof(loader_code_stm32f4),
                                               loader_code_stm32f4_lv, sizeof(loader_code_stm32f4_lv));
        if (retval == -1) {
            return retval;
        }
    } else if (sl->core_id == STM32F7_CORE_ID ||
               sl->chip_id == STLINK_CHIPID_STM32_F7 ||
               sl->chip_id == STLINK_CHIPID_STM32_F7XXXX) {
        int retval;
        retval = loader_v_dependent_assignment(sl,
                                               &loader_code, &loader_size,
                                               loader_code_stm32f7, sizeof(loader_code_stm32f7),
                                               loader_code_stm32f7_lv, sizeof(loader_code_stm32f7_lv));
        if (retval == -1) {
            return retval;
        }
    } else if (sl->chip_id == STLINK_CHIPID_STM32_F0 || sl->chip_id == STLINK_CHIPID_STM32_F04 || sl->chip_id == STLINK_CHIPID_STM32_F0_CAN || sl->chip_id == STLINK_CHIPID_STM32_F0_SMALL || sl->chip_id == STLINK_CHIPID_STM32_F09X) {
        loader_code = loader_code_stm32f0;
        loader_size = sizeof(loader_code_stm32f0);
    } else if ((sl->chip_id == STLINK_CHIPID_STM32_L4) ||
              (sl->chip_id == STLINK_CHIPID_STM32_L43X) ||
              (sl->chip_id == STLINK_CHIPID_STM32_L46X) ||
	       (sl->chip_id == STLINK_CHIPID_STM32_L496X))
      {
        loader_code = loader_code_stm32l4;
        loader_size = sizeof(loader_code_stm32l4);
    } else {
        ELOG("unknown coreid, not sure what flash loader to use, aborting! coreid: %x, chipid: %x\n", sl->core_id, sl->chip_id);
        return -1;
    }

    memcpy(sl->q_buf, loader_code, loader_size);
    stlink_write_mem32(sl, sl->sram_base, loader_size);

    *addr = sl->sram_base;
    *size = loader_size;

    /* success */
    return 0;
}

int stlink_flash_loader_run(stlink_t *sl, flash_loader_t* fl, stm32_addr_t target, const uint8_t* buf, size_t size)
{
    struct stlink_reg rr;
    int i = 0;
    size_t count = 0;
    uint32_t flash_base = 0;

    DLOG("Running flash loader, write address:%#x, size: %u\n", target, (unsigned int)size);
    // FIXME This can never return -1
    if (write_buffer_to_sram(sl, fl, buf, size) == -1) {
        // IMPOSSIBLE!
        ELOG("write_buffer_to_sram() == -1\n");
        return -1;
    }

    if ((sl->flash_type == STLINK_FLASH_TYPE_F0) || (sl->flash_type == STLINK_FLASH_TYPE_F1_XL)) {
        count = size / sizeof(uint16_t);
        if (size % sizeof(uint16_t))
            ++count;
    } else if (sl->flash_type == STLINK_FLASH_TYPE_F4 || sl->flash_type == STLINK_FLASH_TYPE_L0) {
        count = size / sizeof(uint32_t);
        if (size % sizeof(uint32_t))
            ++count;
    } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
        count = size / sizeof(uint64_t);
        if (size % sizeof(uint64_t))
            ++count;
    }

    if ((sl->flash_type == STLINK_FLASH_TYPE_F1_XL) && (target >= FLASH_BANK2_START_ADDR)) {
        flash_base = FLASH_REGS_BANK2_OFS;
    }

    /* setup core */
    stlink_write_reg(sl, fl->buf_addr, 0); /* source */
    stlink_write_reg(sl, target, 1); /* target */
    stlink_write_reg(sl, (uint32_t) count, 2); /* count */
    stlink_write_reg(sl, flash_base, 3); /* flash register base, only used on VL/F1_XL, but harmless for others */
    stlink_write_reg(sl, fl->loader_addr, 15); /* pc register */

    /* run loader */
    stlink_run(sl);

#define WAIT_ROUNDS 10000
    /* wait until done (reaches breakpoint) */
    for (i = 0; i < WAIT_ROUNDS; i++) {
        usleep(10);
        if (stlink_is_core_halted(sl))
            break;
    }

    if (i >= WAIT_ROUNDS) {
        ELOG("flash loader run error\n");
        return -1;
    }

    /* check written byte count */
    stlink_read_reg(sl, 2, &rr);
    if (rr.r[2] != 0) {
        ELOG("write error, count == %u\n", rr.r[2]);
        return -1;
    }

    return 0;
}
