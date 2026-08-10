/**
  ******************************************************************************
  * @file           : stm32_flash.h
  * @brief          : STM32 specific flash registers, addresses and parametres
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef STM32_FLASH_H
#define STM32_FLASH_H

#include <stdint.h>

/* STM32Fx FPEC flash controller interface, PM0063 manual */
// STM32F05x is identical, based on RM0091 (DM00031936, Doc ID 018940 Rev.2, Aug 2012)
#define FLASH_REGS_ADDR 0x40022000
#define FLASH_REGS_SIZE 0x28

#define FLASH_ACR (FLASH_REGS_ADDR + 0x00)
#define FLASH_KEYR (FLASH_REGS_ADDR + 0x04)
#define FLASH_OPTKEYR (FLASH_REGS_ADDR + 0x08)
#define FLASH_SR (FLASH_REGS_ADDR + 0x0c)
#define FLASH_CR (FLASH_REGS_ADDR + 0x10)
#define FLASH_AR (FLASH_REGS_ADDR + 0x14)
#define FLASH_OBR (FLASH_REGS_ADDR + 0x1c)
#define FLASH_WRPR (FLASH_REGS_ADDR + 0x20)

// STM32F10x_XL has two flash memory banks
// with separate registers to control the second bank.
#define FLASH_KEYR2 (FLASH_REGS_ADDR + 0x44)
#define FLASH_SR2 (FLASH_REGS_ADDR + 0x4c)
#define FLASH_CR2 (FLASH_REGS_ADDR + 0x50)
#define FLASH_AR2 (FLASH_REGS_ADDR + 0x54)

#define FLASH_RDPTR_KEY 0x00a5
#define FLASH_KEY1 0x45670123
#define FLASH_KEY2 0xcdef89ab

#define FLASH_OPTKEY1 0x08192a3b
#define FLASH_OPTKEY2 0x4c5d6e7f

#define FLASH_SR_BSY 0
#define FLASH_SR_PG_ERR 2
#define FLASH_SR_WRPRT_ERR 4
#define FLASH_SR_EOP 5

#define FLASH_SR_ERROR_MASK ((1 << FLASH_SR_PG_ERR) | (1 << FLASH_SR_WRPRT_ERR))

#define FLASH_CR_PG 0
#define FLASH_CR_PER 1
#define FLASH_CR_MER 2
#define FLASH_CR_OPTPG 4
#define FLASH_CR_OPTER 5
#define FLASH_CR_STRT 6
#define FLASH_CR_LOCK 7
#define FLASH_CR_OPTWRE 9
#define FLASH_CR_OBL_LAUNCH 13

#define FLASH_ACR_OFF     ((uint32_t) 0x00)
#define FLASH_PECR_OFF    ((uint32_t) 0x04)
#define FLASH_PDKEYR_OFF  ((uint32_t) 0x08)
#define FLASH_PEKEYR_OFF  ((uint32_t) 0x0c)
#define FLASH_PRGKEYR_OFF ((uint32_t) 0x10)
#define FLASH_OPTKEYR_OFF ((uint32_t) 0x14)
#define FLASH_SR_OFF      ((uint32_t) 0x18)
#define FLASH_OBR_OFF     ((uint32_t) 0x1c)
#define FLASH_WRPR_OFF    ((uint32_t) 0x20)

// == STM32C0 == (RM0490)
// C0 Flash registers
#define STM32_FLASH_C0_REGS_ADDR ((uint32_t) 0x40022000)
#define STM32_FLASH_C0_KEYR (STM32_FLASH_C0_REGS_ADDR + 0x08)
#define STM32_FLASH_C0_OPT_KEYR (STM32_FLASH_C0_REGS_ADDR + 0x0C)
#define STM32_FLASH_C0_SR (STM32_FLASH_C0_REGS_ADDR + 0x10)
#define STM32_FLASH_C0_CR (STM32_FLASH_C0_REGS_ADDR + 0x14)
#define STM32_FLASH_C0_OPTR (STM32_FLASH_C0_REGS_ADDR + 0x20)

// C0 Flash control register
#define STM32_FLASH_C0_CR_PNB 3
#define STM32_FLASH_C0_CR_STRT 16
#define STM32_FLASH_C0_CR_OPTSTRT 17
#define STM32_FLASH_C0_CR_OBL_LAUNCH 27
#define STM32_FLASH_C0_CR_OPTLOCK 30
#define STM32_FLASH_C0_CR_LOCK 31

// C0 Flash status register
#define STM32_FLASH_C0_SR_ERROR_MASK 0xC3F8 // [15:14], [9:3]
#define STM32_FLASH_C0_SR_PROGERR 3
#define STM32_FLASH_C0_SR_WRPERR 4
#define STM32_FLASH_C0_SR_PGAERR 5
#define STM32_FLASH_C0_SR_BSY 16

// == STM32F0 ==
#define STM32_FLASH_F0_OPTKEY1 0x45670123
#define STM32_FLASH_F0_OPTKEY2 0xcdef89ab

