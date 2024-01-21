/*
 * File: common_flash.c
 *
 * Flash operations
 */

#include <stdint.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <stlink.h>
#include "common_flash.h"

#include "calculate.h"
#include "flash_loader.h"
#include "logging.h"
#include "map_file.h"
#include "md5.h"
#include "read_write.h"

#define DEBUG_FLASH 0

uint32_t get_stm32l0_flash_base(stlink_t *sl) {
  switch (sl->chip_id) {
  case STM32_CHIPID_L0_CAT1:
  case STM32_CHIPID_L0_CAT2:
  case STM32_CHIPID_L0_CAT3:
  case STM32_CHIPID_L0_CAT5:
    return (FLASH_L0_REGS_ADDR);

  case STM32_CHIPID_L1_CAT2:
  case STM32_CHIPID_L1_MD:
  case STM32_CHIPID_L1_MD_PLUS:
  case STM32_CHIPID_L1_MD_PLUS_HD:
  case STM32_CHIPID_L152_RE:
    return (FLASH_Lx_REGS_ADDR);

  default:
    WLOG("Flash base use default L0 address\n");
    return (FLASH_L0_REGS_ADDR);
  }
}

uint32_t read_flash_cr(stlink_t *sl, uint32_t bank) {
  uint32_t reg, res;

  if (sl->flash_type == STM32_FLASH_TYPE_C0) {
    reg = FLASH_C0_CR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F2_F4) {
    reg = FLASH_F4_CR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F7) {
    reg = FLASH_F7_CR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    reg = FLASH_Gx_CR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    reg = (bank == BANK_1) ? FLASH_H7_CR1 : FLASH_H7_CR2;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L4) {
    reg = FLASH_L4_CR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) {
    reg = FLASH_L5_NSCR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
    reg = FLASH_WB_CR;
  } else {
    reg = (bank == BANK_1) ? FLASH_CR : FLASH_CR2;
  }

  stlink_read_debug32(sl, reg, &res);

#if DEBUG_FLASH
  fprintf(stdout, "CR:0x%x\n", res);
#endif
  return (res);
}

void lock_flash(stlink_t *sl) {
  uint32_t cr_lock_shift = 0, cr_reg = 0, n = 0, cr2_reg = 0;
  uint32_t cr_mask = 0xffffffffu;

  if (sl->flash_type == STM32_FLASH_TYPE_C0) {
    cr_reg = FLASH_C0_CR;
    cr_lock_shift = FLASH_C0_CR_LOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F0_F1_F3) {
    cr_reg = FLASH_CR;
    cr_lock_shift = FLASH_CR_LOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F1_XL) {
    cr_reg = FLASH_CR;
    cr2_reg = FLASH_CR2;
    cr_lock_shift = FLASH_CR_LOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F2_F4) {
    cr_reg = FLASH_F4_CR;
    cr_lock_shift = FLASH_F4_CR_LOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F7) {
    cr_reg = FLASH_F7_CR;
    cr_lock_shift = FLASH_F7_CR_LOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    cr_reg = FLASH_Gx_CR;
    cr_lock_shift = FLASH_Gx_CR_LOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    cr_reg = FLASH_H7_CR1;
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
      cr2_reg = FLASH_H7_CR2;
    }
    cr_lock_shift = FLASH_H7_CR_LOCK;
    cr_mask = ~(1u << FLASH_H7_CR_SER);
  } else if (sl->flash_type == STM32_FLASH_TYPE_L0_L1) {
    cr_reg = get_stm32l0_flash_base(sl) + FLASH_PECR_OFF;
    cr_lock_shift = FLASH_L0_PELOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L4) {
    cr_reg = FLASH_L4_CR;
    cr_lock_shift = FLASH_L4_CR_LOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) {
    cr_reg = FLASH_L5_NSCR;
    cr_lock_shift = FLASH_L5_NSCR_NSLOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
    cr_reg = FLASH_WB_CR;
    cr_lock_shift = FLASH_WB_CR_LOCK;
  } else {
    ELOG("unsupported flash method, abort\n");
    return;
  }

  stlink_read_debug32(sl, cr_reg, &n);
  n &= cr_mask;
  n |= (1u << cr_lock_shift);
  stlink_write_debug32(sl, cr_reg, n);

  if (cr2_reg) {
    n = read_flash_cr(sl, BANK_2) | (1u << cr_lock_shift);
    stlink_write_debug32(sl, cr2_reg, n);
  }
}

static inline int32_t write_flash_sr(stlink_t *sl, uint32_t bank, uint32_t val) {
  uint32_t sr_reg;

  if (sl->flash_type == STM32_FLASH_TYPE_C0) {
    sr_reg = FLASH_C0_SR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F0_F1_F3 ||
             sl->flash_type == STM32_FLASH_TYPE_F1_XL) {
    sr_reg = (bank == BANK_1) ? FLASH_SR : FLASH_SR2;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F2_F4) {
    sr_reg = FLASH_F4_SR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F7) {
    sr_reg = FLASH_F7_SR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    sr_reg = FLASH_Gx_SR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    sr_reg = (bank == BANK_1) ? FLASH_H7_SR1 : FLASH_H7_SR2;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L0_L1) {
    sr_reg = get_stm32l0_flash_base(sl) + FLASH_SR_OFF;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L4) {
    sr_reg = FLASH_L4_SR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) {
    sr_reg = FLASH_L5_NSSR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
    sr_reg = FLASH_WB_SR;
  } else {
    ELOG("method 'write_flash_sr' is unsupported\n");
    return (-1);
  }

  return stlink_write_debug32(sl, sr_reg, val);
}

void clear_flash_error(stlink_t *sl) {
  switch (sl->flash_type) {
  case STM32_FLASH_TYPE_C0:
    write_flash_sr(sl, BANK_1, FLASH_C0_SR_ERROR_MASK);
    break;
  case STM32_FLASH_TYPE_F0_F1_F3:
    write_flash_sr(sl, BANK_1, FLASH_SR_ERROR_MASK);
    break;
  case STM32_FLASH_TYPE_F2_F4:
    write_flash_sr(sl, BANK_1, FLASH_F4_SR_ERROR_MASK);
    break;
  case STM32_FLASH_TYPE_F7:
    write_flash_sr(sl, BANK_1, FLASH_F7_SR_ERROR_MASK);
    break;
  case STM32_FLASH_TYPE_G0:
  case STM32_FLASH_TYPE_G4:
    write_flash_sr(sl, BANK_1, FLASH_Gx_SR_ERROR_MASK);
    break;
  case STM32_FLASH_TYPE_H7:
    write_flash_sr(sl, BANK_1, FLASH_H7_SR_ERROR_MASK);
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
      write_flash_sr(sl, BANK_2, FLASH_H7_SR_ERROR_MASK);
    }
    break;
  case STM32_FLASH_TYPE_L0_L1:
    if (get_stm32l0_flash_base(sl) == FLASH_Lx_REGS_ADDR) {
      write_flash_sr(sl, BANK_1, FLASH_L1_SR_ERROR_MASK);
    } else {
      write_flash_sr(sl, BANK_1, FLASH_L0_SR_ERROR_MASK);
    }
    break;
  case STM32_FLASH_TYPE_L4:
    write_flash_sr(sl, BANK_1, FLASH_L4_SR_ERROR_MASK);
    break;
  case STM32_FLASH_TYPE_L5_U5_H5:
    write_flash_sr(sl, BANK_1, FLASH_L5_NSSR_ERROR_MASK);
    break;
  case STM32_FLASH_TYPE_WB_WL:
    write_flash_sr(sl, BANK_1, FLASH_WB_SR_ERROR_MASK);
    break;
  default:
    break;
  }
}

