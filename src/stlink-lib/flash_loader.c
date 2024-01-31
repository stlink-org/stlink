/*
 * File: flash_loader.c
 *
 * Flash loaders
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <stm32.h>
#include <stlink.h>
#include "flash_loader.h"

#include "common_flash.h"
#include "helper.h"
#include "logging.h"
#include "read_write.h"
#include "register.h"

#define FLASH_REGS_BANK2_OFS      0x40
#define FLASH_BANK2_START_ADDR    0x08080000

#define STM32F0_WDG_KR            0x40003000
#define STM32H7_WDG_KR            0x58004800

#define STM32F0_WDG_KR_KEY_RELOAD 0xAAAA

/*
 * !!! DO NOT MODIFY FLASH LOADERS DIRECTLY !!!
 *
 * Edit assembly files in the '/flashloaders' instead. The sizes of binary
 * flash loaders must be aligned by 4 (it's written by stlink_write_mem32)
 */

// flashloaders/stm32f0.s -- compiled with thumb2
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

// flashloaders/stm32f0.s -- thumb1 only, same sequence as for STM32VL, bank ignored
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

// flashloaders/stm32lx.s -- compiled for armv6-m for compatibility with both
// armv6-m cores (STM32L0) and armv7-m cores (STM32L1)
static const uint8_t loader_code_stm32lx[] = {
    0x04, 0x68, 0x0c, 0x60,
    0x04, 0x30, 0x04, 0x31,
    0x04, 0x3a, 0xf9, 0xdc,
    0x00, 0xbe, 0x00, 0x00
};