// == STM32F2 ==
#define STM32_FLASH_F2_REGS_ADDR ((uint32_t) 0x40023c00)
#define STM32_FLASH_F2_KEYR (STM32_FLASH_F2_REGS_ADDR + 0x04)
#define STM32_FLASH_F2_OPT_KEYR (STM32_FLASH_F2_REGS_ADDR + 0x08)
#define STM32_FLASH_F2_SR (STM32_FLASH_F2_REGS_ADDR + 0x0c)
#define STM32_FLASH_F2_CR (STM32_FLASH_F2_REGS_ADDR + 0x10)
#define STM32_FLASH_F2_OPT_CR (STM32_FLASH_F2_REGS_ADDR + 0x14)
#define STM32_FLASH_F2_OPT_LOCK_BIT (1u << 0)

// F2 Flash control register
#define STM32_FLASH_F2_CR_STRT 16
#define STM32_FLASH_F2_CR_LOCK 31
#define STM32_FLASH_F2_CR_SER 1
#define STM32_FLASH_F2_CR_SNB 3
#define STM32_FLASH_F2_CR_SNB_MASK 0x78

// F2 Flash status register
#define STM32_FLASH_F2_SR_BSY 16

// == STM32F4 ==
// F4 Flash registers
#define STM32_FLASH_F4_REGS_ADDR ((uint32_t) 0x40023c00)
#define STM32_FLASH_F4_KEYR (STM32_FLASH_F4_REGS_ADDR + 0x04)
#define STM32_FLASH_F4_OPT_KEYR (STM32_FLASH_F4_REGS_ADDR + 0x08)
#define STM32_FLASH_F4_SR (STM32_FLASH_F4_REGS_ADDR + 0x0c)
#define STM32_FLASH_F4_CR (STM32_FLASH_F4_REGS_ADDR + 0x10)
#define STM32_FLASH_F4_OPTCR (STM32_FLASH_F4_REGS_ADDR + 0x14)
#define STM32_FLASH_F4_OPTCR_LOCK 0
#define STM32_FLASH_F4_OPTCR_START 1

// F4 Flash control register
#define STM32_FLASH_F4_CR_STRT 16
#define STM32_FLASH_F4_CR_LOCK 31
#define STM32_FLASH_F4_CR_SER 1
#define STM32_FLASH_F4_CR_SNB 3
#define STM32_FLASH_F4_CR_SNB_MASK 0xf8

// F4 Flash status register
#define STM32_FLASH_F4_SR_ERROR_MASK 0x000000F0
#define STM32_FLASH_F4_SR_PGAERR 5
#define STM32_FLASH_F4_SR_WRPERR 4
#define STM32_FLASH_F4_SR_BSY 16

// == STM32F7 ==
// F7 Flash registers
#define STM32_FLASH_F7_REGS_ADDR ((uint32_t) 0x40023c00)
#define STM32_FLASH_F7_KEYR (STM32_FLASH_F7_REGS_ADDR + 0x04)
#define STM32_FLASH_F7_OPT_KEYR (STM32_FLASH_F7_REGS_ADDR + 0x08)
#define STM32_FLASH_F7_SR (STM32_FLASH_F7_REGS_ADDR + 0x0c)
#define STM32_FLASH_F7_CR (STM32_FLASH_F7_REGS_ADDR + 0x10)
#define STM32_FLASH_F7_OPTCR (STM32_FLASH_F7_REGS_ADDR + 0x14)
#define STM32_FLASH_F7_OPTCR1 (STM32_FLASH_F7_REGS_ADDR + 0x18)
#define STM32_FLASH_F7_OPTCR_LOCK 0
#define STM32_FLASH_F7_OPTCR_START 1
#define STM32_FLASH_F7_OPTCR1_BOOT_ADD0 0
#define STM32_FLASH_F7_OPTCR1_BOOT_ADD1 16

// F7 Flash control register
#define STM32_FLASH_F7_CR_STRT 16
#define STM32_FLASH_F7_CR_LOCK 31
#define STM32_FLASH_F7_CR_SER 1
#define STM32_FLASH_F7_CR_SNB 3
#define STM32_FLASH_F7_CR_SNB_MASK 0xf8

// F7 Flash status register
#define STM32_FLASH_F7_SR_BSY 16
#define STM32_FLASH_F7_SR_ERS_ERR 7       /* Erase Sequence Error */
#define STM32_FLASH_F7_SR_PGP_ERR 6       /* Programming parallelism error */
#define STM32_FLASH_F7_SR_PGA_ERR 5       /* Programming alignment error */
#define STM32_FLASH_F7_SR_WRP_ERR 4       /* Write protection error */
#define STM32_FLASH_F7_SR_OP_ERR 1        /* Operation error */
#define STM32_FLASH_F7_SR_EOP 0           /* End of operation */
#define STM32_FLASH_F7_SR_ERROR_MASK                               \
  ((1 << STM32_FLASH_F7_SR_ERS_ERR) | (1 << STM32_FLASH_F7_SR_PGP_ERR) | \
   (1 << STM32_FLASH_F7_SR_PGA_ERR) | (1 << STM32_FLASH_F7_SR_WRP_ERR) | \
   (1 << STM32_FLASH_F7_SR_OP_ERR))