uint32_t read_flash_sr(stlink_t *sl, uint32_t bank) {
  uint32_t res, sr_reg;

  if (sl->flash_type == STM32_FLASH_TYPE_C0) {
    sr_reg = FLASH_C0_SR;
  } else if ((sl->flash_type == STM32_FLASH_TYPE_F0_F1_F3) ||
             (sl->flash_type == STM32_FLASH_TYPE_F1_XL)) {
    sr_reg = (bank == BANK_1) ? FLASH_SR : FLASH_SR2;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F2_F4) {
    sr_reg = FLASH_F4_SR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F7) {
    sr_reg = FLASH_F7_SR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    sr_reg = FLASH_Gx_SR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    sr_reg = (bank == BANK_1) ? FLASH_H7_SR1 : FLASH_H7_SR2;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L0_L1) {
    sr_reg = get_stm32l0_flash_base(sl) + FLASH_SR_OFF;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L4) {
    sr_reg = FLASH_L4_SR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) {
    sr_reg = FLASH_L5_NSSR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
    sr_reg = FLASH_WB_SR;
  } else {
    ELOG("method 'read_flash_sr' is unsupported\n");
    return (-1);
  }

  stlink_read_debug32(sl, sr_reg, &res);
  return (res);
}

uint32_t is_flash_busy(stlink_t *sl) {
  uint32_t sr_busy_shift;
  uint32_t res;

  if (sl->flash_type == STM32_FLASH_TYPE_C0) {
    sr_busy_shift = FLASH_C0_SR_BSY;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F0_F1_F3 ||
             sl->flash_type == STM32_FLASH_TYPE_F1_XL ||
             sl->flash_type == STM32_FLASH_TYPE_L0_L1) {
    sr_busy_shift = FLASH_SR_BSY;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F2_F4) {
    sr_busy_shift = FLASH_F4_SR_BSY;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F7) {
    sr_busy_shift = FLASH_F7_SR_BSY;
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    sr_busy_shift = FLASH_Gx_SR_BSY;
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    sr_busy_shift = FLASH_H7_SR_QW;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L4) {
    sr_busy_shift = FLASH_L4_SR_BSY;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) {
    sr_busy_shift = FLASH_L5_NSSR_BSY;
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
    sr_busy_shift = FLASH_WB_SR_BSY;
  } else {
    ELOG("method 'is_flash_busy' is unsupported\n");
    return (-1);
  }

  res = read_flash_sr(sl, BANK_1) & (1 << sr_busy_shift);

  if (sl->flash_type == STM32_FLASH_TYPE_F1_XL ||
      (sl->flash_type == STM32_FLASH_TYPE_H7 &&
       sl->chip_flags & CHIP_F_HAS_DUAL_BANK)) {
    res |= read_flash_sr(sl, BANK_2) & (1 << sr_busy_shift);
  }

  return (res);
}

void wait_flash_busy(stlink_t *sl) {
  // TODO: add some delays here
  while (is_flash_busy(sl))
    ;
}

int32_t check_flash_error(stlink_t *sl) {
  uint32_t res = 0;
  uint32_t WRPERR, PROGERR, PGAERR;

  WRPERR = PROGERR = PGAERR = 0;

  switch (sl->flash_type) {
  case STM32_FLASH_TYPE_C0:
    res = read_flash_sr(sl, BANK_1) & FLASH_C0_SR_ERROR_MASK;
    WRPERR = (1 << FLASH_C0_SR_WRPERR);
    PROGERR = (1 << FLASH_C0_SR_PROGERR);
    PGAERR = (1 << FLASH_C0_SR_PGAERR);
    break;
  case STM32_FLASH_TYPE_F0_F1_F3:
  case STM32_FLASH_TYPE_F1_XL:
    res = read_flash_sr(sl, BANK_1) & FLASH_SR_ERROR_MASK;
    if (sl->flash_type == STM32_FLASH_TYPE_F1_XL) {
      res |= read_flash_sr(sl, BANK_2) & FLASH_SR_ERROR_MASK;
    }
    WRPERR = (1 << FLASH_SR_WRPRT_ERR);
    PROGERR = (1 << FLASH_SR_PG_ERR);
    break;
  case STM32_FLASH_TYPE_F2_F4:
    res = read_flash_sr(sl, BANK_1) & FLASH_F4_SR_ERROR_MASK;
    WRPERR = (1 << FLASH_F4_SR_WRPERR);
    PGAERR = (1 << FLASH_F4_SR_PGAERR);
    break;
  case STM32_FLASH_TYPE_F7:
    res = read_flash_sr(sl, BANK_1) & FLASH_F7_SR_ERROR_MASK;
    WRPERR = (1 << FLASH_F7_SR_WRP_ERR);
    PROGERR = (1 << FLASH_F7_SR_PGP_ERR);
    break;
  case STM32_FLASH_TYPE_G0:
  case STM32_FLASH_TYPE_G4:
    res = read_flash_sr(sl, BANK_1) & FLASH_Gx_SR_ERROR_MASK;
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
      res |= read_flash_sr(sl, BANK_2) & FLASH_Gx_SR_ERROR_MASK;
    }
    WRPERR = (1 << FLASH_Gx_SR_WRPERR);
    PROGERR = (1 << FLASH_Gx_SR_PROGERR);
    PGAERR = (1 << FLASH_Gx_SR_PGAERR);
    break;
  case STM32_FLASH_TYPE_H7:
    res = read_flash_sr(sl, BANK_1) & FLASH_H7_SR_ERROR_MASK;
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
      res |= read_flash_sr(sl, BANK_2) & FLASH_H7_SR_ERROR_MASK;
    }
    WRPERR = (1 << FLASH_H7_SR_WRPERR);
    break;
  case STM32_FLASH_TYPE_L0_L1:
    res = read_flash_sr(sl, BANK_1);
    if (get_stm32l0_flash_base(sl) == FLASH_Lx_REGS_ADDR) {
      res &= FLASH_L1_SR_ERROR_MASK;
    } else {
      res &= FLASH_L0_SR_ERROR_MASK;
      PROGERR = (1 << FLASH_L0_SR_NOTZEROERR);
    }
    WRPERR = (1 << FLASH_L0_SR_WRPERR);
    PGAERR = (1 << FLASH_L0_SR_PGAERR);
    break;
  case STM32_FLASH_TYPE_L4:
    res = read_flash_sr(sl, BANK_1) & FLASH_L4_SR_ERROR_MASK;
    WRPERR = (1 << FLASH_L4_SR_WRPERR);
    PROGERR = (1 << FLASH_L4_SR_PROGERR);
    PGAERR = (1 << FLASH_L4_SR_PGAERR);
    break;
  case STM32_FLASH_TYPE_L5_U5_H5:
    res = read_flash_sr(sl, BANK_1) & FLASH_L5_NSSR_ERROR_MASK;
    WRPERR = (1 << FLASH_L5_NSSR_NSWRPERR);
    PROGERR = (1 << FLASH_L5_NSSR_NSPROGERR);
    PGAERR = (1 << FLASH_L5_NSSR_NSPGAERR);
    break;
  case STM32_FLASH_TYPE_WB_WL:
    res = read_flash_sr(sl, BANK_1) & FLASH_WB_SR_ERROR_MASK;
    WRPERR = (1 << FLASH_WB_SR_WRPERR);
    PROGERR = (1 << FLASH_WB_SR_PROGERR);
    PGAERR = (1 << FLASH_WB_SR_PGAERR);
    break;
  default:
    break;
  }

  if (res) {
    if (WRPERR && (WRPERR & res) == WRPERR) {
      ELOG("Flash memory is write protected\n");
      res &= ~WRPERR;
    } else if (PROGERR && (PROGERR & res) == PROGERR) {
      ELOG("Flash memory contains a non-erased value\n");
      res &= ~PROGERR;
    } else if (PGAERR && (PGAERR & res) == PGAERR) {
      ELOG("Invalid flash address\n");
      res &= ~PGAERR;
    }

    if (res) ELOG("Flash programming error: %#010x\n", res);
    return (-1);
  }

  return (0);
}