// flashloaders/stm32f4.s
static const uint8_t loader_code_stm32f4[] = {
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

// flashloaders/stm32f4lv.s
static const uint8_t loader_code_stm32f4_lv[] = {
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

// flashloaders/stm32l4.s
static const uint8_t loader_code_stm32l4[] = {
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

// flashloaders/stm32f7.s
static const uint8_t loader_code_stm32f7[] = {
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

// flashloaders/stm32f7lv.s
static const uint8_t loader_code_stm32f7_lv[] = {
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


int32_t stlink_flash_loader_init(stlink_t *sl, flash_loader_t *fl) {
    uint32_t size = 0;
    uint32_t dfsr, cfsr, hfsr;

    /* Interrupt masking according to DDI0419C, Table C1-7 firstly force halt */
    stlink_write_debug32(sl, STLINK_REG_DHCSR,
                           STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_DEBUGEN |
                           STLINK_REG_DHCSR_C_HALT);
    /* and only then disable interrupts */
    stlink_write_debug32(sl, STLINK_REG_DHCSR,
                           STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_DEBUGEN |
                           STLINK_REG_DHCSR_C_HALT | STLINK_REG_DHCSR_C_MASKINTS);

    // allocate the loader in SRAM
    if (stlink_flash_loader_write_to_sram(sl, &fl->loader_addr, &size) == -1) {
        WLOG("Failed to write flash loader to sram!\n");
        return (-1);
    }

    // allocate a one page buffer in SRAM right after loader
    fl->buf_addr = fl->loader_addr + size;
    ILOG("Successfully loaded flash loader in sram\n");

    // set address of IWDG key register for reset it
    if (sl->flash_type == STM32_FLASH_TYPE_H7) {
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

    return (0);
}

static int32_t loader_v_dependent_assignment(stlink_t *sl,
                                            const uint8_t **loader_code, uint32_t *loader_size,
                                            const uint8_t *high_v_loader, uint32_t high_v_loader_size,
                                            const uint8_t *low_v_loader, uint32_t low_v_loader_size) {
    int32_t retval = 0;

    if ( sl->version.stlink_v == 1) {
        printf("STLINK V1 cannot read voltage, defaulting to 32-bit writes\n");
        *loader_code = high_v_loader;
        *loader_size = high_v_loader_size;
    } else {
        int32_t voltage = stlink_target_voltage(sl);

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

    return (retval);
}

int32_t stlink_flash_loader_write_to_sram(stlink_t *sl, stm32_addr_t* addr, uint32_t* size) {
    const uint8_t* loader_code;
    uint32_t loader_size;

    if (sl->chip_id == STM32_CHIPID_L1_MD ||
        sl->chip_id == STM32_CHIPID_L1_CAT2 ||
        sl->chip_id == STM32_CHIPID_L1_MD_PLUS ||
        sl->chip_id == STM32_CHIPID_L1_MD_PLUS_HD ||
        sl->chip_id == STM32_CHIPID_L152_RE ||
        sl->chip_id == STM32_CHIPID_L0_CAT1 ||
        sl->chip_id == STM32_CHIPID_L0_CAT2 ||
        sl->chip_id == STM32_CHIPID_L0_CAT3 ||
        sl->chip_id == STM32_CHIPID_L0_CAT5) {
        loader_code = loader_code_stm32lx;
        loader_size = sizeof(loader_code_stm32lx);
    } else if (sl->core_id == STM32_CORE_ID_M3_r1p1_SWD ||
               sl->chip_id == STM32_CHIPID_F1_MD ||
               sl->chip_id == STM32_CHIPID_F1_HD ||
               sl->chip_id == STM32_CHIPID_F1_LD ||
               sl->chip_id == STM32_CHIPID_F1_VL_MD_LD ||
               sl->chip_id == STM32_CHIPID_F1_VL_HD ||
               sl->chip_id == STM32_CHIPID_F1_XLD ||
               sl->chip_id == STM32_CHIPID_F1_CONN ||
               sl->chip_id == STM32_CHIPID_F3 ||
               sl->chip_id == STM32_CHIPID_F3xx_SMALL ||
               sl->chip_id == STM32_CHIPID_F303_HD ||
               sl->chip_id == STM32_CHIPID_F37x ||
               sl->chip_id == STM32_CHIPID_F334) {
        loader_code = loader_code_stm32vl;
        loader_size = sizeof(loader_code_stm32vl);
    } else if (sl->chip_id == STM32_CHIPID_F2 ||
               sl->chip_id == STM32_CHIPID_F4 ||
               sl->chip_id == STM32_CHIPID_F4_DE ||
               sl->chip_id == STM32_CHIPID_F4_LP ||
               sl->chip_id == STM32_CHIPID_F4_HD ||
               sl->chip_id == STM32_CHIPID_F4_DSI ||
               sl->chip_id == STM32_CHIPID_F410 ||
               sl->chip_id == STM32_CHIPID_F411xx ||
               sl->chip_id == STM32_CHIPID_F412 ||
               sl->chip_id == STM32_CHIPID_F413 ||
               sl->chip_id == STM32_CHIPID_F446) {
        int32_t retval;
        retval = loader_v_dependent_assignment(sl,
                                               &loader_code, &loader_size,
                                               loader_code_stm32f4, sizeof(loader_code_stm32f4),
                                               loader_code_stm32f4_lv, sizeof(loader_code_stm32f4_lv));

        if (retval == -1) { return (retval); }
    } else if (sl->core_id == STM32_CORE_ID_M7F_SWD ||
               sl->chip_id == STM32_CHIPID_F7 ||
               sl->chip_id == STM32_CHIPID_F76xxx ||
               sl->chip_id == STM32_CHIPID_F72xxx) {
        int32_t retval;
        retval = loader_v_dependent_assignment(sl,
                                               &loader_code, &loader_size,
                                               loader_code_stm32f7, sizeof(loader_code_stm32f7),
                                               loader_code_stm32f7_lv, sizeof(loader_code_stm32f7_lv));

        if (retval == -1) { return (retval); }
    } else if (sl->chip_id == STM32_CHIPID_F0 ||
               sl->chip_id == STM32_CHIPID_F04 ||
               sl->chip_id == STM32_CHIPID_F0_CAN ||
               sl->chip_id == STM32_CHIPID_F0xx_SMALL ||
               sl->chip_id == STM32_CHIPID_F09x) {
        loader_code = loader_code_stm32f0;
        loader_size = sizeof(loader_code_stm32f0);
    } else if ((sl->chip_id == STM32_CHIPID_L4) ||
               (sl->chip_id == STM32_CHIPID_L41x_L42x) ||
               (sl->chip_id == STM32_CHIPID_L43x_L44x) ||
               (sl->chip_id == STM32_CHIPID_L45x_L46x) ||
               (sl->chip_id == STM32_CHIPID_L4Rx) ||
               (sl->chip_id == STM32_CHIPID_L496x_L4A6x)) {
        loader_code = loader_code_stm32l4;
        loader_size = sizeof(loader_code_stm32l4);
    } else {
        ELOG("unknown coreid, not sure what flash loader to use, aborting! coreid: %x, chipid: %x\n",
            sl->core_id, sl->chip_id);
        return (-1);
    }

    memcpy(sl->q_buf, loader_code, loader_size);
    int32_t ret = stlink_write_mem32(sl, sl->sram_base, (uint16_t)loader_size);

    if (ret) { return (ret); }

    *addr = sl->sram_base;
    *size = loader_size;

    return (0); // success
}

int32_t stlink_flash_loader_run(stlink_t *sl, flash_loader_t* fl, stm32_addr_t target, const uint8_t* buf, uint32_t size) {
    struct stlink_reg rr;
    uint32_t timeout;
    uint32_t flash_base = 0;
    uint32_t dhcsr, dfsr, cfsr, hfsr;

    DLOG("Running flash loader, write address:%#x, size: %u\n", target, size);

    if (write_buffer_to_sram(sl, fl, buf, size) == -1) {
        ELOG("write_buffer_to_sram() == -1\n");
        return (-1);
    }

    if ((sl->flash_type == STM32_FLASH_TYPE_F1_XL) && (target >= FLASH_BANK2_START_ADDR)) {
        flash_base = FLASH_REGS_BANK2_OFS;
    }

  /* Setup core */
  stlink_write_reg(sl, fl->buf_addr, 0);     // source
  stlink_write_reg(sl, target, 1);           // target
  stlink_write_reg(sl, size, 2);   // count
  stlink_write_reg(sl, flash_base, 3);       // flash register base
                                             // only used on VL/F1_XL, but harmless for others
  stlink_write_reg(sl, fl->loader_addr, 15); // pc register

  /* Reset IWDG */
  if (fl->iwdg_kr) {
      stlink_write_debug32(sl, fl->iwdg_kr, STM32F0_WDG_KR_KEY_RELOAD);
  }

  /* Run loader */
  stlink_run(sl, RUN_FLASH_LOADER);

/*
 * This piece of code used to try to spin for .1 second by waiting doing 10000 rounds of 10 µs.
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

  /*
  * The chunk size for loading is not rounded. The flash loader
  * subtracts the size of the written block (1-8 bytes) from
  * the remaining size each time. A negative value may mean that
  * several bytes garbage have been written due to the unaligned
  * firmware size.
  */
  if ((int32_t)rr.r[2] > 0 || (int32_t)rr.r[2] < -7) {
      ELOG("Flash loader write error\n");
      goto error;
  }

  return (0);

  error:
      dhcsr = dfsr = cfsr = hfsr = 0;
      stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr);
      stlink_read_debug32(sl, STLINK_REG_DFSR, &dfsr);
      stlink_read_debug32(sl, STLINK_REG_CFSR, &cfsr);
      stlink_read_debug32(sl, STLINK_REG_HFSR, &hfsr);
      stlink_read_all_regs(sl, &rr);

      WLOG("Loader state: R2 0x%X R15 0x%X\n", rr.r[2], rr.r[15]);
      if (dhcsr != 0x3000B || dfsr || cfsr || hfsr) {
          WLOG("MCU state: DHCSR 0x%X DFSR 0x%X CFSR 0x%X HFSR 0x%X\n", dhcsr, dfsr, cfsr, hfsr);
      }

  return (-1);
}


/* === Content from old source file flashloader.c === */

#define L1_WRITE_BLOCK_SIZE 0x80
#define L0_WRITE_BLOCK_SIZE 0x40

int32_t stm32l1_write_half_pages(stlink_t *sl, flash_loader_t *fl, stm32_addr_t addr, uint8_t *base, uint32_t len, uint32_t pagesize) {
  uint32_t count, off;
  uint32_t num_half_pages = len / pagesize;
  uint32_t val;
  uint32_t flash_regs_base = get_stm32l0_flash_base(sl);
  bool use_loader = true;
  int32_t ret = 0;

  // enable half page write
  stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
  val |= (1 << FLASH_L1_FPRG);
  stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
  val |= (1 << FLASH_L1_PROG);
  stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);

  wait_flash_busy(sl);

  for (count = 0; count < num_half_pages; count++) {
    if (use_loader) {
      ret = stlink_flash_loader_run(sl, fl, addr + count * pagesize, base + count * pagesize, pagesize);
      if (ret && count == 0) {
        /* It seems that stm32lx devices have a problem when it is blank */
        WLOG("Failed to use flash loader, fallback to soft write\n");
        use_loader = false;
      }
    }
    if (!use_loader) {
      ret = 0;
      for (off = 0; off < pagesize && !ret; off += 64) {
        uint32_t chunk = (pagesize - off > 64) ? 64 : pagesize - off;
        memcpy(sl->q_buf, base + count * pagesize + off, chunk);
        ret = stlink_write_mem32(sl, addr + count * pagesize + off, (uint16_t)chunk);
      }
    }

    if (ret) {
      WLOG("l1_stlink_flash_loader_run(%#x) failed! == -1\n", addr + count * pagesize);
      break;
    }

    if (sl->verbose >= 1) {
      // show progress; writing procedure is slow and previous errors are misleading
      fprintf(stdout, "\r%3u/%3u halfpages written", count + 1, num_half_pages);
      fflush(stdout);
    }

    // wait for sr.busy to be cleared
    wait_flash_busy(sl);
  }

  // disable half page write
  stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
  val &= ~((1 << FLASH_L1_FPRG) | (1 << FLASH_L1_PROG));
  stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
  return (ret);
}

static void set_flash_cr_pg(stlink_t *sl, uint32_t bank) {
  uint32_t cr_reg, x;

  x = read_flash_cr(sl, bank);

  if (sl->flash_type == STM32_FLASH_TYPE_C0) {
    cr_reg = FLASH_C0_CR;
    x |= (1 << FLASH_CR_PG);
  } else if (sl->flash_type == STM32_FLASH_TYPE_F2_F4) {
    cr_reg = FLASH_F4_CR;
    x |= (1 << FLASH_CR_PG);
  } else if (sl->flash_type == STM32_FLASH_TYPE_F7) {
    cr_reg = FLASH_F7_CR;
    x |= (1 << FLASH_CR_PG);
  } else if (sl->flash_type == STM32_FLASH_TYPE_L4) {
    cr_reg = FLASH_L4_CR;
    x &= ~FLASH_L4_CR_OPBITS;
    x |= (1 << FLASH_L4_CR_PG);
  } else if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) {
    cr_reg = FLASH_L5_NSCR;
    x |= (1 << FLASH_CR_PG);
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    cr_reg = FLASH_Gx_CR;
    x |= (1 << FLASH_CR_PG);
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
    cr_reg = FLASH_WB_CR;
    x |= (1 << FLASH_CR_PG);
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    cr_reg = (bank == BANK_1) ? FLASH_H7_CR1 : FLASH_H7_CR2;
    x |= (1 << FLASH_H7_CR_PG);
  } else {
    cr_reg = (bank == BANK_1) ? FLASH_CR : FLASH_CR2;
    x = (1 << FLASH_CR_PG);
  }

  stlink_write_debug32(sl, cr_reg, x);
}

static void set_dma_state(stlink_t *sl, flash_loader_t *fl, int32_t bckpRstr) {
  uint32_t rcc, rcc_dma_mask, value;

  rcc = rcc_dma_mask = value = 0;

  switch (sl->flash_type) {
  case STM32_FLASH_TYPE_C0:
    rcc = STM32C0_RCC_AHBENR;
    rcc_dma_mask = STM32C0_RCC_DMAEN;
    break;
  case STM32_FLASH_TYPE_F0_F1_F3:
  case STM32_FLASH_TYPE_F1_XL:
    rcc = STM32F1_RCC_AHBENR;
    rcc_dma_mask = STM32F1_RCC_DMAEN;
    break;
  case STM32_FLASH_TYPE_F2_F4:
  case STM32_FLASH_TYPE_F7:
    rcc = STM32F4_RCC_AHB1ENR;
    rcc_dma_mask = STM32F4_RCC_DMAEN;
    break;
  case STM32_FLASH_TYPE_G0:
    rcc = STM32G0_RCC_AHBENR;
    rcc_dma_mask = STM32G0_RCC_DMAEN;
    break;
  case STM32_FLASH_TYPE_G4:
  case STM32_FLASH_TYPE_L4:
    rcc = STM32G4_RCC_AHB1ENR;
    rcc_dma_mask = STM32G4_RCC_DMAEN;
    break;
  case STM32_FLASH_TYPE_L0_L1:
    if (get_stm32l0_flash_base(sl) == FLASH_Lx_REGS_ADDR) {
      rcc = STM32L1_RCC_AHBENR;
      rcc_dma_mask = STM32L1_RCC_DMAEN;
    } else {
      rcc = STM32L0_RCC_AHBENR;
      rcc_dma_mask = STM32L0_RCC_DMAEN;
    }
    break;
  case STM32_FLASH_TYPE_L5_U5_H5:
    rcc = STM32L5_RCC_AHB1ENR;
    rcc_dma_mask = STM32L5_RCC_DMAEN;
    break;
  case STM32_FLASH_TYPE_H7:
    rcc = STM32H7_RCC_AHB1ENR;
    rcc_dma_mask = STM32H7_RCC_DMAEN;
    break;
  case STM32_FLASH_TYPE_WB_WL:
    rcc = STM32WB_RCC_AHB1ENR;
    rcc_dma_mask = STM32WB_RCC_DMAEN;
    break;
  default:
    return;
  }

  if (!stlink_read_debug32(sl, rcc, &value)) {
    if (bckpRstr) {
      value = (value & (~rcc_dma_mask)) | fl->rcc_dma_bkp;
    } else {
      fl->rcc_dma_bkp = value & rcc_dma_mask;
      value &= ~rcc_dma_mask;
    }
    stlink_write_debug32(sl, rcc, value);
  }
}

int32_t stlink_flashloader_start(stlink_t *sl, flash_loader_t *fl) {
  // disable DMA
  set_dma_state(sl, fl, 0);

  // wait for ongoing op to finish
  wait_flash_busy(sl);
  // Clear errors
  clear_flash_error(sl);

  if ((sl->flash_type == STM32_FLASH_TYPE_F2_F4) ||
      (sl->flash_type == STM32_FLASH_TYPE_F7) ||
      (sl->flash_type == STM32_FLASH_TYPE_L4)) {
    ILOG("Starting Flash write for F2/F4/F7/L4\n");

    // Flash loader initialisation
    if (stlink_flash_loader_init(sl, fl) == -1) {
      ELOG("stlink_flash_loader_init() == -1\n");
      return (-1);
    }

    unlock_flash_if(sl); // first unlock the cr

    int32_t voltage;
    if (sl->version.stlink_v == 1) {
      WLOG("STLINK V1 cannot read voltage, use default voltage 3.2V\n");
      voltage = 3200;
    } else {
      voltage = stlink_target_voltage(sl);
    }

    if (voltage == -1) {
      ELOG("Failed to read Target voltage\n");
      return (-1);
    }

    if (sl->flash_type == STM32_FLASH_TYPE_L4) {
      // L4 does not have a byte-write mode
      if (voltage < 1710) {
        ELOG("Target voltage (%d mV) too low for flash writes!\n", voltage);
        return (-1);
      }
    } else {
      if (voltage > 2700) {
        ILOG("enabling 32-bit flash writes\n");
        write_flash_cr_psiz(sl, 2, BANK_1);
      } else {
        ILOG("Target voltage (%d mV) too low for 32-bit flash, using 8-bit flash writes\n", voltage);
        write_flash_cr_psiz(sl, 0, BANK_1);
      }
    }

    // set programming mode
    set_flash_cr_pg(sl, BANK_1);
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL ||
             sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4 ||
             sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5 ||
             sl->flash_type == STM32_FLASH_TYPE_C0) {
    ILOG("Starting Flash write for WB/G0/G4/L5/U5/H5/C0\n");

    unlock_flash_if(sl);         // unlock flash if necessary
    set_flash_cr_pg(sl, BANK_1); // set PG 'allow programming' bit
  } else if (sl->flash_type == STM32_FLASH_TYPE_L0_L1) {
    ILOG("Starting Flash write for L0\n");

    uint32_t val;
    uint32_t flash_regs_base = get_stm32l0_flash_base(sl);

    // disable pecr protection
    stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF, FLASH_L0_PEKEY1);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF, FLASH_L0_PEKEY2);

    // check pecr.pelock is cleared
    stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
    if (val & (1 << 0)) {
      ELOG("pecr.pelock not clear\n");
      return (-1);
    }

    // unlock program memory
    stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, FLASH_L0_PRGKEY1);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, FLASH_L0_PRGKEY2);

    // check pecr.prglock is cleared
    stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
    if (val & (1 << 1)) {
      ELOG("pecr.prglock not clear\n");
      return (-1);
    }

    /* Flash loader initialisation */
    if (stlink_flash_loader_init(sl, fl) == -1) {
      // L0/L1 have fallback to soft write
      WLOG("stlink_flash_loader_init() == -1\n");
    }
  } else if ((sl->flash_type == STM32_FLASH_TYPE_F0_F1_F3) ||
             (sl->flash_type == STM32_FLASH_TYPE_F1_XL)) {
    ILOG("Starting Flash write for VL/F0/F3/F1_XL\n");

    // flash loader initialisation
    if (stlink_flash_loader_init(sl, fl) == -1) {
      ELOG("stlink_flash_loader_init() == -1\n");
      return (-1);
    }

    // unlock flash
    unlock_flash_if(sl);

    // set programming mode
    set_flash_cr_pg(sl, BANK_1);
    if (sl->flash_type == STM32_FLASH_TYPE_F1_XL) {
      set_flash_cr_pg(sl, BANK_2);
    }
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    ILOG("Starting Flash write for H7\n");

    unlock_flash_if(sl);         // unlock the cr
    set_flash_cr_pg(sl, BANK_1); // set programming mode
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
      set_flash_cr_pg(sl, BANK_2);
    }
    if (sl->chip_id != STM32_CHIPID_H7Ax) {
      // set parallelism
      write_flash_cr_psiz(sl, 3 /* 64bit */, BANK_1);
      if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
        write_flash_cr_psiz(sl, 3 /* 64bit */, BANK_2);
      }
    }
  } else {
    ELOG("unknown coreid, not sure how to write: %x\n", sl->core_id);
    return (-1);
  }

  return (0);
}

