#include <stdio.h>
#include <string.h>
#include <stlink.h>
#include "common_flash.h"

#define L1_WRITE_BLOCK_SIZE 0x80
#define L0_WRITE_BLOCK_SIZE 0x40

int stm32l1_write_half_pages(stlink_t *sl, stm32_addr_t addr, uint8_t *base,
                             uint32_t len, uint32_t pagesize) {
  unsigned int count, off;
  unsigned int num_half_pages = len / pagesize;
  uint32_t val;
  uint32_t flash_regs_base = get_stm32l0_flash_base(sl);
  flash_loader_t fl;
  bool use_loader = true;
  int ret = 0;

  // enable half page write
  stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
  val |= (1 << FLASH_L1_FPRG);
  stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
  val |= (1 << FLASH_L1_PROG);
  stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);

  wait_flash_busy(sl);

  for (count = 0; count < num_half_pages; count++) {
    if (use_loader) {
      ret = stlink_flash_loader_run(sl, &fl, addr + count * pagesize,
                                base + count * pagesize, pagesize);
      if (ret && count == 0) {
        /* It seems that stm32lx devices have a problem when it is blank */
        WLOG("Failed to use flash loader, fallback to soft write\n");
        use_loader = false;
      }
    }
    if (!use_loader) {
      ret = 0;
      for (off = 0; off < pagesize && !ret; off += 64) {
        size_t chunk = (pagesize - off > 64) ? 64 : pagesize - off;
        memcpy(sl->q_buf, base + count * pagesize + off, chunk);
        ret = stlink_write_mem32(sl, addr + count * pagesize + off, (uint16_t)chunk);
      }
    }

    if (ret) {
      WLOG("l1_stlink_flash_loader_run(%#x) failed! == -1\n",
           addr + count * pagesize);
      break;
    }

    if (sl->verbose >= 1) {
      // show progress; writing procedure is slow and previous errors are
      // misleading
      fprintf(stdout, "\r%3u/%u halfpages written", count + 1, num_half_pages);
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

static void set_flash_cr_pg(stlink_t *sl, unsigned bank) {
  uint32_t cr_reg, x;

  x = read_flash_cr(sl, bank);

  if (sl->flash_type == STM32_FLASH_TYPE_F2_F4) {
    cr_reg = FLASH_F4_CR;
    x |= 1 << FLASH_CR_PG;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F7) {
    cr_reg = FLASH_F7_CR;
    x |= 1 << FLASH_CR_PG;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L4_L4P) {
    cr_reg = STM32L4_FLASH_CR;
    x &= ~STM32L4_FLASH_CR_OPBITS;
    x |= (1 << STM32L4_FLASH_CR_PG);
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    cr_reg = STM32Gx_FLASH_CR;
    x |= (1 << FLASH_CR_PG);
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
    cr_reg = STM32WB_FLASH_CR;
    x |= (1 << FLASH_CR_PG);
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    cr_reg = (bank == BANK_1) ? FLASH_H7_CR1 : FLASH_H7_CR2;
    x |= (1 << FLASH_H7_CR_PG);
  } else {
    cr_reg = FLASH_CR;
    x = (1 << FLASH_CR_PG);
  }

  stlink_write_debug32(sl, cr_reg, x);
}

static void set_dma_state(stlink_t *sl, flash_loader_t *fl, int bckpRstr) {
  uint32_t rcc, rcc_dma_mask, value;

  rcc = rcc_dma_mask = value = 0;

  switch (sl->flash_type) {
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
  case STM32_FLASH_TYPE_L4_L4P:
    rcc = STM32G4_RCC_AHB1ENR;
    rcc_dma_mask = STM32G4_RCC_DMAEN;
    break;
  case STM32_FLASH_TYPE_L0_L1:
    rcc = STM32L0_RCC_AHBENR;
    rcc_dma_mask = STM32L0_RCC_DMAEN;
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

int stlink_flashloader_start(stlink_t *sl, flash_loader_t *fl) {
  // disable DMA
  set_dma_state(sl, fl, 0);

  // wait for ongoing op to finish
  wait_flash_busy(sl);
  // Clear errors
  clear_flash_error(sl);

  if ((sl->flash_type == STM32_FLASH_TYPE_F2_F4) ||
      (sl->flash_type == STM32_FLASH_TYPE_F7) ||
      (sl->flash_type == STM32_FLASH_TYPE_L4_L4P)) {
    ILOG("Starting Flash write for F2/F4/F7/L4\n");

    // Flash loader initialisation
    if (stlink_flash_loader_init(sl, fl) == -1) {
      ELOG("stlink_flash_loader_init() == -1\n");
      return (-1);
    }

    unlock_flash_if(sl); // first unlock the cr

    int voltage;
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

    if (sl->flash_type == STM32_FLASH_TYPE_L4_L4P) {
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
        ILOG("Target voltage (%d mV) too low for 32-bit flash, "
             "using 8-bit flash writes\n",
             voltage);
        write_flash_cr_psiz(sl, 0, BANK_1);
      }
    }

    // set programming mode
    set_flash_cr_pg(sl, BANK_1);
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL ||
             sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    ILOG("Starting Flash write for WB/G0/G4\n");

    unlock_flash_if(sl);         // unlock flash if necessary
    set_flash_cr_pg(sl, BANK_1); // set PG 'allow programming' bit
  } else if (sl->flash_type == STM32_FLASH_TYPE_L0_L1) {
    ILOG("Starting Flash write for L0\n");

    uint32_t val;
    uint32_t flash_regs_base = get_stm32l0_flash_base(sl);

    // disable pecr protection
    stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF,
                         FLASH_L0_PEKEY1);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF,
                         FLASH_L0_PEKEY2);

    // check pecr.pelock is cleared
    stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
    if (val & (1 << 0)) {
      ELOG("pecr.pelock not clear\n");
      return (-1);
    }

    // unlock program memory
    stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF,
                         FLASH_L0_PRGKEY1);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF,
                         FLASH_L0_PRGKEY2);

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
      write_flash_cr_psiz(sl, 3 /*64it*/, BANK_1);
      if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
        write_flash_cr_psiz(sl, 3 /*64bit*/, BANK_2);
      }
    }
  } else {
    ELOG("unknown coreid, not sure how to write: %x\n", sl->core_id);
    return (-1);
  }

  return (0);
}