static inline uint32_t is_flash_locked(stlink_t *sl) {
  /* return non zero for true */
  uint32_t cr_lock_shift;
  uint32_t cr_reg;
  uint32_t n;

  if (sl->flash_type == STM32_FLASH_TYPE_C0) {
    cr_reg = FLASH_C0_CR;
    cr_lock_shift = FLASH_C0_CR_LOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F0_F1_F3 ||
             sl->flash_type == STM32_FLASH_TYPE_F1_XL) {
    cr_reg = FLASH_CR;
    cr_lock_shift = FLASH_CR_LOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F2_F4) {
    cr_reg = FLASH_F4_CR;
    cr_lock_shift = FLASH_F4_CR_LOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F7) {
    cr_reg = FLASH_F7_CR;
    cr_lock_shift = FLASH_F7_CR_LOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    cr_reg = FLASH_Gx_CR;
    cr_lock_shift = FLASH_Gx_CR_LOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    cr_reg = FLASH_H7_CR1;
    cr_lock_shift = FLASH_H7_CR_LOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L0_L1) {
    cr_reg = get_stm32l0_flash_base(sl) + FLASH_PECR_OFF;
    cr_lock_shift = FLASH_L0_PELOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L4) {
    cr_reg = FLASH_L4_CR;
    cr_lock_shift = FLASH_L4_CR_LOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) {
    cr_reg = FLASH_L5_NSCR;
    cr_lock_shift = FLASH_L5_NSCR_NSLOCK;
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
    cr_reg = FLASH_WB_CR;
    cr_lock_shift = FLASH_WB_CR_LOCK;
  } else {
    ELOG("unsupported flash method, abort\n");
    return (-1);
  }

  stlink_read_debug32(sl, cr_reg, &n);
  return (n & (1u << cr_lock_shift));
}

static void unlock_flash(stlink_t *sl) {
  uint32_t key_reg, key2_reg = 0;
  uint32_t flash_key1 = FLASH_KEY1;
  uint32_t flash_key2 = FLASH_KEY2;
  /* The unlock sequence consists of 2 write cycles where 2 key values are
   * written to the FLASH_KEYR register. An invalid sequence results in a
   * definitive lock of the FPEC block until next reset.
   */

  if (sl->flash_type == STM32_FLASH_TYPE_C0) {
    key_reg = FLASH_C0_KEYR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F0_F1_F3) {
    key_reg = FLASH_KEYR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F1_XL) {
    key_reg = FLASH_KEYR;
    key2_reg = FLASH_KEYR2;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F2_F4) {
    key_reg = FLASH_F4_KEYR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F7) {
    key_reg = FLASH_F7_KEYR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    key_reg = FLASH_Gx_KEYR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    key_reg = FLASH_H7_KEYR1;
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
      key2_reg = FLASH_H7_KEYR2;
    }
  } else if (sl->flash_type == STM32_FLASH_TYPE_L0_L1) {
    key_reg = get_stm32l0_flash_base(sl) + FLASH_PEKEYR_OFF;
    flash_key1 = FLASH_L0_PEKEY1;
    flash_key2 = FLASH_L0_PEKEY2;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L4) {
    key_reg = FLASH_L4_KEYR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) {
    // Set voltage scaling to range 0 to perform flash operations (RM0438 p. 183)
    uint32_t mask = (0b11 << STM32L5_PWR_CR1_VOS);
    uint32_t val;
    if (!stlink_read_debug32(sl, STM32L5_PWR_CR1, &val) && (val & mask) > (1 << STM32L5_PWR_CR1_VOS)) {
      val &= ~mask;
      stlink_write_debug32(sl, STM32L5_PWR_CR1, val);
    }
    key_reg = FLASH_L5_NSKEYR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
    key_reg = FLASH_WB_KEYR;
  } else {
    ELOG("unsupported flash method, abort\n");
    return;
  }

  stlink_write_debug32(sl, key_reg, flash_key1);
  stlink_write_debug32(sl, key_reg, flash_key2);

  if (key2_reg) {
    stlink_write_debug32(sl, key2_reg, flash_key1);
    stlink_write_debug32(sl, key2_reg, flash_key2);
  }
}

/* unlock flash if already locked */
int32_t unlock_flash_if(stlink_t *sl) {
  if (is_flash_locked(sl)) {
    unlock_flash(sl);

    if (is_flash_locked(sl)) {
      WLOG("Failed to unlock flash!\n");
      return (-1);
    }
  }

  DLOG("Successfully unlocked flash\n");
  return (0);
}

int32_t lock_flash_option(stlink_t *sl) {
  uint32_t optlock_shift, optcr_reg, n, optcr2_reg = 0;
  int32_t active_bit_level = 1;

  switch (sl->flash_type) {
  case STM32_FLASH_TYPE_C0:
    optcr_reg = FLASH_C0_CR;
    optlock_shift = FLASH_C0_CR_OPTLOCK;
    break;
  case STM32_FLASH_TYPE_F0_F1_F3:
  case STM32_FLASH_TYPE_F1_XL:
    optcr_reg = FLASH_CR;
    optlock_shift = FLASH_CR_OPTWRE;
    active_bit_level = 0;
    break;
  case STM32_FLASH_TYPE_F2_F4:
    optcr_reg = FLASH_F4_OPTCR;
    optlock_shift = FLASH_F4_OPTCR_LOCK;
    break;
  case STM32_FLASH_TYPE_F7:
    optcr_reg = FLASH_F7_OPTCR;
    optlock_shift = FLASH_F7_OPTCR_LOCK;
    break;
  case STM32_FLASH_TYPE_G0:
  case STM32_FLASH_TYPE_G4:
    optcr_reg = FLASH_Gx_CR;
    optlock_shift = FLASH_Gx_CR_OPTLOCK;
    break;
  case STM32_FLASH_TYPE_H7:
    optcr_reg = FLASH_H7_OPTCR;
    optlock_shift = FLASH_H7_OPTCR_OPTLOCK;
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK)
      optcr2_reg = FLASH_H7_OPTCR2;
    break;
  case STM32_FLASH_TYPE_L0_L1:
    optcr_reg = get_stm32l0_flash_base(sl) + FLASH_PECR_OFF;
    optlock_shift = FLASH_L0_OPTLOCK;
    break;
  case STM32_FLASH_TYPE_L4:
    optcr_reg = FLASH_L4_CR;
    optlock_shift = FLASH_L4_CR_OPTLOCK;
    break;
  case STM32_FLASH_TYPE_L5_U5_H5:
    optcr_reg = FLASH_L5_NSCR;
    optlock_shift = FLASH_L5_NSCR_OPTLOCK;
    break;
  case STM32_FLASH_TYPE_WB_WL:
    optcr_reg = FLASH_WB_CR;
    optlock_shift = FLASH_WB_CR_OPTLOCK;
    break;
  default:
    ELOG("unsupported flash method, abort\n");
    return -1;
  }

  stlink_read_debug32(sl, optcr_reg, &n);

  if (active_bit_level == 0) {
    n &= ~(1u << optlock_shift);
  } else {
    n |= (1u << optlock_shift);
  }

  stlink_write_debug32(sl, optcr_reg, n);

  if (optcr2_reg) {
    stlink_read_debug32(sl, optcr2_reg, &n);

    if (active_bit_level == 0) {
      n &= ~(1u << optlock_shift);
    } else {
      n |= (1u << optlock_shift);
    }

    stlink_write_debug32(sl, optcr2_reg, n);
  }

  return (0);
}