int32_t stlink_flashloader_write(stlink_t *sl, flash_loader_t *fl, stm32_addr_t addr, uint8_t *base, uint32_t len) {
  uint32_t off;

  if ((sl->flash_type == STM32_FLASH_TYPE_F2_F4) ||
      (sl->flash_type == STM32_FLASH_TYPE_F7) ||
      (sl->flash_type == STM32_FLASH_TYPE_L4)) {
    uint32_t buf_size = (sl->sram_size > 0x8000) ? 0x8000 : 0x4000;
    for (off = 0; off < len;) {
      uint32_t size = len - off > buf_size ? buf_size : len - off;
      if (stlink_flash_loader_run(sl, fl, addr + off, base + off, size) == -1) {
        ELOG("stlink_flash_loader_run(%#x) failed! == -1\n", (addr + off));
        check_flash_error(sl);
        return (-1);
      }

      off += size;
    }
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL ||
             sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4 ||
             sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5 ||
             sl->flash_type == STM32_FLASH_TYPE_C0) {
  
    if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5 && (len % 16)) {
        WLOG("Data size is aligned to 16 byte");
        len += 16 - len%16;
    }
    DLOG("Starting %3u page write\n", len / sl->flash_pgsz);
    for (off = 0; off < len; off += sizeof(uint32_t)) {
      uint32_t data;

      if ((off % sl->flash_pgsz) > (sl->flash_pgsz - 5)) {
        fprintf(stdout, "\r%3u/%-3u pages written", (off / sl->flash_pgsz + 1), (len / sl->flash_pgsz));
        fflush(stdout);
      }

      // write_uint32((unsigned char *)&data, *(uint32_t *)(base + off));
      data = 0;
      memcpy(&data, base + off, (len - off) < 4 ? (len - off) : 4);
      stlink_write_debug32(sl, addr + off, data);
      wait_flash_busy(sl); // wait for 'busy' bit in FLASH_SR to clear
    }
    fprintf(stdout, "\n");

    // flash writes happen as 2 words at a time
    if ((off / sizeof(uint32_t)) % 2 != 0) {
      stlink_write_debug32(sl, addr + off, 0); // write a single word of zeros
      wait_flash_busy(sl); // wait for 'busy' bit in FLASH_SR to clear
    }
  } else if (sl->flash_type == STM32_FLASH_TYPE_L0_L1) {
    uint32_t val;
    uint32_t flash_regs_base = get_stm32l0_flash_base(sl);
    uint32_t pagesize = (flash_regs_base == FLASH_L0_REGS_ADDR)? L0_WRITE_BLOCK_SIZE : L1_WRITE_BLOCK_SIZE;

    DLOG("Starting %3u page write\r\n", len / sl->flash_pgsz);

    off = 0;

    if (len > pagesize) {
      if (stm32l1_write_half_pages(sl, fl, addr, base, len, pagesize)) {
        return (-1);
      } else {
        off = (size_t)(len / pagesize) * pagesize;
      }
    }

    // write remaining word in program memory
    for (; off < len; off += sizeof(uint32_t)) {
      uint32_t data;

      if ((off % sl->flash_pgsz) > (sl->flash_pgsz - 5)) {
        fprintf(stdout, "\r%3u/%-3u pages written", (off / sl->flash_pgsz + 1), (len / sl->flash_pgsz));
        fflush(stdout);
      }

      write_uint32((unsigned char *)&data, *(uint32_t *)(base + off));
      stlink_write_debug32(sl, addr + off, data);

      // wait for sr.busy to be cleared
      do {
        stlink_read_debug32(sl, flash_regs_base + FLASH_SR_OFF, &val);
      } while ((val & (1 << 0)) != 0);

      // TODO: check redo write operation
    }
    fprintf(stdout, "\n");
  } else if ((sl->flash_type == STM32_FLASH_TYPE_F0_F1_F3) || (sl->flash_type == STM32_FLASH_TYPE_F1_XL)) {
    int32_t write_block_count = 0;
    for (off = 0; off < len; off += sl->flash_pgsz) {
      // adjust last write size
      uint32_t size = len - off > sl->flash_pgsz ? sl->flash_pgsz : len - off;

      // unlock and set programming mode
      unlock_flash_if(sl);

      DLOG("Finished unlocking flash, running loader!\n");

      if (stlink_flash_loader_run(sl, fl, addr + off, base + off, size) == -1) {
        ELOG("stlink_flash_loader_run(%#x) failed! == -1\n", (addr + off));
        check_flash_error(sl);
        return (-1);
      }

      lock_flash(sl);

      if (sl->verbose >= 1) {
        // show progress; writing procedure is slow and previous errors are
        // misleading
        fprintf(stdout, "\r%3u/%-3u pages written", ++write_block_count,
                (len + sl->flash_pgsz - 1) / sl->flash_pgsz);
        fflush(stdout);
      }
    }
    if (sl->verbose >= 1) {
      fprintf(stdout, "\n");
    }
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    for (off = 0; off < len;) {
      // Program STM32H7x with 64-byte Flash words
      uint32_t chunk = (len - off > 64) ? 64 : len - off;
      memcpy(sl->q_buf, base + off, chunk);
      stlink_write_mem32(sl, addr + off, 64);
      wait_flash_busy(sl);

      off += chunk;

      if (sl->verbose >= 1) {
        // show progress
        fprintf(stdout, "\r%u/%u bytes written", off, len);
        fflush(stdout);
      }
    }
    if (sl->verbose >= 1) {
      fprintf(stdout, "\n");
    }
  } else {
    return (-1);
  }

  return check_flash_error(sl);
}