// == STM32G0/G4 ==
// G0/G4 Flash registers (RM0440, p.146)
#define STM32_FLASH_Gx_REGS_ADDR ((uint32_t) 0x40022000)
#define STM32_FLASH_Gx_ACR (STM32_FLASH_Gx_REGS_ADDR + 0x00)
#define STM32_FLASH_Gx_KEYR (STM32_FLASH_Gx_REGS_ADDR + 0x08)
#define STM32_FLASH_Gx_OPTKEYR (STM32_FLASH_Gx_REGS_ADDR + 0x0c)
#define STM32_FLASH_Gx_SR (STM32_FLASH_Gx_REGS_ADDR + 0x10)
#define STM32_FLASH_Gx_CR (STM32_FLASH_Gx_REGS_ADDR + 0x14)
#define STM32_FLASH_Gx_ECCR (STM32_FLASH_Gx_REGS_ADDR + 0x18)
#define STM32_FLASH_Gx_OPTR (STM32_FLASH_Gx_REGS_ADDR + 0x20)

// G0/G4 Flash control register
#define STM32_FLASH_Gx_CR_PG (0)          /* Program */
#define STM32_FLASH_Gx_CR_PER (1)         /* Page erase */
#define STM32_FLASH_Gx_CR_MER1 (2)        /* Mass erase */
#define STM32_FLASH_Gx_CR_PNB (3)         /* Page number */
#define STM32_FLASH_G0_CR_PNG_LEN (5)     /* STM32G0: 5 page number bits */
#define STM32_FLASH_G4_CR_PNG_LEN (7)     /* STM32G4: 7 page number bits */
#define STM32_FLASH_G0_CR_BKER (13)       /* Bank selection for erase operation on G0*/
#define STM32_FLASH_G4_CR_BKER (11)       /* Bank selection for erase operation on G4*/
#define STM32_FLASH_Gx_CR_MER2 (15)       /* Mass erase (2nd bank)*/
#define STM32_FLASH_Gx_CR_STRT (16)       /* Start */
#define STM32_FLASH_Gx_CR_OPTSTRT (17)    /* Start of modification of option bytes */
#define STM32_FLASH_Gx_CR_FSTPG (18)      /* Fast programming */
#define STM32_FLASH_Gx_CR_EOPIE (24)      /* End of operation interrupt enable */
#define STM32_FLASH_Gx_CR_ERRIE (25)      /* Error interrupt enable */
#define STM32_FLASH_Gx_CR_OBL_LAUNCH (27) /* Forces the option byte loading */
#define STM32_FLASH_Gx_CR_OPTLOCK (30)    /* Options Lock */
#define STM32_FLASH_Gx_CR_LOCK (31)       /* FLASH_CR Lock */

// G0/G4 Flash status register
#define STM32_FLASH_Gx_SR_ERROR_MASK (0x3fa)
#define STM32_FLASH_Gx_SR_PROGERR (3)
#define STM32_FLASH_Gx_SR_WRPERR (4)
#define STM32_FLASH_Gx_SR_PGAERR (5)
#define STM32_FLASH_Gx_SR_BSY (16)        /* FLASH_SR Busy */
#define STM32_FLASH_Gx_SR_EOP (0)         /* FLASH_EOP End of Operation */

// == STM32G0 == (RM0444 Table 1, sec. 3.7)
// Mostly the same as G4 chips, but the notation
// varies a bit after the 'OPTR' register.
#define STM32_FLASH_G0_REGS_ADDR (STM32_FLASH_Gx_REGS_ADDR)
#define STM32_FLASH_G0_PCROP1ASR (STM32_FLASH_G0_REGS_ADDR + 0x24)
#define STM32_FLASH_G0_PCROP1AER (STM32_FLASH_G0_REGS_ADDR + 0x28)
#define STM32_FLASH_G0_WRP1AR (STM32_FLASH_G0_REGS_ADDR + 0x2c)
#define STM32_FLASH_G0_WRP1BR (STM32_FLASH_G0_REGS_ADDR + 0x30)
#define STM32_FLASH_G0_PCROP1BSR (STM32_FLASH_G0_REGS_ADDR + 0x34)
#define STM32_FLASH_G0_PCROP1BER (STM32_FLASH_G0_REGS_ADDR + 0x38)
#define STM32_FLASH_G0_SECR (STM32_FLASH_G0_REGS_ADDR + 0x80)

// == STM32G4 == (RM0440 Table 17, sec. 3.7.19)

#define STM32_FLASH_G4_OPTR_DBANK (22) /* FLASH option register FLASH_OPTR Dual-Bank Mode */