static bool is_flash_option_locked(stlink_t *sl) {
  uint32_t optlock_shift, optcr_reg;
  int32_t active_bit_level = 1;
  uint32_t n;

  switch (sl->flash_type) {
  case STM32_FLASH_TYPE_C0:
    optcr_reg = FLASH_C0_CR;
    optlock_shift = FLASH_C0_CR_OPTLOCK;
    break;
  case STM32_FLASH_TYPE_F0_F1_F3:
  case STM32_FLASH_TYPE_F1_XL:
    optcr_reg = FLASH_CR;
    optlock_shift = FLASH_CR_OPTWRE;
    active_bit_level = 0; /* bit is "option write enable", not lock */
    break;
  case STM32_FLASH_TYPE_F2_F4:
    optcr_reg = FLASH_F4_OPTCR;
    optlock_shift = FLASH_F4_OPTCR_LOCK;
    break;
  case STM32_FLASH_TYPE_F7:
    optcr_reg = FLASH_F7_OPTCR;
    optlock_shift = FLASH_F7_OPTCR_LOCK;
    break;
  case STM32_FLASH_TYPE_G0:
  case STM32_FLASH_TYPE_G4:
    optcr_reg = FLASH_Gx_CR;
    optlock_shift = FLASH_Gx_CR_OPTLOCK;
    break;
  case STM32_FLASH_TYPE_H7:
    optcr_reg = FLASH_H7_OPTCR;
    optlock_shift = FLASH_H7_OPTCR_OPTLOCK;
    break;
  case STM32_FLASH_TYPE_L0_L1:
    optcr_reg = get_stm32l0_flash_base(sl) + FLASH_PECR_OFF;
    optlock_shift = FLASH_L0_OPTLOCK;
    break;
  case STM32_FLASH_TYPE_L4:
    optcr_reg = FLASH_L4_CR;
    optlock_shift = FLASH_L4_CR_OPTLOCK;
    break;
  case STM32_FLASH_TYPE_L5_U5_H5:
    optcr_reg = FLASH_L5_NSCR;
    optlock_shift = FLASH_L5_NSCR_OPTLOCK;
    break;
  case STM32_FLASH_TYPE_WB_WL:
    optcr_reg = FLASH_WB_CR;
    optlock_shift = FLASH_WB_CR_OPTLOCK;
    break;
  default:
    ELOG("unsupported flash method, abort\n");
    return -1;
  }

  stlink_read_debug32(sl, optcr_reg, &n);

  if (active_bit_level == 0) {
    return (!(n & (1u << optlock_shift)));
  }

  return (n & (1u << optlock_shift));
}

static int32_t unlock_flash_option(stlink_t *sl) {
  uint32_t optkey_reg, optkey2_reg = 0;
  uint32_t optkey1 = FLASH_OPTKEY1;
  uint32_t optkey2 = FLASH_OPTKEY2;

  switch (sl->flash_type) {
  case STM32_FLASH_TYPE_C0:
    optkey_reg = FLASH_C0_OPT_KEYR;
    break;
  case STM32_FLASH_TYPE_F0_F1_F3:
  case STM32_FLASH_TYPE_F1_XL:
    optkey_reg = FLASH_OPTKEYR;
    optkey1 = FLASH_F0_OPTKEY1;
    optkey2 = FLASH_F0_OPTKEY2;
    break;
  case STM32_FLASH_TYPE_F2_F4:
    optkey_reg = FLASH_F4_OPT_KEYR;
    break;
  case STM32_FLASH_TYPE_F7:
    optkey_reg = FLASH_F7_OPT_KEYR;
    break;
  case STM32_FLASH_TYPE_G0:
  case STM32_FLASH_TYPE_G4:
    optkey_reg = FLASH_Gx_OPTKEYR;
    break;
  case STM32_FLASH_TYPE_H7:
    optkey_reg = FLASH_H7_OPT_KEYR;
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK)
      optkey2_reg = FLASH_H7_OPT_KEYR2;
    break;
  case STM32_FLASH_TYPE_L0_L1:
    optkey_reg = get_stm32l0_flash_base(sl) + FLASH_OPTKEYR_OFF;
    optkey1 = FLASH_L0_OPTKEY1;
    optkey2 = FLASH_L0_OPTKEY2;
    break;
  case STM32_FLASH_TYPE_L4:
    optkey_reg = FLASH_L4_OPTKEYR;
    break;
  case STM32_FLASH_TYPE_L5_U5_H5:
    optkey_reg = FLASH_L5_OPTKEYR;
    break;
  case STM32_FLASH_TYPE_WB_WL:
    optkey_reg = FLASH_WB_OPT_KEYR;
    break;
  default:
    ELOG("unsupported flash method, abort\n");
    return (-1);
  }

  stlink_write_debug32(sl, optkey_reg, optkey1);
  stlink_write_debug32(sl, optkey_reg, optkey2);

  if (optkey2_reg) {
    stlink_write_debug32(sl, optkey2_reg, optkey1);
    stlink_write_debug32(sl, optkey2_reg, optkey2);
  }

  return (0);
}

int32_t unlock_flash_option_if(stlink_t *sl) {
  if (is_flash_option_locked(sl)) {
    if (unlock_flash_option(sl)) {
      ELOG("Could not unlock flash option!\n");
      return (-1);
    }

    if (is_flash_option_locked(sl)) {
      ELOG("Failed to unlock flash option!\n");
      return (-1);
    }
  }

  DLOG("Successfully unlocked flash option\n");
  return (0);
}

void write_flash_cr_psiz(stlink_t *sl, uint32_t n,
                                       uint32_t bank) {
  uint32_t cr_reg, psize_shift;
  uint32_t x = read_flash_cr(sl, bank);

  if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    cr_reg = (bank == BANK_1) ? FLASH_H7_CR1 : FLASH_H7_CR2;
    psize_shift = FLASH_H7_CR_PSIZE;
  } else {
    cr_reg = FLASH_F4_CR;
    psize_shift = 8;
  }

  x &= ~(0x03 << psize_shift);
  x |= (n << psize_shift);
#if DEBUG_FLASH
  fprintf(stdout, "PSIZ:0x%x 0x%x\n", x, n);
#endif
  stlink_write_debug32(sl, cr_reg, x);
}

void clear_flash_cr_pg(stlink_t *sl, uint32_t bank) {
  uint32_t cr_reg, n;
  uint32_t bit = FLASH_CR_PG;

  if (sl->flash_type == STM32_FLASH_TYPE_C0) {
    cr_reg = FLASH_C0_CR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F2_F4) {
    cr_reg = FLASH_F4_CR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F7) {
    cr_reg = FLASH_F7_CR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    cr_reg = FLASH_Gx_CR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    cr_reg = (bank == BANK_1) ? FLASH_H7_CR1 : FLASH_H7_CR2;
    bit = FLASH_H7_CR_PG;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L4) {
    cr_reg = FLASH_L4_CR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) {
    cr_reg = FLASH_L5_NSCR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
    cr_reg = FLASH_WB_CR;
  } else {
    cr_reg = FLASH_CR;
  }

  n = read_flash_cr(sl, bank) & ~(1 << bit);
  stlink_write_debug32(sl, cr_reg, n);
}

/* ------------------------------------------------------------------------ */