int32_t stlink_flashloader_stop(stlink_t *sl, flash_loader_t *fl) {
  uint32_t dhcsr;

  if ((sl->flash_type == STM32_FLASH_TYPE_C0) ||
      (sl->flash_type == STM32_FLASH_TYPE_F0_F1_F3) ||
      (sl->flash_type == STM32_FLASH_TYPE_F1_XL) ||
      (sl->flash_type == STM32_FLASH_TYPE_F2_F4) ||
      (sl->flash_type == STM32_FLASH_TYPE_F7) ||
      (sl->flash_type == STM32_FLASH_TYPE_G0) ||
      (sl->flash_type == STM32_FLASH_TYPE_G4) ||
      (sl->flash_type == STM32_FLASH_TYPE_H7) ||
      (sl->flash_type == STM32_FLASH_TYPE_L4) ||
      (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) ||
      (sl->flash_type == STM32_FLASH_TYPE_WB_WL)) {

    clear_flash_cr_pg(sl, BANK_1);
    if ((sl->flash_type == STM32_FLASH_TYPE_H7 && sl->chip_flags & CHIP_F_HAS_DUAL_BANK) ||
        sl->flash_type == STM32_FLASH_TYPE_F1_XL) {
      clear_flash_cr_pg(sl, BANK_2);
    }
    lock_flash(sl);
  } else if (sl->flash_type == STM32_FLASH_TYPE_L0_L1) {
    uint32_t val;
    uint32_t flash_regs_base = get_stm32l0_flash_base(sl);

    // reset lock bits
    stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
    val |= (1 << 0) | (1 << 1) | (1 << 2);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
  }

  // enable interrupt
  if (!stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr)) {
    stlink_write_debug32(sl, STLINK_REG_DHCSR, STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_DEBUGEN |
                                               (dhcsr & (~STLINK_REG_DHCSR_C_MASKINTS)));
  }

  // restore DMA state
  set_dma_state(sl, fl, 1);

  return (0);
}