// There are a few extra registers because 'cat 3' devices can have
// two Flash banks.
#define STM32_FLASH_G4_REGS_ADDR (STM32_FLASH_Gx_REGS_ADDR)
#define STM32_FLASH_G4_PDKEYR (STM32_FLASH_G4_REGS_ADDR + 0x04)
#define STM32_FLASH_G4_PCROP1SR (STM32_FLASH_G4_REGS_ADDR + 0x24)
#define STM32_FLASH_G4_PCROP1ER (STM32_FLASH_G4_REGS_ADDR + 0x28)
#define STM32_FLASH_G4_WRP1AR (STM32_FLASH_G4_REGS_ADDR + 0x2c)
#define STM32_FLASH_G4_WRP1BR (STM32_FLASH_G4_REGS_ADDR + 0x30)
#define STM32_FLASH_G4_PCROP2SR (STM32_FLASH_G4_REGS_ADDR + 0x44)
#define STM32_FLASH_G4_PCROP2ER (STM32_FLASH_G4_REGS_ADDR + 0x48)
#define STM32_FLASH_G4_WRP2AR (STM32_FLASH_G4_REGS_ADDR + 0x4c)
#define STM32_FLASH_G4_WRP2BR (STM32_FLASH_G4_REGS_ADDR + 0x50)
#define STM32_FLASH_G4_SEC1R (STM32_FLASH_G4_REGS_ADDR + 0x70)
#define STM32_FLASH_G4_SEC2R (STM32_FLASH_G4_REGS_ADDR + 0x74)

// == STM32H7 ==
// H7 Flash registers
#define STM32_FLASH_H7_REGS_ADDR ((uint32_t) 0x52002000)
#define STM32_FLASH_H7_KEYR1 (STM32_FLASH_H7_REGS_ADDR + 0x04)
#define STM32_FLASH_H7_KEYR2 (STM32_FLASH_H7_REGS_ADDR + 0x104)
#define STM32_FLASH_H7_OPT_KEYR (STM32_FLASH_H7_REGS_ADDR + 0x08)
#define STM32_FLASH_H7_CR1 (STM32_FLASH_H7_REGS_ADDR + 0x0c)
#define STM32_FLASH_H7_CR2 (STM32_FLASH_H7_REGS_ADDR + 0x10c)
#define STM32_FLASH_H7_SR1 (STM32_FLASH_H7_REGS_ADDR + 0x10)
#define STM32_FLASH_H7_SR2 (STM32_FLASH_H7_REGS_ADDR + 0x110)
#define STM32_FLASH_H7_CCR1 (STM32_FLASH_H7_REGS_ADDR + 0x14)
#define STM32_FLASH_H7_CCR2 (STM32_FLASH_H7_REGS_ADDR + 0x114)
#define STM32_FLASH_H7_OPTCR (STM32_FLASH_H7_REGS_ADDR + 0x18)
#define STM32_FLASH_H7_OPTCR2 (STM32_FLASH_H7_REGS_ADDR + 0x118)
#define STM32_FLASH_H7_OPTSR_CUR (STM32_FLASH_H7_REGS_ADDR + 0x1c)
#define STM32_FLASH_H7_OPTCCR (STM32_FLASH_H7_REGS_ADDR + 0x24)

#define STM32_FLASH_H7_OPTCR_OPTLOCK 0
#define STM32_FLASH_H7_OPTCR_OPTSTART 1
#define STM32_FLASH_H7_OPTCR_MER 4

#define STM32_FLASH_H7_OPTSR_OPT_BUSY 0
#define STM32_FLASH_H7_OPTSR_OPTCHANGEERR 30

#define STM32_FLASH_H7_OPTCCR_CLR_OPTCHANGEERR 30

// H7 Flash control register
#define STM32_FLASH_H7_CR_LOCK 0
#define STM32_FLASH_H7_CR_PG 1
#define STM32_FLASH_H7_CR_SER 2
#define STM32_FLASH_H7_CR_BER 3
#define STM32_FLASH_H7_CR_PSIZE 4
#define STM32_FLASH_H7_CR_START(chipid) (chipid == STM32_CHIPID_H7Ax ? 5 : 7)
#define STM32_FLASH_H7_CR_SNB 8
#define STM32_FLASH_H7_CR_SNB_MASK 0x700

// H7 Flash status register
#define STM32_FLASH_H7_SR_QW 2
#define STM32_FLASH_H7_SR_WRPERR 17
#define STM32_FLASH_H7_SR_PGSERR 18
#define STM32_FLASH_H7_SR_STRBERR 19
#define STM32_FLASH_H7_SR_ERROR_MASK                              \
  ((1 << STM32_FLASH_H7_SR_PGSERR) | (1 << STM32_FLASH_H7_SR_STRBERR) | \
   (1 << STM32_FLASH_H7_SR_WRPERR))

// == STM32L0/L1/L4/L5 ==
// Lx Flash registers
#define STM32_FLASH_Lx_REGS_ADDR ((uint32_t) 0x40023c00)
#define STM32_FLASH_Lx_ACR (STM32_FLASH_Lx_REGS_ADDR + 0x00)
#define STM32_FLASH_Lx_PECR (STM32_FLASH_Lx_REGS_ADDR + 0x04)
#define STM32_FLASH_Lx_PDKEYR (STM32_FLASH_Lx_REGS_ADDR + 0x08)
#define STM32_FLASH_Lx_PEKEYR (STM32_FLASH_Lx_REGS_ADDR + 0x0c)
#define STM32_FLASH_Lx_PRGKEYR (STM32_FLASH_Lx_REGS_ADDR + 0x10)
#define STM32_FLASH_Lx_OPTKEYR (STM32_FLASH_Lx_REGS_ADDR + 0x14)
#define STM32_FLASH_Lx_SR (STM32_FLASH_Lx_REGS_ADDR + 0x18)
#define STM32_FLASH_Lx_OBR (STM32_FLASH_Lx_REGS_ADDR + 0x1c)
#define STM32_FLASH_Lx_WRPR (STM32_FLASH_Lx_REGS_ADDR + 0x20)