static void wait_flash_busy_progress(stlink_t *sl) {
  int32_t i = 0;
  fprintf(stdout, "Mass erasing...");
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

static inline void write_flash_ar(stlink_t *sl, uint32_t n, uint32_t bank) {
  stlink_write_debug32(sl, (bank == BANK_1) ? FLASH_AR : FLASH_AR2, n);
}

static inline void write_flash_cr_snb(stlink_t *sl, uint32_t n, uint32_t bank) {
  uint32_t cr_reg, snb_mask, snb_shift, ser_shift;
  uint32_t x = read_flash_cr(sl, bank);

  if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    cr_reg = (bank == BANK_1) ? FLASH_H7_CR1 : FLASH_H7_CR2;
    snb_mask = FLASH_H7_CR_SNB_MASK;
    snb_shift = FLASH_H7_CR_SNB;
    ser_shift = FLASH_H7_CR_SER;
  } else {
    cr_reg = FLASH_F4_CR;
    snb_mask = FLASH_F4_CR_SNB_MASK;
    snb_shift = FLASH_F4_CR_SNB;
    ser_shift = FLASH_F4_CR_SER;
  }

  x &= ~snb_mask;
  x |= (n << snb_shift);
  x |= (1 << ser_shift);
#if DEBUG_FLASH
  fprintf(stdout, "SNB:0x%x 0x%x\n", x, n);
#endif
  stlink_write_debug32(sl, cr_reg, x);
}

static void set_flash_cr_per(stlink_t *sl, uint32_t bank) {
  uint32_t cr_reg, val;

  if (sl->flash_type == STM32_FLASH_TYPE_C0) {
    cr_reg = FLASH_C0_CR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    cr_reg = FLASH_Gx_CR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) {
    cr_reg = FLASH_L5_NSCR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
    cr_reg = FLASH_WB_CR;
  } else {
    cr_reg = (bank == BANK_1) ? FLASH_CR : FLASH_CR2;
  }

  stlink_read_debug32(sl, cr_reg, &val);
  val |= (1 << FLASH_CR_PER);
  stlink_write_debug32(sl, cr_reg, val);
}

static void clear_flash_cr_per(stlink_t *sl, uint32_t bank) {
  uint32_t cr_reg;

  if (sl->flash_type == STM32_FLASH_TYPE_C0) {
    cr_reg = FLASH_C0_CR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    cr_reg = FLASH_Gx_CR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) {
    cr_reg = FLASH_L5_NSCR;
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
    cr_reg = FLASH_WB_CR;
  } else {
    cr_reg = (bank == BANK_1) ? FLASH_CR : FLASH_CR2;
  }

  const uint32_t n = read_flash_cr(sl, bank) & ~(1 << FLASH_CR_PER);
  stlink_write_debug32(sl, cr_reg, n);
}

static inline void write_flash_cr_bker_pnb(stlink_t *sl, uint32_t n) {
  stlink_write_debug32(sl, FLASH_L4_SR,
                       0xFFFFFFFF & ~(1 << FLASH_L4_SR_BSY));
  uint32_t x = read_flash_cr(sl, BANK_1);
  x &= ~FLASH_L4_CR_OPBITS;
  x &= ~FLASH_L4_CR_PAGEMASK;
  x &= ~(1 << FLASH_L4_CR_MER1);
  x &= ~(1 << FLASH_L4_CR_MER2);
  x |= (n << FLASH_L4_CR_PNB);
  x |= (uint32_t)(1lu << FLASH_L4_CR_PER);
#if DEBUG_FLASH
  fprintf(stdout, "BKER:PNB:0x%x 0x%x\n", x, n);
#endif
  stlink_write_debug32(sl, FLASH_L4_CR, x);
}

static void set_flash_cr_strt(stlink_t *sl, uint32_t bank) {
  uint32_t val, cr_reg, cr_strt;

  if (sl->flash_type == STM32_FLASH_TYPE_C0) {
    cr_reg = FLASH_C0_CR;
    cr_strt = 1 << FLASH_C0_CR_STRT;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F2_F4) {
    cr_reg = FLASH_F4_CR;
    cr_strt = 1 << FLASH_F4_CR_STRT;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F7) {
    cr_reg = FLASH_F7_CR;
    cr_strt = 1 << FLASH_F7_CR_STRT;
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    cr_reg = FLASH_Gx_CR;
    cr_strt = (1 << FLASH_Gx_CR_STRT);
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    cr_reg = (bank == BANK_1) ? FLASH_H7_CR1 : FLASH_H7_CR2;
    cr_strt = 1 << FLASH_H7_CR_START(sl->chip_id);
  } else if (sl->flash_type == STM32_FLASH_TYPE_L4) {
    cr_reg = FLASH_L4_CR;
    cr_strt = (1 << FLASH_L4_CR_STRT);
  } else if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) {
    cr_reg = FLASH_L5_NSCR;
    cr_strt = (1 << FLASH_L5_NSCR_NSSTRT);
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
    cr_reg = FLASH_WB_CR;
    cr_strt = (1 << FLASH_WB_CR_STRT);
  } else {
    cr_reg = (bank == BANK_1) ? FLASH_CR : FLASH_CR2;
    cr_strt = (1 << FLASH_CR_STRT);
  }

  stlink_read_debug32(sl, cr_reg, &val);
  val |= cr_strt;
  stlink_write_debug32(sl, cr_reg, val);
}

static void set_flash_cr_mer(stlink_t *sl, bool v, uint32_t bank) {
  uint32_t val, cr_reg, cr_mer, cr_pg;

  if (sl->flash_type == STM32_FLASH_TYPE_C0) {
    cr_reg = FLASH_C0_CR;
    cr_mer = 1 << FLASH_CR_MER;
    cr_pg = 1 << FLASH_CR_PG;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F2_F4) {
    cr_reg = FLASH_F4_CR;
    cr_mer = 1 << FLASH_CR_MER;
    cr_pg = 1 << FLASH_CR_PG;
  } else if (sl->flash_type == STM32_FLASH_TYPE_F7) {
    cr_reg = FLASH_F7_CR;
    cr_mer = 1 << FLASH_CR_MER;
    cr_pg = 1 << FLASH_CR_PG;
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4) {
    cr_reg = FLASH_Gx_CR;
    cr_mer = (1 << FLASH_Gx_CR_MER1);
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
      cr_mer |= (1 << FLASH_Gx_CR_MER2);
    }
    cr_pg = (1 << FLASH_CR_PG);
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    cr_reg = (bank == BANK_1) ? FLASH_H7_CR1 : FLASH_H7_CR2;
    cr_mer = (1 << FLASH_H7_CR_BER);
    cr_pg = (1 << FLASH_H7_CR_PG);
  } else if (sl->flash_type == STM32_FLASH_TYPE_L4) {
    cr_reg = FLASH_L4_CR;
    cr_mer = (1 << FLASH_L4_CR_MER1) | (1 << FLASH_L4_CR_MER2);
    cr_pg = (1 << FLASH_L4_CR_PG);
  } else if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) {
    cr_reg = FLASH_L5_NSCR;
    cr_mer = (1 << FLASH_L5_NSCR_NSMER1) | (1 << FLASH_L5_NSCR_NSMER2);
    cr_pg = (1 << FLASH_L5_NSCR_NSPG);
  } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
    cr_reg = FLASH_WB_CR;
    cr_mer = (1 << FLASH_CR_MER);
    cr_pg = (1 << FLASH_CR_PG);
  } else {
    cr_reg = (bank == BANK_1) ? FLASH_CR : FLASH_CR2;
    cr_mer = (1 << FLASH_CR_MER);
    cr_pg = (1 << FLASH_CR_PG);
  }

  stlink_read_debug32(sl, cr_reg, &val);

  if (val & cr_pg) {
    // STM32F030 will drop MER bit if PG was set
    val &= ~cr_pg;
    stlink_write_debug32(sl, cr_reg, val);
  }

  if (v) {
    val |= cr_mer;
  } else {
    val &= ~cr_mer;
  }

  stlink_write_debug32(sl, cr_reg, val);
}