int stlink_flashloader_write(stlink_t *sl, flash_loader_t *fl,
                             stm32_addr_t addr, uint8_t *base, uint32_t len) {
  size_t off;
  if ((sl->flash_type == STM32_FLASH_TYPE_F2_F4) ||
      (sl->flash_type == STM32_FLASH_TYPE_F7) ||
      (sl->flash_type == STM32_FLASH_TYPE_L4_L4P)) {
    size_t buf_size = (sl->sram_size > 0x8000) ? 0x8000 : 0x4000;
    for (off = 0; off < len;) {
      size_t size = len - off > buf_size ? buf_size : len - off;
      if (stlink_flash_loader_run(sl, fl, addr + (uint32_t)off, base + off,
                                  size) == -1) {
        ELOG("stlink_flash_loader_run(%#x) failed! == -1\n",
             (unsigned)(addr + off));
        check_flash_error(sl);
        return (-1);
      }

      off += size;
    }
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL ||
             sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    DLOG("Starting %3u page write\r\n", (unsigned int)(len / sl->flash_pgsz));
    for (off = 0; off < len; off += sizeof(uint32_t)) {
      uint32_t data;

      if ((off % sl->flash_pgsz) > (sl->flash_pgsz - 5)) {
        fprintf(stdout, "\r%3u/%3u pages written",
                (unsigned int)(off / sl->flash_pgsz),
                (unsigned int)(len / sl->flash_pgsz));
        fflush(stdout);
      }

      write_uint32((unsigned char *)&data, *(uint32_t *)(base + off));
      stlink_write_debug32(sl, addr + (uint32_t)off, data);
      wait_flash_busy(sl); // wait for 'busy' bit in FLASH_SR to clear
    }
    fprintf(stdout, "\n");

    // flash writes happen as 2 words at a time
    if ((off / sizeof(uint32_t)) % 2 != 0) {
      stlink_write_debug32(sl, addr + (uint32_t)off,
                           0); // write a single word of zeros
      wait_flash_busy(sl);     // wait for 'busy' bit in FLASH_SR to clear
    }
  } else if (sl->flash_type == STM32_FLASH_TYPE_L0_L1) {
    uint32_t val;
    uint32_t flash_regs_base = get_stm32l0_flash_base(sl);
    uint32_t pagesize = (flash_regs_base==STM32L0_FLASH_REGS_ADDR)?
                                L0_WRITE_BLOCK_SIZE:L1_WRITE_BLOCK_SIZE;

    DLOG("Starting %3u page write\r\n", (unsigned int)(len / sl->flash_pgsz));

    off = 0;

    if (len > pagesize) {
      if (stm32l1_write_half_pages(sl, addr, base, len, pagesize)) {
        return (-1);
      } else {
        off = (size_t)(len / pagesize) * pagesize;
      }
    }

    // write remaining word in program memory
    for (; off < len; off += sizeof(uint32_t)) {
      uint32_t data;

      if ((off % sl->flash_pgsz) > (sl->flash_pgsz - 5)) {
        fprintf(stdout, "\r%3u/%3u pages written",
                (unsigned int)(off / sl->flash_pgsz),
                (unsigned int)(len / sl->flash_pgsz));
        fflush(stdout);
      }

      write_uint32((unsigned char *)&data, *(uint32_t *)(base + off));
      stlink_write_debug32(sl, addr + (uint32_t)off, data);

      // wait for sr.busy to be cleared
      do {
        stlink_read_debug32(sl, flash_regs_base + FLASH_SR_OFF, &val);
      } while ((val & (1 << 0)) != 0);

      // TODO: check redo write operation
    }
    fprintf(stdout, "\n");
  } else if ((sl->flash_type == STM32_FLASH_TYPE_F0_F1_F3) ||
             (sl->flash_type == STM32_FLASH_TYPE_F1_XL)) {
    int write_block_count = 0;
    for (off = 0; off < len; off += sl->flash_pgsz) {
      // adjust last write size
      size_t size = len - off > sl->flash_pgsz ? sl->flash_pgsz : len - off;

      // unlock and set programming mode
      unlock_flash_if(sl);

      DLOG("Finished unlocking flash, running loader!\n");

      if (stlink_flash_loader_run(sl, fl, addr + (uint32_t)off, base + off,
                                  size) == -1) {
        ELOG("stlink_flash_loader_run(%#x) failed! == -1\n",
             (unsigned)(addr + off));
        check_flash_error(sl);
        return (-1);
      }

      lock_flash(sl);

      if (sl->verbose >= 1) {
        // show progress; writing procedure is slow and previous errors are
        // misleading
        fprintf(stdout, "\r%3u/%3u pages written", ++write_block_count,
                (unsigned int)((len + sl->flash_pgsz - 1) / sl->flash_pgsz));
        fflush(stdout);
      }
    }
    if (sl->verbose >= 1) {
      fprintf(stdout, "\n");
    }
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    for (off = 0; off < len;) {
      // Program STM32H7x with 64-byte Flash words
      size_t chunk = (len - off > 64) ? 64 : len - off;
      memcpy(sl->q_buf, base + off, chunk);
      stlink_write_mem32(sl, addr + (uint32_t)off, 64);
      wait_flash_busy(sl);

      off += chunk;

      if (sl->verbose >= 1) {
        // show progress
        fprintf(stdout, "\r%u/%u bytes written", (unsigned int)off,
                (unsigned int)len);
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

int stlink_flashloader_stop(stlink_t *sl, flash_loader_t *fl) {
  uint32_t dhcsr;

  if ((sl->flash_type == STM32_FLASH_TYPE_F0_F1_F3) ||
      (sl->flash_type == STM32_FLASH_TYPE_F1_XL) ||
      (sl->flash_type == STM32_FLASH_TYPE_F2_F4) ||
      (sl->flash_type == STM32_FLASH_TYPE_F7) ||
      (sl->flash_type == STM32_FLASH_TYPE_L4_L4P) ||
      (sl->flash_type == STM32_FLASH_TYPE_WB_WL) ||
      (sl->flash_type == STM32_FLASH_TYPE_G0) ||
      (sl->flash_type == STM32_FLASH_TYPE_G4) ||
      (sl->flash_type == STM32_FLASH_TYPE_H7)) {

    clear_flash_cr_pg(sl, BANK_1);
    if ((sl->flash_type == STM32_FLASH_TYPE_H7 &&
        sl->chip_flags & CHIP_F_HAS_DUAL_BANK) ||
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
    stlink_write_debug32(sl, STLINK_REG_DHCSR,
                         STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_DEBUGEN |
                             (dhcsr & (~STLINK_REG_DHCSR_C_MASKINTS)));
  }

  // restore DMA state
  set_dma_state(sl, fl, 1);

  return (0);
}