// == STM32L0 ==
// L0 Flash registers
#define STM32_FLASH_L0_REGS_ADDR ((uint32_t) 0x40022000)

#define STM32_FLASH_L0_PEKEY1 0x89abcdef
#define STM32_FLASH_L0_PEKEY2 0x02030405

#define STM32_FLASH_L0_PRGKEY1 0x8c9daebf
#define STM32_FLASH_L0_PRGKEY2 0x13141516

#define STM32_FLASH_L0_OPTKEY1 0xFBEAD9C8
#define STM32_FLASH_L0_OPTKEY2 0x24252627

#define STM32_FLASH_L0_PELOCK (0)
#define STM32_FLASH_L0_OPTLOCK (2)
#define STM32_FLASH_L0_OBL_LAUNCH (18)

// L0 Flash status register
#define STM32_FLASH_L0_SR_ERROR_MASK 0x00013f00
#define STM32_FLASH_L0_SR_WRPERR 8
#define STM32_FLASH_L0_SR_PGAERR 9
#define STM32_FLASH_L0_SR_NOTZEROERR 16

// == STM32L1 ==
// L1 Flash registers
#define STM32_FLASH_L1_FPRG 10
#define STM32_FLASH_L1_PROG 3

// L1 Flash status register
#define STM32_FLASH_L1_SR_ERROR_MASK 0x00003f00
#define STM32_FLASH_L1_SR_WRPERR 8
#define STM32_FLASH_L1_SR_PGAERR 9

// == STM32L4 ==
// L4 Flash registers
// L4 register base is at STM32_FLASH_L0_REGS_ADDR (0x40022000)
#define STM32_FLASH_L4_KEYR (STM32_FLASH_L0_REGS_ADDR + 0x08)
#define STM32_FLASH_L4_OPTKEYR (STM32_FLASH_L0_REGS_ADDR + 0x0c)
#define STM32_FLASH_L4_SR (STM32_FLASH_L0_REGS_ADDR + 0x10)
#define STM32_FLASH_L4_CR (STM32_FLASH_L0_REGS_ADDR + 0x14)
#define STM32_FLASH_L4_OPTR (STM32_FLASH_L0_REGS_ADDR + 0x20)

// L4 Flash status register
#define STM32_FLASH_L4_SR_ERROR_MASK 0x3f8 // SR [9:3]
#define STM32_FLASH_L4_SR_PROGERR 3
#define STM32_FLASH_L4_SR_WRPERR 4
#define STM32_FLASH_L4_SR_PGAERR 5
#define STM32_FLASH_L4_SR_BSY 16

// L4 Flash control register
#define STM32_FLASH_L4_CR_LOCK 31         /* Lock control register */
#define STM32_FLASH_L4_CR_OPTLOCK 30      /* Lock option bytes */
#define STM32_FLASH_L4_CR_PG 0            /* Program */
#define STM32_FLASH_L4_CR_PER 1           /* Page erase */
#define STM32_FLASH_L4_CR_MER1 2          /* Bank 1 erase */
#define STM32_FLASH_L4_CR_MER2 15         /* Bank 2 erase */
#define STM32_FLASH_L4_CR_STRT 16         /* Start command */
#define STM32_FLASH_L4_CR_OPTSTRT 17      /* Start writing option bytes */
#define STM32_FLASH_L4_CR_BKER 11         /* Bank select for page erase */
#define STM32_FLASH_L4_CR_PNB 3           /* Page number (8 bits) */
#define STM32_FLASH_L4_CR_OBL_LAUNCH 27   /* Option bytes reload */
// Bits requesting flash operations (useful when we want to clear them)
#define STM32_FLASH_L4_CR_OPBITS                                        \
  (uint32_t) ((1lu << STM32_FLASH_L4_CR_PG) | (1lu << STM32_FLASH_L4_CR_PER) | \
             (1lu << STM32_FLASH_L4_CR_MER1) | (1lu << STM32_FLASH_L4_CR_MER1))
// Page is fully specified by BKER and PNB
#define STM32_FLASH_L4_CR_PAGEMASK (uint32_t) (0x1fflu << STM32_FLASH_L4_CR_PNB)

#define STM32_FLASH_L4_OPTR_DUALBANK 21

// == STM32L5 == (RM0438, p.241)
// L5 Flash registers
#define STM32_FLASH_L5_REGS_ADDR ((uint32_t) 0x40022000)
#define STM32_FLASH_L5_ACR (STM32_FLASH_L5_REGS_ADDR + 0x00)
#define STM32_FLASH_L5_NSKEYR (STM32_FLASH_L5_REGS_ADDR + 0x08)
#define STM32_FLASH_L5_OPTKEYR (STM32_FLASH_L5_REGS_ADDR + 0x10)
#define STM32_FLASH_L5_NSSR (STM32_FLASH_L5_REGS_ADDR + 0x20)
#define STM32_FLASH_L5_NSCR (STM32_FLASH_L5_REGS_ADDR + 0x28)
#define STM32_FLASH_L5_ECCR (STM32_FLASH_L5_REGS_ADDR + 0x30)
#define STM32_FLASH_L5_OPTR (STM32_FLASH_L5_REGS_ADDR + 0x40)