/**
 * Erase a page of flash, assumes sl is fully populated with things like
 * chip/core ids
 * @param sl stlink context
 * @param flashaddr an address in the flash page to erase
 * @return 0 on success -ve on failure
 */
int32_t stlink_erase_flash_page(stlink_t *sl, stm32_addr_t flashaddr) {
  // wait for ongoing op to finish
  wait_flash_busy(sl);
  // clear flash IO errors
  clear_flash_error(sl);

  if (sl->flash_type == STM32_FLASH_TYPE_F2_F4 ||
      sl->flash_type == STM32_FLASH_TYPE_F7 ||
      sl->flash_type == STM32_FLASH_TYPE_L4) {
    // unlock if locked
    unlock_flash_if(sl);

    // select the page to erase
    if (sl->flash_type == STM32_FLASH_TYPE_L4) {
      // calculate the actual bank+page from the address
      uint32_t page = calculate_L4_page(sl, flashaddr);

      fprintf(stderr, "EraseFlash - Page:0x%x Size:0x%x ", page,
              stlink_calculate_pagesize(sl, flashaddr));

      write_flash_cr_bker_pnb(sl, page);
    } else if (sl->chip_id == STM32_CHIPID_F7 ||
               sl->chip_id == STM32_CHIPID_F76xxx) {
      // calculate the actual page from the address
      uint32_t sector = calculate_F7_sectornum(flashaddr);

      fprintf(stderr, "EraseFlash - Sector:0x%x Size:0x%x ", sector,
              stlink_calculate_pagesize(sl, flashaddr));
      write_flash_cr_snb(sl, sector, BANK_1);
    } else {
      // calculate the actual page from the address
      uint32_t sector = calculate_F4_sectornum(flashaddr);

      fprintf(stderr, "EraseFlash - Sector:0x%x Size:0x%x ", sector,
              stlink_calculate_pagesize(sl, flashaddr));

      // the SNB values for flash sectors in the second bank do not directly
      // follow the values for the first bank on 2mb devices...
      if (sector >= 12) {
        sector += 4;
      }

      write_flash_cr_snb(sl, sector, BANK_1);
    }

    set_flash_cr_strt(sl, BANK_1); // start erase operation
    wait_flash_busy(sl);           // wait for completion
    lock_flash(sl);                // TODO: fails to program if this is in
#if DEBUG_FLASH
    fprintf(stdout, "Erase Final CR:0x%x\n", read_flash_cr(sl, BANK_1));
#endif
  } else if (sl->flash_type == STM32_FLASH_TYPE_L0_L1) {

    uint32_t val;
    uint32_t flash_regs_base = get_stm32l0_flash_base(sl);

    // check if the locks are set
    stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);

    if ((val & (1 << 0)) || (val & (1 << 1))) {
      // disable pecr protection
      stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF, FLASH_L0_PEKEY1);
      stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF, FLASH_L0_PEKEY2);

      // check pecr.pelock is cleared
      stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);

      if (val & (1 << 0)) {
        WLOG("pecr.pelock not clear (%#x)\n", val);
        return (-1);
      }

      // unlock program memory
      stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, FLASH_L0_PRGKEY1);
      stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, FLASH_L0_PRGKEY2);

      // check pecr.prglock is cleared
      stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);

      if (val & (1 << 1)) {
        WLOG("pecr.prglock not clear (%#x)\n", val);
        return (-1);
      }
    }

    // set pecr.{erase,prog}
    val |= (1 << 9) | (1 << 3);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);

    // write 0 to the first word of the page to be erased
    stlink_write_debug32(sl, flashaddr, 0);

    /* MP: It is better to wait for clearing the busy bit after issuing page
     * erase command, even though PM0062 recommends to wait before it.
     * Test shows that a few iterations is performed in the following loop
     * before busy bit is cleared.
     */
    wait_flash_busy(sl);

    // reset lock bits
    stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
    val |= (1 << 0) | (1 << 1) | (1 << 2);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
  } else if (sl->flash_type == STM32_FLASH_TYPE_G0 ||
             sl->flash_type == STM32_FLASH_TYPE_G4 ||
             sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5 ||
             sl->flash_type == STM32_FLASH_TYPE_WB_WL ||
             sl->flash_type == STM32_FLASH_TYPE_C0) {
    uint32_t val;
    unlock_flash_if(sl);
    set_flash_cr_per(sl, BANK_1); // set the 'enable Flash erase' bit

    // set the page to erase
    if (sl->flash_type == STM32_FLASH_TYPE_G0) {
      uint32_t flash_page = ((flashaddr - STM32_FLASH_BASE) / sl->flash_pgsz);
      stlink_read_debug32(sl, FLASH_Gx_CR, &val);
      // sec 3.7.5 - PNB[9:0] is offset by 3. PER is 0x2.
      val &= ~(0x3FF << 3);
      val |= ((flash_page & 0x3FF) << 3) | (1 << FLASH_CR_PER);
      stlink_write_debug32(sl, FLASH_Gx_CR, val);
    } else if (sl->flash_type == STM32_FLASH_TYPE_G4) {
      uint32_t flash_page = ((flashaddr - STM32_FLASH_BASE) / sl->flash_pgsz);
      stlink_read_debug32(sl, FLASH_Gx_CR, &val);
      // sec 3.7.5 - PNB[9:0] is offset by 3. PER is 0x2.
      val &= ~(0x7FF << 3);
      val |= ((flash_page & 0x7FF) << 3) | (1 << FLASH_CR_PER);
      stlink_write_debug32(sl, FLASH_Gx_CR, val);
    // STM32L5x2xx has two banks with 2k pages or single with 4k pages
    // STM32H5xx, STM32U535, STM32U545, STM32U575 or STM32U585 have 2 banks with 8k pages
    } else if (sl->flash_type == STM32_FLASH_TYPE_L5_U5_H5) {
      uint32_t flash_page;
      stlink_read_debug32(sl, FLASH_L5_NSCR, &val);
      if ((sl->flash_pgsz == 0x800 || sl->flash_pgsz == 0x2000) && (flashaddr - STM32_FLASH_BASE) >= sl->flash_size/2) {
        flash_page = (flashaddr - STM32_FLASH_BASE - sl->flash_size/2) / sl->flash_pgsz;
        // set bank 2 for erasure
        val |= (1 << FLASH_L5_NSCR_NSBKER);
      } else {
        flash_page = ((flashaddr - STM32_FLASH_BASE) / sl->flash_pgsz);
        // set bank 1 for erasure
        val &= ~(1 << FLASH_L5_NSCR_NSBKER);
      }
      // sec 7.9.9 for U5, 6.9.9 for L5 (for L7 we have 7 bits instead of 8 bits for U5 but 
      // the bit position for 8th bit reserved.
      // Maybe the best solution is to handle each one separately.
      val &= ~(0xFF << 3);
      val |= ((flash_page & 0xFF) << 3) | (1 << FLASH_CR_PER);
      stlink_write_debug32(sl, FLASH_L5_NSCR, val);
    } else if (sl->flash_type == STM32_FLASH_TYPE_WB_WL) {
      uint32_t flash_page = ((flashaddr - STM32_FLASH_BASE) / sl->flash_pgsz);
      stlink_read_debug32(sl, FLASH_WB_CR, &val);

      // sec 3.10.5 - PNB[7:0] is offset by 3.
      val &= ~(0xFF << 3); // Clear previously set page number (if any)
      val |= ((flash_page & 0xFF) << 3);

      stlink_write_debug32(sl, FLASH_WB_CR, val);
    } else if (sl->flash_type == STM32_FLASH_TYPE_C0) {
      uint32_t flash_page = ((flashaddr - STM32_FLASH_BASE) / sl->flash_pgsz);
      stlink_read_debug32(sl, FLASH_C0_CR, &val);

      val &= ~(0xF << FLASH_C0_CR_PNB);
      val |= ((flash_page & 0xF) << FLASH_C0_CR_PNB);

      stlink_write_debug32(sl, FLASH_C0_CR, val);
    }

    set_flash_cr_strt(sl, BANK_1);  // set the 'start operation' bit
    wait_flash_busy(sl);            // wait for the 'busy' bit to clear
    clear_flash_cr_per(sl, BANK_1); // clear the 'enable page erase' bit
    lock_flash(sl);
  } else if (sl->flash_type == STM32_FLASH_TYPE_F0_F1_F3 ||
             sl->flash_type == STM32_FLASH_TYPE_F1_XL) {
    uint32_t bank = (flashaddr < STM32_F1_FLASH_BANK2_BASE) ? BANK_1 : BANK_2;
    unlock_flash_if(sl);
    clear_flash_cr_pg(sl, bank);         // clear the pg bit
    set_flash_cr_per(sl, bank);          // set the page erase bit
    write_flash_ar(sl, flashaddr, bank); // select the page to erase
    set_flash_cr_strt(sl, bank); // start erase operation, reset by hw with busy bit
    wait_flash_busy(sl);
    clear_flash_cr_per(sl, bank); // clear the page erase bit
    lock_flash(sl);
  } else if (sl->flash_type == STM32_FLASH_TYPE_H7) {
    uint32_t bank = (flashaddr < STM32_H7_FLASH_BANK2_BASE) ? BANK_1 : BANK_2;
    unlock_flash_if(sl); // unlock if locked
    uint32_t sector = calculate_H7_sectornum(sl, flashaddr, bank); // calculate the actual page from the address
    write_flash_cr_snb(sl, sector, bank); // select the page to erase
    set_flash_cr_strt(sl, bank);          // start erase operation
    wait_flash_busy(sl);                  // wait for completion
    lock_flash(sl);
  } else {
    WLOG("unknown coreid %x, page erase failed\n", sl->core_id);
    return (-1);
  }

  return check_flash_error(sl);
}

int32_t stlink_erase_flash_section(stlink_t *sl, stm32_addr_t base_addr, uint32_t size, bool align_size) {
  // Check the address and size validity
  if (stlink_check_address_range_validity(sl, base_addr, size) < 0) {
    return -1;
  }

  // Make sure the requested address is aligned with the beginning of a page
  if (stlink_check_address_alignment(sl, base_addr) < 0) {
    ELOG("The address to erase is not aligned with the beginning of a page\n");
    return -1;
  }

  stm32_addr_t addr = base_addr;
  do {
    uint32_t page_size = stlink_calculate_pagesize(sl, addr);

    // Check if size is aligned with a page, unless we want to completely erase the last page
    if ((addr + page_size) > (base_addr + size) && !align_size) {
      ELOG("Invalid size (not aligned with a page). Page size at address %#x is %#x\n", addr, page_size);
      return (-1);
    }

    if (stlink_erase_flash_page(sl, addr)) {
      WLOG("Failed to erase_flash_page(%#x) == -1\n", addr);
      return (-1);
    }

    fprintf(stdout, "-> Flash page at %#x erased (size: %#x)\r", addr, page_size);
    fflush(stdout);

    // check the next page is within the range to erase
    addr += page_size;
  } while (addr < (base_addr + size));

  fprintf(stdout, "\n");
  return 0;
}

int32_t stlink_erase_flash_mass(stlink_t *sl) {
  int32_t err = 0;

  // TODO: Use MER bit to mass-erase WB series.
  if (sl->flash_type == STM32_FLASH_TYPE_L0_L1 ||
      sl->flash_type == STM32_FLASH_TYPE_WB_WL) {

    err = stlink_erase_flash_section(sl, sl->flash_base, sl->flash_size, false);

  } else {
    wait_flash_busy(sl);
    clear_flash_error(sl);
    unlock_flash_if(sl);

    if (sl->flash_type == STM32_FLASH_TYPE_H7 && sl->chip_id != STM32_CHIPID_H7Ax) {
      // set parallelism
      write_flash_cr_psiz(sl, 3 /*64bit*/, BANK_1);
      if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
        write_flash_cr_psiz(sl, 3 /*64bit*/, BANK_2);
      }
    }

    set_flash_cr_mer(sl, 1, BANK_1); // set the mass erase bit
    set_flash_cr_strt(sl, BANK_1); // start erase operation, reset by hw with busy bit

    if (sl->flash_type == STM32_FLASH_TYPE_F1_XL ||
        (sl->flash_type == STM32_FLASH_TYPE_H7 && sl->chip_flags & CHIP_F_HAS_DUAL_BANK)) {
      set_flash_cr_mer(sl, 1, BANK_2); // set the mass erase bit in bank 2
      set_flash_cr_strt(sl, BANK_2);   // start erase operation in bank 2
    }

    wait_flash_busy_progress(sl);
    lock_flash(sl);

    // reset the mass erase bit
    set_flash_cr_mer(sl, 0, BANK_1);
    if (sl->flash_type == STM32_FLASH_TYPE_F1_XL ||
        (sl->flash_type == STM32_FLASH_TYPE_H7 && sl->chip_flags & CHIP_F_HAS_DUAL_BANK)) {
      set_flash_cr_mer(sl, 0, BANK_2);
    }

    err = check_flash_error(sl);
  }

  return (err);
}

int32_t stlink_mwrite_flash(stlink_t *sl, uint8_t *data, uint32_t length, stm32_addr_t addr) {
  /* Write the block in flash at addr */
  int32_t err;
  uint32_t num_empty, idx;
  uint8_t erased_pattern = stlink_get_erased_pattern(sl);

  /*
   * This optimisation may cause unexpected garbage data remaining.
   * Therfore it is turned off by default.
   */
  if (sl->opt) {
    idx = length;

    for (num_empty = 0; num_empty != length; ++num_empty)
      if (data[--idx] != erased_pattern) {
        break;
      }

    num_empty -= (num_empty & 3); // Round down to words

    if (num_empty != 0) {
      ILOG("Ignoring %d bytes of 0x%02x at end of file\n", num_empty, erased_pattern);
    }
  } else {
    num_empty = 0;
  }

  /*
   * TODO: investigate a kind of weird behaviour here:
   * If the file is identified to be all-empty and four-bytes aligned,
   * still flash the whole file even if ignoring message is printed.
   */
  err = stlink_write_flash(sl, addr, data,
                          (num_empty == length) ? length : length - num_empty,
                          num_empty == length);
  stlink_fwrite_finalize(sl, addr);
  return (err);
}

/**
 * Write the given binary file into flash at address "addr"
 * @param sl
 * @param path readable file path, should be binary image
 * @param addr where to start writing
 * @return 0 on success, -ve on failure.
 */