// FLASH_NSCR control registers (RM0438, p. 242)
#define STM32_FLASH_L5_NSCR_NSPG 0        /* Program */
#define STM32_FLASH_L5_NSCR_NSPER 1       /* Page erase */
#define STM32_FLASH_L5_NSCR_NSMER1 2      /* Bank 1 erase */
#define STM32_FLASH_L5_NSCR_NSPNB 3       /* Page number (7 bits) */
#define STM32_FLASH_L5_NSCR_NSBKER 11     /* Bank select for page erase */
#define STM32_FLASH_L5_NSCR_NSMER2 15     /* Bank 2 erase */
#define STM32_FLASH_L5_NSCR_NSSTRT 16     /* Start command */
#define STM32_FLASH_L5_NSCR_NSOPTSTRT 17  /* Start writing option bytes */
#define STM32_FLASH_L5_NSCR_NSEOPIE 24
#define STM32_FLASH_L5_NSCR_NSERRIE 25
#define STM32_FLASH_L5_NSCR_OBL_LAUNCH 27 /* Option bytes reload */
#define STM32_FLASH_L5_NSCR_OPTLOCK 30    /* Lock option bytes */
#define STM32_FLASH_L5_NSCR_NSLOCK 31     /* Lock control register */

// FLASH_NSSR status register (RM0438, p. 241)
#define STM32_FLASH_L5_NSSR_NSEOP 0       /* End of Operation */
#define STM32_FLASH_L5_NSSR_NSOPERR 1
#define STM32_FLASH_L5_NSSR_NSPROGERR 3
#define STM32_FLASH_L5_NSSR_NSWRPERR 4
#define STM32_FLASH_L5_NSSR_NSPGAERR 5
#define STM32_FLASH_L5_NSSR_NSSIZERR 6
#define STM32_FLASH_L5_NSSR_NSPGSERR 7
#define STM32_FLASH_L5_NSSR_OPTWERR 12
#define STM32_FLASH_L5_NSSR_BSY 16        /* Busy */
#define STM32_FLASH_L5_NSSR_ERROR_MASK (0x20fa)

// == STM32H5 == (RM0481)
// H5 non-secure flash registers. Bit positions and the dedicated clear-control
// register (NSCCR) differ from L5/U5, so H5 has its own flash type.
#define STM32_FLASH_H5_REGS_ADDR ((uint32_t) 0x40022000)
#define STM32_FLASH_H5_ACR    (STM32_FLASH_H5_REGS_ADDR + 0x00)
#define STM32_FLASH_H5_NSKEYR (STM32_FLASH_H5_REGS_ADDR + 0x04)
#define STM32_FLASH_H5_OPTKEYR (STM32_FLASH_H5_REGS_ADDR + 0x10)
#define STM32_FLASH_H5_NSSR   (STM32_FLASH_H5_REGS_ADDR + 0x20)
#define STM32_FLASH_H5_NSCR   (STM32_FLASH_H5_REGS_ADDR + 0x28)
#define STM32_FLASH_H5_NSCCR  (STM32_FLASH_H5_REGS_ADDR + 0x30)
#define STM32_FLASH_H5_OPTSR_CUR (STM32_FLASH_H5_REGS_ADDR + 0x50)

// FLASH_OPTSR_CUR: SWAP_BANK reports whether the two physical banks are mapped
// swapped in the address space (RM0481).
#define STM32_FLASH_H5_OPTSR_SWAP_BANK 31

// FLASH_NSCR control bits (RM0481)
#define STM32_FLASH_H5_NSCR_LOCK 0
#define STM32_FLASH_H5_NSCR_PG   1   /* Program */
#define STM32_FLASH_H5_NSCR_SER  2   /* Sector erase */
#define STM32_FLASH_H5_NSCR_BER  3   /* Bank erase */
#define STM32_FLASH_H5_NSCR_STRT 5   /* Start */
#define STM32_FLASH_H5_NSCR_SNB  6   /* Sector number [12:6] (7 bits) */
#define STM32_FLASH_H5_NSCR_SNB_MASK (0x7Fu << STM32_FLASH_H5_NSCR_SNB)
#define STM32_FLASH_H5_NSCR_MER  15  /* Mass erase */
#define STM32_FLASH_H5_NSCR_BKSEL 31 /* Bank selector */

// FLASH_NSSR status bits (RM0481)
#define STM32_FLASH_H5_NSSR_BSY  0   /* Busy */
#define STM32_FLASH_H5_NSSR_EOP  16  /* End of operation */
#define STM32_FLASH_H5_NSSR_WRPERR 17
#define STM32_FLASH_H5_NSSR_PGSERR 18
#define STM32_FLASH_H5_NSSR_STRBERR 19
#define STM32_FLASH_H5_NSSR_INCERR 20
#define STM32_FLASH_H5_NSSR_OPTCHANGEERR 23
#define STM32_FLASH_H5_NSSR_ERROR_MASK (0x009E0000) /* WRPERR|PGSERR|STRBERR|INCERR|OPTCHANGEERR */

// FLASH_NSCCR: write 1 to clear the matching NSSR flag (bits 16..23)
#define STM32_FLASH_H5_NSCCR_CLEAR_ALL (0x00FF0000)

// == STM32WB0 == (RM0530)
// WB0 FLASH registers
#define STM32_FLASH_WB0_REGS_ADDR ((uint32_t) 0x40001000)
#define STM32_FLASH_WB0_COMMAND (STM32_FLASH_WB0_REGS_ADDR + 0x00)
#define STM32_FLASH_WB0_CONFIG (STM32_FLASH_WB0_REGS_ADDR + 0x04)
#define STM32_FLASH_WB0_IRQSTAT (STM32_FLASH_WB0_REGS_ADDR + 0x08)
#define STM32_FLASH_WB0_IRQMASK (STM32_FLASH_WB0_REGS_ADDR + 0x0c)
#define STM32_FLASH_WB0_IRQRAW (STM32_FLASH_WB0_REGS_ADDR + 0x10)
#define STM32_FLASH_WB0_FLASH_SIZE (STM32_FLASH_WB0_REGS_ADDR + 0x14)
#define STM32_FLASH_WB0_ADDRESS (STM32_FLASH_WB0_REGS_ADDR + 0x18)
#define STM32_FLASH_WB0_LFSRVAL (STM32_FLASH_WB0_REGS_ADDR + 0x24)
#define STM32_FLASH_WB0_PAGEPROT0 (STM32_FLASH_WB0_REGS_ADDR + 0x34)
#define STM32_FLASH_WB0_PAGEPROT1 (STM32_FLASH_WB0_REGS_ADDR + 0x38)
#define STM32_FLASH_WB0_DATA0 (STM32_FLASH_WB0_REGS_ADDR + 0x40)
#define STM32_FLASH_WB0_DATA1 (STM32_FLASH_WB0_REGS_ADDR + 0x44)
#define STM32_FLASH_WB0_DATA2 (STM32_FLASH_WB0_REGS_ADDR + 0x48)
#define STM32_FLASH_WB0_DATA3 (STM32_FLASH_WB0_REGS_ADDR + 0x4c)

// WB0 FLASH commands
#define STM32_FLASH_WB0_CMD_ERASE_PAGE (0x11)
#define STM32_FLASH_WB0_CMD_MASS_ERASE (0x22) //WB06,WB07,WB09 only
#define STM32_FLASH_WB0_CMD_WRITE (0x33)
#define STM32_FLASH_WB0_CMD_MASSREAD (0x55)
#define STM32_FLASH_WB0_CMD_BURSTWRITE (0xCC)
#define STM32_FLASH_WB0_CMD_OTPWRITE (0xEE)
#define STM32_FLASH_WB0_CMD_KEYWRITE (0xFF)

// FLASH_SIZE register bits
#define STM32_FLASH_WB0_RAM_SIZE  (17)
#define STM32_FLASH_WB0_FLASH_SECURE (19)
#define STM32_FLASH_WB0_SWD_DISABLE  (20)

// FLASH IRQ (status) bits
#define STM32_FLASH_WB0_IRQ_ILLCMD (1 << 3)
#define STM32_FLASH_WB0_IRQ_CMDERR (1 << 2)
#define STM32_FLASH_WB0_IRQ_CMDSTART (1 << 1)
#define STM32_FLASH_WB0_IRQ_CMDDONE (1 << 0)
#define STM32_FLASH_WB0_IRQ_ERR_MASK (STM32_FLASH_WB0_IRQ_CMDERR | STM32_FLASH_WB0_IRQ_ILLCMD)
#define STM32_FLASH_WB0_IRQ_ALL (STM32_FLASH_WB0_IRQ_CMDDONE | STM32_FLASH_WB0_IRQ_CMDSTART | STM32_FLASH_WB0_IRQ_ERR_MASK)

// key values, to be written into DATA0-3
#define STM32_FLASH_WB0_KEY01_UNLOCK (0xFFFFFFFF)
#define STM32_FLASH_WB0_KEY01_READOUT_PROT (0xAAAAAAAA)
#define STM32_FLASH_WB0_KEY01_SWD_DISABLED (0xABACABAD)   /* irreversible protection level */
#define STM32_FLASH_WB0_KEY2 (0xC7EF584D)
#define STM32_FLASH_WB0_KEY3 (0xB3A21096)


// == STM32C5 == (RM0522)
// C5 Flash registers
// Base address: AHB1PERIPH_BASE(0x40020000) + 0x2000 = 0x40022000
#define STM32_FLASH_C5_REGS_ADDR ((uint32_t) 0x40022000)
#define STM32_FLASH_C5_KEYR (STM32_FLASH_C5_REGS_ADDR + 0x04)
#define STM32_FLASH_C5_OPTKEYR (STM32_FLASH_C5_REGS_ADDR + 0x0c)
#define STM32_FLASH_C5_SR (STM32_FLASH_C5_REGS_ADDR + 0x20)
#define STM32_FLASH_C5_CR (STM32_FLASH_C5_REGS_ADDR + 0x28)
#define STM32_FLASH_C5_CCR (STM32_FLASH_C5_REGS_ADDR + 0x30)
#define STM32_FLASH_C5_OPTSR_CUR (STM32_FLASH_C5_REGS_ADDR + 0x50)