int32_t stlink_fwrite_flash(stlink_t *sl, const char *path, stm32_addr_t addr) {
  /* Write the file in flash at addr */
  int32_t err;
  uint32_t num_empty, idx;
  uint8_t erased_pattern = stlink_get_erased_pattern(sl);
  mapped_file_t mf = MAPPED_FILE_INITIALIZER;

  if (map_file(&mf, path) == -1) {
    ELOG("map_file() == -1\n");
    return (-1);
  }

  printf("file %s ", path);
  md5_calculate(&mf);
  stlink_checksum(&mf);

  if (sl->opt) {
    idx = (uint32_t)mf.len;

    for (num_empty = 0; num_empty != mf.len; ++num_empty) {
      if (mf.base[--idx] != erased_pattern) {
        break;
      }
    }

    num_empty -= (num_empty & 3); // round down to words

    if (num_empty != 0) {
      ILOG("Ignoring %d bytes of 0x%02x at end of file\n", num_empty, erased_pattern);
    }
  } else {
    num_empty = 0;
  }

  /*
   * TODO: investigate a kind of weird behaviour here:
   * If the file is identified to be all-empty and four-bytes aligned,
   * still flash the whole file even if ignoring message is printed.
   */

  /* In case the address is within the OTP area we use a different flash method */
  if(addr >= sl->otp_base && addr < sl->otp_base + sl->otp_size) {
    err = stlink_write_otp(sl, addr, mf.base,
                           (num_empty == mf.len) ? (uint32_t)mf.len : (uint32_t)mf.len - num_empty);
  } else {
    err = stlink_write_flash(sl, addr, mf.base,
                           (num_empty == mf.len) ? (uint32_t)mf.len : (uint32_t)mf.len - num_empty,
                           num_empty == mf.len);
  }
  stlink_fwrite_finalize(sl, addr);
  unmap_file(&mf);
  return (err);
}


int32_t stlink_fcheck_flash(stlink_t *sl, const char *path, stm32_addr_t addr) {
  // check the contents of path are at addr

  int32_t res;
  mapped_file_t mf = MAPPED_FILE_INITIALIZER;

  if (map_file(&mf, path) == -1) {
    return (-1);
  }

  res = check_file(sl, &mf, addr);
  unmap_file(&mf);
  return (res);
}

/**
 * Verify addr..addr+len is binary identical to base...base+len
 * @param sl stlink context
 * @param address stm device address
 * @param data host side buffer to check against
 * @param length how much
 * @return 0 for success, -ve for failure
 */
int32_t stlink_verify_write_flash(stlink_t *sl, stm32_addr_t address, uint8_t *data, uint32_t length) {
  uint32_t off;
  uint32_t cmp_size = (sl->flash_pgsz > 0x1800) ? 0x1800 : sl->flash_pgsz;
  ILOG("Starting verification of write complete\n");

  for (off = 0; off < length; off += cmp_size) {
    uint32_t aligned_size;

    // adjust last page size
    if ((off + cmp_size) > length) {
      cmp_size = length - off;
    }

    aligned_size = cmp_size;

    if (aligned_size & (4 - 1)) {
      aligned_size = (cmp_size + 4) & ~(4 - 1);
    }

    stlink_read_mem32(sl, address + off, (uint16_t)aligned_size);

    if (memcmp(sl->q_buf, data + off, cmp_size)) {
      ELOG("Verification of flash failed at offset: %u\n", off);
      return (-1);
    }
  }

  ILOG("Flash written and verified! jolly good!\n");
  return (0);
}

// Check if an address and size are within the flash
int32_t stlink_check_address_range_validity(stlink_t *sl, stm32_addr_t addr, uint32_t size) {
  uint32_t logvar;
  if (addr < sl->flash_base || addr >= (sl->flash_base + sl->flash_size)) {
    logvar = sl->flash_base + sl->flash_size - 1;
    ELOG("Invalid address, it should be within 0x%08x - 0x%08x\n", sl->flash_base, logvar);
    return (-1);
  }
  if ((addr + size) > (sl->flash_base + sl->flash_size)) {
    logvar = sl->flash_base + sl->flash_size - addr;
    ELOG("The size exceeds the size of the flash (0x%08x bytes available)\n", logvar);
    return (-1);
  }
  return 0;
}

// Check if an address and size are within the flash (otp area)
int32_t stlink_check_address_range_validity_otp(stlink_t *sl, stm32_addr_t addr, uint32_t size) {
  uint32_t logvar;
  if (addr < sl->otp_base || addr >= (sl->otp_base + sl->otp_size)) {
    logvar = sl->otp_base + sl->otp_size - 1;
    ELOG("Invalid address, it should be within 0x%08x - 0x%08x\n", sl->otp_base, logvar);
    return (-1);
  }
  if ((addr + size) >= (sl->otp_base + sl->otp_size)) {
    logvar = sl->otp_base + sl->otp_size - addr;
    ELOG("The size exceeds the size of the OTP Area (0x%08x bytes available)\n", logvar);
    return (-1);
  }
  return 0;
}

// Check if an address is aligned with the beginning of a page
int32_t stlink_check_address_alignment(stlink_t *sl, stm32_addr_t addr) {
  stm32_addr_t page = sl->flash_base;

  while (page < addr) {
    page += stlink_calculate_pagesize(sl, page);
  }

  if (page != addr) {
    return -1;
  }

  return 0;
}

int32_t stlink_write_flash(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len, uint8_t eraseonly) {
  int32_t ret;
  flash_loader_t fl;
  ILOG("Attempting to write %d (%#x) bytes to stm32 address: %u (%#x)\n", len, len, addr, addr);

  // check addr range is inside the flash
  stlink_calculate_pagesize(sl, addr);

  // Check the address and size validity
  if (stlink_check_address_range_validity(sl, addr, len) < 0) {
    return (-1);
  } else if (len & 1) {
    WLOG("unaligned len 0x%x -- padding with zero\n", len);
    len += 1;
  } else if (stlink_check_address_alignment(sl, addr) < 0) {
    ELOG("addr not a multiple of current pagesize (%u bytes), not supported, "
         "check page start address and compare with flash module organisation "
         "in related ST reference manual of your device.\n",
         sl->flash_pgsz);
    return (-1);
  }

  // make sure we've loaded the context with the chip details
  stlink_core_id(sl);

  // Erase this section of the flash
  if (stlink_erase_flash_section(sl, addr, len, true) < 0) {
    ELOG("Failed to erase the flash prior to writing\n");
    return (-1);
  }

  if (eraseonly) {
    return (0);
  }
 
  ret = stlink_flashloader_start(sl, &fl);
  if (ret)
    return ret;
  ret = stlink_flashloader_write(sl, &fl, addr, base, len);
  if (ret)
    return ret;
  ret = stlink_flashloader_stop(sl, &fl);
  if (ret)
    return ret;

  return (stlink_verify_write_flash(sl, addr, base, len));
}

int32_t stlink_write_otp(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len) {
  int32_t ret;
  flash_loader_t fl;
  ILOG("Attempting to write %d (%#x) bytes to stm32 address: %u (%#x)\n", len, len, addr, addr);
  
  // Check the address and size validity
  if (stlink_check_address_range_validity_otp(sl, addr, len) < 0) {
    return (-1);
  }

  // make sure we've loaded the context with the chip details
  stlink_core_id(sl);
 
  ret = stlink_flashloader_start(sl, &fl);
  if (ret)
    return ret;
  ret = stlink_flashloader_write(sl, &fl, addr, base, len);
  if (ret)
    return ret;
  ret = stlink_flashloader_stop(sl, &fl);
  if (ret)
    return ret;

  return (stlink_verify_write_flash(sl, addr, base, len));
}

void stlink_fwrite_finalize(stlink_t *sl, stm32_addr_t addr) {
  uint32_t val;
  // set PC to the reset routine
  stlink_read_debug32(sl, addr + 4, &val);
  stlink_write_reg(sl, val, 15);
  stlink_run(sl, RUN_NORMAL);
}