// C5 Flash control register (FLASH_CR)
#define STM32_FLASH_C5_CR_LOCK 0
#define STM32_FLASH_C5_CR_PG 1
#define STM32_FLASH_C5_CR_PER 2
#define STM32_FLASH_C5_CR_BER 3
#define STM32_FLASH_C5_CR_STRT 5
#define STM32_FLASH_C5_CR_PNB 6
#define STM32_FLASH_C5_CR_MER 15
#define STM32_FLASH_C5_CR_BKSEL 31

// C5 Flash option status register
#define STM32_FLASH_C5_OPTSR_CUR_SINGLE_BANK 30
#define STM32_FLASH_C5_OPTSR_CUR_SWAP_BANK 31

// C5 Flash status register (FLASH_SR)
// SR is read-only; error flags are cleared by writing their clear bits to CCR.
#define STM32_FLASH_C5_SR_BSY 0
#define STM32_FLASH_C5_SR_WBNE 1
#define STM32_FLASH_C5_SR_DBNE 3
#define STM32_FLASH_C5_SR_EOP 16
#define STM32_FLASH_C5_SR_WRPERR 17
#define STM32_FLASH_C5_SR_PGSERR 18
#define STM32_FLASH_C5_SR_STRBERR 19
#define STM32_FLASH_C5_SR_INCERR 20
#define STM32_FLASH_C5_SR_ERROR_MASK                                      \
  ((1u << STM32_FLASH_C5_SR_WRPERR) | (1u << STM32_FLASH_C5_SR_PGSERR) | \
   (1u << STM32_FLASH_C5_SR_STRBERR) | (1u << STM32_FLASH_C5_SR_INCERR))


// == STM32WB == (RM0434)
// WB Flash registers
#define STM32_FLASH_WB_REGS_ADDR ((uint32_t) 0x58004000)
#define STM32_FLASH_WB_ACR (STM32_FLASH_WB_REGS_ADDR + 0x00)
#define STM32_FLASH_WB_KEYR (STM32_FLASH_WB_REGS_ADDR + 0x08)
#define STM32_FLASH_WB_OPT_KEYR (STM32_FLASH_WB_REGS_ADDR + 0x0c)
#define STM32_FLASH_WB_SR (STM32_FLASH_WB_REGS_ADDR + 0x10)
#define STM32_FLASH_WB_CR (STM32_FLASH_WB_REGS_ADDR + 0x14)
#define STM32_FLASH_WB_ECCR (STM32_FLASH_WB_REGS_ADDR + 0x18)
#define STM32_FLASH_WB_OPTR (STM32_FLASH_WB_REGS_ADDR + 0x20)
#define STM32_FLASH_WB_PCROP1ASR (STM32_FLASH_WB_REGS_ADDR + 0x24)
#define STM32_FLASH_WB_PCROP1AER (STM32_FLASH_WB_REGS_ADDR + 0x28)
#define STM32_FLASH_WB_WRP1AR (STM32_FLASH_WB_REGS_ADDR + 0x2c)
#define STM32_FLASH_WB_WRP1BR (STM32_FLASH_WB_REGS_ADDR + 0x30)
#define STM32_FLASH_WB_PCROP1BSR (STM32_FLASH_WB_REGS_ADDR + 0x34)
#define STM32_FLASH_WB_PCROP1BER (STM32_FLASH_WB_REGS_ADDR + 0x38)
#define STM32_FLASH_WB_IPCCBR (STM32_FLASH_WB_REGS_ADDR + 0x3c)
#define STM32_FLASH_WB_C2ACR (STM32_FLASH_WB_REGS_ADDR + 0x5c)
#define STM32_FLASH_WB_C2SR (STM32_FLASH_WB_REGS_ADDR + 0x60)
#define STM32_FLASH_WB_C2CR (STM32_FLASH_WB_REGS_ADDR + 0x64)
#define STM32_FLASH_WB_SFR (STM32_FLASH_WB_REGS_ADDR + 0x80)
#define STM32_FLASH_WB_SRRVR (STM32_FLASH_WB_REGS_ADDR + 0x84)

// WB Flash control register
#define STM32_FLASH_WB_CR_STRT (16)       /* Start */
#define STM32_FLASH_WB_CR_OPTSTRT (17)    /* Start writing option bytes */
#define STM32_FLASH_WB_CR_OBL_LAUNCH (27) /* Forces the option byte loading */
#define STM32_FLASH_WB_CR_OPTLOCK (30)    /* Option Lock */
#define STM32_FLASH_WB_CR_LOCK (31)       /* Lock */

// WB Flash status register
#define STM32_FLASH_WB_SR_ERROR_MASK (0x3f8) // SR [9:3]
#define STM32_FLASH_WB_SR_PROGERR (3)     /* Programming alignment error */
#define STM32_FLASH_WB_SR_WRPERR (4)      /* Write protection error */
#define STM32_FLASH_WB_SR_PGAERR (5)      /* Programming error */
#define STM32_FLASH_WB_SR_BSY (16)        /* Busy */

#endif // STM32_FLASH_H
