#define DEBUG_FLASH 0
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <helper.h>
#include <logging.h>
#include <md5.h>
#include <stlink.h>

#ifdef STLINK_HAVE_SYS_MMAN_H
#include <sys/mman.h>
#else
#include <mmap.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef _MSC_VER
#define __attribute__(x)
#endif

#define BANK_1 0
#define BANK_2 1

/* stm32f FPEC flash controller interface, pm0063 manual */
// TODO - all of this needs to be abstracted out....
// STM32F05x is identical, based on RM0091 (DM00031936, Doc ID 018940 Rev 2,
// August 2012)
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

// STM32F10x_XL has two flash memory banks with separate registers to control
// the second bank.
#define FLASH_KEYR2 (FLASH_REGS_ADDR + 0x44)
#define FLASH_SR2 (FLASH_REGS_ADDR + 0x4c)
#define FLASH_CR2 (FLASH_REGS_ADDR + 0x50)
#define FLASH_AR2 (FLASH_REGS_ADDR + 0x54)

// For STM32F05x, the RDPTR_KEY may be wrong, but as it is not used anywhere...
#define FLASH_RDPTR_KEY 0x00a5
#define FLASH_KEY1 0x45670123
#define FLASH_KEY2 0xcdef89ab

#define FLASH_L0_PRGKEY1 0x8c9daebf
#define FLASH_L0_PRGKEY2 0x13141516

#define FLASH_L0_PEKEY1 0x89abcdef
#define FLASH_L0_PEKEY2 0x02030405

#define FLASH_OPTKEY1 0x08192A3B
#define FLASH_OPTKEY2 0x4C5D6E7F

#define FLASH_F0_OPTKEY1 0x45670123
#define FLASH_F0_OPTKEY2 0xCDEF89AB

#define FLASH_L0_OPTKEY1 0xFBEAD9C8
#define FLASH_L0_OPTKEY2 0x24252627

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

// Flash registers common to STM32G0 and STM32G4 series.
#define STM32Gx_FLASH_REGS_ADDR ((uint32_t)0x40022000)
#define STM32Gx_FLASH_ACR (STM32Gx_FLASH_REGS_ADDR + 0x00)
#define STM32Gx_FLASH_KEYR (STM32Gx_FLASH_REGS_ADDR + 0x08)
#define STM32Gx_FLASH_OPTKEYR (STM32Gx_FLASH_REGS_ADDR + 0x0c)
#define STM32Gx_FLASH_SR (STM32Gx_FLASH_REGS_ADDR + 0x10)
#define STM32Gx_FLASH_CR (STM32Gx_FLASH_REGS_ADDR + 0x14)
#define STM32Gx_FLASH_ECCR (STM32Gx_FLASH_REGS_ADDR + 0x18)
#define STM32Gx_FLASH_OPTR (STM32Gx_FLASH_REGS_ADDR + 0x20)

// G0 (RM0444 Table 1, sec 3.7)
// Mostly the same as G4 chips, but the notation
// varies a bit after the 'OPTR' register.
#define STM32G0_FLASH_REGS_ADDR (STM32Gx_FLASH_REGS_ADDR)
#define STM32G0_FLASH_PCROP1ASR (STM32G0_FLASH_REGS_ADDR + 0x24)
#define STM32G0_FLASH_PCROP1AER (STM32G0_FLASH_REGS_ADDR + 0x28)
#define STM32G0_FLASH_WRP1AR (STM32G0_FLASH_REGS_ADDR + 0x2C)
#define STM32G0_FLASH_WRP1BR (STM32G0_FLASH_REGS_ADDR + 0x30)
#define STM32G0_FLASH_PCROP1BSR (STM32G0_FLASH_REGS_ADDR + 0x34)
#define STM32G0_FLASH_PCROP1BER (STM32G0_FLASH_REGS_ADDR + 0x38)
#define STM32G0_FLASH_SECR (STM32G0_FLASH_REGS_ADDR + 0x80)

// G4 (RM0440 Table 17, sec 3.7.19)
// Mostly the same as STM32G0 chips, but there are a few extra
// registers because 'cat 3' devices can have two Flash banks.
#define STM32G4_FLASH_REGS_ADDR (STM32Gx_FLASH_REGS_ADDR)
#define STM32G4_FLASH_PDKEYR (STM32G4_FLASH_REGS_ADDR + 0x04)
#define STM32G4_FLASH_PCROP1SR (STM32G4_FLASH_REGS_ADDR + 0x24)
#define STM32G4_FLASH_PCROP1ER (STM32G4_FLASH_REGS_ADDR + 0x28)
#define STM32G4_FLASH_WRP1AR (STM32G4_FLASH_REGS_ADDR + 0x2C)
#define STM32G4_FLASH_WRP1BR (STM32G4_FLASH_REGS_ADDR + 0x30)
#define STM32G4_FLASH_PCROP2SR (STM32G4_FLASH_REGS_ADDR + 0x44)
#define STM32G4_FLASH_PCROP2ER (STM32G4_FLASH_REGS_ADDR + 0x48)
#define STM32G4_FLASH_WRP2AR (STM32G4_FLASH_REGS_ADDR + 0x4C)
#define STM32G4_FLASH_WRP2BR (STM32G4_FLASH_REGS_ADDR + 0x50)
#define STM32G4_FLASH_SEC1R (STM32G4_FLASH_REGS_ADDR + 0x70)
#define STM32G4_FLASH_SEC2R (STM32G4_FLASH_REGS_ADDR + 0x74)

// G0/G4 FLASH control register
#define STM32Gx_FLASH_CR_PG (0)      /* Program */
#define STM32Gx_FLASH_CR_PER (1)     /* Page erase */
#define STM32Gx_FLASH_CR_MER1 (2)    /* Mass erase */
#define STM32Gx_FLASH_CR_PNB (3)     /* Page number */
#define STM32G0_FLASH_CR_PNG_LEN (5) /* STM32G0: 5 page number bits */
#define STM32G4_FLASH_CR_PNG_LEN (7) /* STM32G4: 7 page number bits */
#define STM32Gx_FLASH_CR_MER2 (15)   /* Mass erase (2nd bank)*/
#define STM32Gx_FLASH_CR_STRT (16)   /* Start */
#define STM32Gx_FLASH_CR_OPTSTRT                                               \
  (17)                              /* Start of modification of option bytes */
#define STM32Gx_FLASH_CR_FSTPG (18) /* Fast programming */
#define STM32Gx_FLASH_CR_EOPIE (24) /* End of operation interrupt enable */
#define STM32Gx_FLASH_CR_ERRIE (25) /* Error interrupt enable */
#define STM32Gx_FLASH_CR_OBL_LAUNCH (27) /* Forces the option byte loading */
#define STM32Gx_FLASH_CR_OPTLOCK (30)    /* Options Lock */
#define STM32Gx_FLASH_CR_LOCK (31)       /* FLASH_CR Lock */

// G0/G4 FLASH status register
#define STM32Gx_FLASH_SR_ERROR_MASK (0x3fa)
#define STM32Gx_FLASH_SR_PROGERR (3)
#define STM32Gx_FLASH_SR_WRPERR (4)
#define STM32Gx_FLASH_SR_PGAERR (5)
#define STM32Gx_FLASH_SR_BSY (16) /* FLASH_SR Busy */
#define STM32Gx_FLASH_SR_EOP (0)  /* FLASH_EOP End of Operation */

// G4 FLASH option register
#define STM32G4_FLASH_OPTR_DBANK (22) /* FLASH_OPTR Dual Bank Mode */

// WB (RM0434)
#define STM32WB_FLASH_REGS_ADDR ((uint32_t)0x58004000)
#define STM32WB_FLASH_ACR (STM32WB_FLASH_REGS_ADDR + 0x00)
#define STM32WB_FLASH_KEYR (STM32WB_FLASH_REGS_ADDR + 0x08)
#define STM32WB_FLASH_OPT_KEYR (STM32WB_FLASH_REGS_ADDR + 0x0C)
#define STM32WB_FLASH_SR (STM32WB_FLASH_REGS_ADDR + 0x10)
#define STM32WB_FLASH_CR (STM32WB_FLASH_REGS_ADDR + 0x14)
#define STM32WB_FLASH_ECCR (STM32WB_FLASH_REGS_ADDR + 0x18)
#define STM32WB_FLASH_OPTR (STM32WB_FLASH_REGS_ADDR + 0x20)
#define STM32WB_FLASH_PCROP1ASR (STM32WB_FLASH_REGS_ADDR + 0x24)
#define STM32WB_FLASH_PCROP1AER (STM32WB_FLASH_REGS_ADDR + 0x28)
#define STM32WB_FLASH_WRP1AR (STM32WB_FLASH_REGS_ADDR + 0x2C)
#define STM32WB_FLASH_WRP1BR (STM32WB_FLASH_REGS_ADDR + 0x30)
#define STM32WB_FLASH_PCROP1BSR (STM32WB_FLASH_REGS_ADDR + 0x34)
#define STM32WB_FLASH_PCROP1BER (STM32WB_FLASH_REGS_ADDR + 0x38)
#define STM32WB_FLASH_IPCCBR (STM32WB_FLASH_REGS_ADDR + 0x3C)
#define STM32WB_FLASH_C2ACR (STM32WB_FLASH_REGS_ADDR + 0x5C)
#define STM32WB_FLASH_C2SR (STM32WB_FLASH_REGS_ADDR + 0x60)
#define STM32WB_FLASH_C2CR (STM32WB_FLASH_REGS_ADDR + 0x64)
#define STM32WB_FLASH_SFR (STM32WB_FLASH_REGS_ADDR + 0x80)
#define STM32WB_FLASH_SRRVR (STM32WB_FLASH_REGS_ADDR + 0x84)

// WB Flash control register.
#define STM32WB_FLASH_CR_STRT (16)    /* Start */
#define STM32WB_FLASH_CR_OPTLOCK (30) /* Option Lock */
#define STM32WB_FLASH_CR_LOCK (31)    /* Lock */
// WB Flash status register.
#define STM32WB_FLASH_SR_ERROR_MASK (0x3f8) /* SR [9:3] */
#define STM32WB_FLASH_SR_PROGERR (3)        /* Programming alignment error */
#define STM32WB_FLASH_SR_WRPERR (4)         /* Write protection error */
#define STM32WB_FLASH_SR_PGAERR (5)         /* Programming error */
#define STM32WB_FLASH_SR_BSY (16)           /* Busy */

// 32L4 register base is at FLASH_REGS_ADDR (0x40022000)
#define STM32L4_FLASH_KEYR (FLASH_REGS_ADDR + 0x08)
#define STM32L4_FLASH_OPTKEYR (FLASH_REGS_ADDR + 0x0C)
#define STM32L4_FLASH_SR (FLASH_REGS_ADDR + 0x10)
#define STM32L4_FLASH_CR (FLASH_REGS_ADDR + 0x14)
#define STM32L4_FLASH_OPTR (FLASH_REGS_ADDR + 0x20)

#define STM32L4_FLASH_SR_ERROR_MASK 0x3f8 /* SR [9:3] */
#define STM32L4_FLASH_SR_PROGERR 3
#define STM32L4_FLASH_SR_WRPERR 4
#define STM32L4_FLASH_SR_PGAERR 5
#define STM32L4_FLASH_SR_BSY 16

#define STM32L4_FLASH_CR_LOCK 31       /* Lock control register */
#define STM32L4_FLASH_CR_OPTLOCK 30    /* Lock option bytes */
#define STM32L4_FLASH_CR_PG 0          /* Program */
#define STM32L4_FLASH_CR_PER 1         /* Page erase */
#define STM32L4_FLASH_CR_MER1 2        /* Bank 1 erase */
#define STM32L4_FLASH_CR_MER2 15       /* Bank 2 erase */
#define STM32L4_FLASH_CR_STRT 16       /* Start command */
#define STM32L4_FLASH_CR_OPTSTRT 17    /* Start writing option bytes */
#define STM32L4_FLASH_CR_BKER 11       /* Bank select for page erase */
#define STM32L4_FLASH_CR_PNB 3         /* Page number (8 bits) */
#define STM32L4_FLASH_CR_OBL_LAUNCH 27 /* Option bytes reload */
// Bits requesting flash operations (useful when we want to clear them)
#define STM32L4_FLASH_CR_OPBITS                                                \
  (uint32_t)((1lu << STM32L4_FLASH_CR_PG) | (1lu << STM32L4_FLASH_CR_PER) |    \
             (1lu << STM32L4_FLASH_CR_MER1) | (1lu << STM32L4_FLASH_CR_MER1))
// Page is fully specified by BKER and PNB
#define STM32L4_FLASH_CR_PAGEMASK (uint32_t)(0x1fflu << STM32L4_FLASH_CR_PNB)

#define STM32L4_FLASH_OPTR_DUALBANK 21

// STM32L0x flash register base and offsets RM0090 - DM00031020.pdf
#define STM32L0_FLASH_REGS_ADDR ((uint32_t)0x40022000)

#define STM32L0_FLASH_PELOCK (0)
#define STM32L0_FLASH_OPTLOCK (2)
#define STM32L0_FLASH_OBL_LAUNCH (18)

#define STM32L0_FLASH_SR_ERROR_MASK 0x00013F00
#define STM32L0_FLASH_SR_WRPERR 8
#define STM32L0_FLASH_SR_PGAERR 9
#define STM32L0_FLASH_SR_NOTZEROERR 16

#define FLASH_ACR_OFF ((uint32_t)0x00)
#define FLASH_PECR_OFF ((uint32_t)0x04)
#define FLASH_PDKEYR_OFF ((uint32_t)0x08)
#define FLASH_PEKEYR_OFF ((uint32_t)0x0c)
#define FLASH_PRGKEYR_OFF ((uint32_t)0x10)
#define FLASH_OPTKEYR_OFF ((uint32_t)0x14)
#define FLASH_SR_OFF ((uint32_t)0x18)
#define FLASH_OBR_OFF ((uint32_t)0x1c)
#define FLASH_WRPR_OFF ((uint32_t)0x20)

// STM32F7
#define FLASH_F7_REGS_ADDR ((uint32_t)0x40023c00)
#define FLASH_F7_KEYR (FLASH_F7_REGS_ADDR + 0x04)
#define FLASH_F7_OPT_KEYR (FLASH_F7_REGS_ADDR + 0x08)
#define FLASH_F7_SR (FLASH_F7_REGS_ADDR + 0x0c)
#define FLASH_F7_CR (FLASH_F7_REGS_ADDR + 0x10)
#define FLASH_F7_OPTCR (FLASH_F7_REGS_ADDR + 0x14)
#define FLASH_F7_OPTCR1 (FLASH_F7_REGS_ADDR + 0x18)
#define FLASH_F7_OPTCR_LOCK 0
#define FLASH_F7_OPTCR_START 1
#define FLASH_F7_CR_STRT 16
#define FLASH_F7_CR_LOCK 31
#define FLASH_F7_CR_SER 1
#define FLASH_F7_CR_SNB 3
#define FLASH_F7_CR_SNB_MASK 0xf8
#define FLASH_F7_SR_BSY 16
#define FLASH_F7_SR_ERS_ERR 7 /* Erase Sequence Error */
#define FLASH_F7_SR_PGP_ERR 6 /* Programming parallelism error */
#define FLASH_F7_SR_PGA_ERR 5 /* Programming alignment error */
#define FLASH_F7_SR_WRP_ERR 4 /* Write protection error */
#define FLASH_F7_SR_OP_ERR 1  /* Operation error */
#define FLASH_F7_SR_EOP 0     /* End of operation */
#define FLASH_F7_OPTCR1_BOOT_ADD0 0
#define FLASH_F7_OPTCR1_BOOT_ADD1 16

#define FLASH_F7_SR_ERROR_MASK                                                 \
  ((1 << FLASH_F7_SR_ERS_ERR) | (1 << FLASH_F7_SR_PGP_ERR) |                   \
   (1 << FLASH_F7_SR_PGA_ERR) | (1 << FLASH_F7_SR_WRP_ERR) |                   \
   (1 << FLASH_F7_SR_OP_ERR))

// STM32F4
#define FLASH_F4_REGS_ADDR ((uint32_t)0x40023c00)
#define FLASH_F4_KEYR (FLASH_F4_REGS_ADDR + 0x04)
#define FLASH_F4_OPT_KEYR (FLASH_F4_REGS_ADDR + 0x08)
#define FLASH_F4_SR (FLASH_F4_REGS_ADDR + 0x0c)
#define FLASH_F4_CR (FLASH_F4_REGS_ADDR + 0x10)
#define FLASH_F4_OPTCR (FLASH_F4_REGS_ADDR + 0x14)
#define FLASH_F4_OPTCR_LOCK 0
#define FLASH_F4_OPTCR_START 1
#define FLASH_F4_CR_STRT 16
#define FLASH_F4_CR_LOCK 31
#define FLASH_F4_CR_SER 1
#define FLASH_F4_CR_SNB 3
#define FLASH_F4_CR_SNB_MASK 0xf8
#define FLASH_F4_SR_ERROR_MASK 0x000000F0
#define FLASH_F4_SR_PGAERR 5
#define FLASH_F4_SR_WRPERR 4
#define FLASH_F4_SR_BSY 16

// STM32F2
#define FLASH_F2_REGS_ADDR ((uint32_t)0x40023c00)
#define FLASH_F2_KEYR (FLASH_F2_REGS_ADDR + 0x04)
#define FLASH_F2_OPT_KEYR (FLASH_F2_REGS_ADDR + 0x08)
#define FLASH_F2_SR (FLASH_F2_REGS_ADDR + 0x0c)
#define FLASH_F2_CR (FLASH_F2_REGS_ADDR + 0x10)
#define FLASH_F2_OPT_CR (FLASH_F2_REGS_ADDR + 0x14)
#define FLASH_F2_OPT_LOCK_BIT (1u << 0)
#define FLASH_F2_CR_STRT 16
#define FLASH_F2_CR_LOCK 31

#define FLASH_F2_CR_SER 1
#define FLASH_F2_CR_SNB 3
#define FLASH_F2_CR_SNB_MASK 0x78
#define FLASH_F2_SR_BSY 16

// STM32H7xx
#define FLASH_H7_CR_LOCK 0
#define FLASH_H7_CR_PG 1
#define FLASH_H7_CR_SER 2
#define FLASH_H7_CR_BER 3
#define FLASH_H7_CR_PSIZE 4
#define FLASH_H7_CR_START(chipid) (chipid == STLINK_CHIPID_STM32_H7Ax ? 5 : 7)
#define FLASH_H7_CR_SNB 8
#define FLASH_H7_CR_SNB_MASK 0x700

#define FLASH_H7_SR_QW 2
#define FLASH_H7_SR_WRPERR 17
#define FLASH_H7_SR_PGSERR 18
#define FLASH_H7_SR_STRBERR 19
#define FLASH_H7_SR_ERROR_MASK                                                 \
  ((1 << FLASH_H7_SR_PGSERR) | (1 << FLASH_H7_SR_STRBERR) |                    \
   (1 << FLASH_H7_SR_WRPERR))

#define FLASH_H7_OPTCR_OPTLOCK 0
#define FLASH_H7_OPTCR_OPTSTART 1
#define FLASH_H7_OPTCR_MER 4

#define FLASH_H7_OPTSR_OPT_BUSY 0
#define FLASH_H7_OPTSR_OPTCHANGEERR 30

#define FLASH_H7_OPTCCR_CLR_OPTCHANGEERR 30

#define FLASH_H7_REGS_ADDR ((uint32_t)0x52002000)
#define FLASH_H7_KEYR1 (FLASH_H7_REGS_ADDR + 0x04)
#define FLASH_H7_KEYR2 (FLASH_H7_REGS_ADDR + 0x104)
#define FLASH_H7_OPT_KEYR (FLASH_H7_REGS_ADDR + 0x08)
#define FLASH_H7_OPT_KEYR2 (FLASH_H7_REGS_ADDR + 0x108)
#define FLASH_H7_CR1 (FLASH_H7_REGS_ADDR + 0x0c)
#define FLASH_H7_CR2 (FLASH_H7_REGS_ADDR + 0x10c)
#define FLASH_H7_SR1 (FLASH_H7_REGS_ADDR + 0x10)
#define FLASH_H7_SR2 (FLASH_H7_REGS_ADDR + 0x110)
#define FLASH_H7_CCR1 (FLASH_H7_REGS_ADDR + 0x14)
#define FLASH_H7_CCR2 (FLASH_H7_REGS_ADDR + 0x114)
#define FLASH_H7_OPTCR (FLASH_H7_REGS_ADDR + 0x18)
#define FLASH_H7_OPTCR2 (FLASH_H7_REGS_ADDR + 0x118)
#define FLASH_H7_OPTSR_CUR (FLASH_H7_REGS_ADDR + 0x1c)
#define FLASH_H7_OPTCCR (FLASH_H7_REGS_ADDR + 0x24)

#define STM32F0_DBGMCU_CR 0xE0042004
#define STM32F0_DBGMCU_CR_IWDG_STOP 8
#define STM32F0_DBGMCU_CR_WWDG_STOP 9

#define STM32F4_DBGMCU_APB1FZR1 0xE0042008
#define STM32F4_DBGMCU_APB1FZR1_WWDG_STOP 11
#define STM32F4_DBGMCU_APB1FZR1_IWDG_STOP 12

#define STM32L0_DBGMCU_APB1_FZ 0x40015808
#define STM32L0_DBGMCU_APB1_FZ_WWDG_STOP 11
#define STM32L0_DBGMCU_APB1_FZ_IWDG_STOP 12

#define STM32H7_DBGMCU_APB1HFZ 0x5C001054
#define STM32H7_DBGMCU_APB1HFZ_IWDG_STOP 18

#define STM32WB_DBGMCU_APB1FZR1 0xE004203C
#define STM32WB_DBGMCU_APB1FZR1_WWDG_STOP 11
#define STM32WB_DBGMCU_APB1FZR1_IWDG_STOP 12

#define STM32F1_RCC_AHBENR 0x40021014
#define STM32F1_RCC_DMAEN 0x00000003 // DMA2EN | DMA1EN

#define STM32F4_RCC_AHB1ENR 0x40023830
#define STM32F4_RCC_DMAEN 0x00600000 // DMA2EN | DMA1EN

#define STM32G0_RCC_AHBENR 0x40021038
#define STM32G0_RCC_DMAEN 0x00000003 // DMA2EN | DMA1EN

#define STM32G4_RCC_AHB1ENR 0x40021048
#define STM32G4_RCC_DMAEN 0x00000003 // DMA2EN | DMA1EN

#define STM32L0_RCC_AHBENR 0x40021030
#define STM32L0_RCC_DMAEN 0x00000001 // DMAEN

#define STM32H7_RCC_AHB1ENR 0x58024538
#define STM32H7_RCC_DMAEN 0x00000003 // DMA2EN | DMA1EN

#define STM32WB_RCC_AHB1ENR 0x58000048
#define STM32WB_RCC_DMAEN 0x00000003 // DMA2EN | DMA1EN

#define L1_WRITE_BLOCK_SIZE 0x80
#define L0_WRITE_BLOCK_SIZE 0x40

// Endianness
// https://commandcenter.blogspot.com/2012/04/byte-order-fallacy.html
// These functions encode and decode little endian uint16 and uint32 values.

void write_uint32(unsigned char *buf, uint32_t ui) {
  buf[0] = ui;
  buf[1] = ui >> 8;
  buf[2] = ui >> 16;
  buf[3] = ui >> 24;
}

void write_uint16(unsigned char *buf, uint16_t ui) {
  buf[0] = (uint8_t)ui;
  buf[1] = (uint8_t)(ui >> 8);
}

uint32_t read_uint32(const unsigned char *c, const int pt) {
  return ((uint32_t)c[pt]) | ((uint32_t)c[pt + 1] << 8) |
         ((uint32_t)c[pt + 2] << 16) | ((uint32_t)c[pt + 3] << 24);
}

uint16_t read_uint16(const unsigned char *c, const int pt) {
  return ((uint16_t)c[pt]) | ((uint16_t)c[pt + 1] << 8);
}

static uint32_t get_stm32l0_flash_base(stlink_t *sl) {
  switch (sl->chip_id) {
  case STLINK_CHIPID_STM32_L0:
  case STLINK_CHIPID_STM32_L0_CAT5:
  case STLINK_CHIPID_STM32_L0_CAT2:
  case STLINK_CHIPID_STM32_L011:
    return (STM32L0_FLASH_REGS_ADDR);

  case STLINK_CHIPID_STM32_L1_CAT2:
  case STLINK_CHIPID_STM32_L1_MD:
  case STLINK_CHIPID_STM32_L1_MD_PLUS:
  case STLINK_CHIPID_STM32_L1_MD_PLUS_HD:
    return (STM32L_FLASH_REGS_ADDR);

  default:
    WLOG("Flash base use default L0 address\n");
    return (STM32L0_FLASH_REGS_ADDR);
  }
}

static uint32_t __attribute__((unused)) read_flash_rdp(stlink_t *sl) {
  uint32_t rdp;
  stlink_read_debug32(sl, FLASH_WRPR, &rdp);
  return (rdp & 0xff);
}

static inline uint32_t read_flash_cr(stlink_t *sl, unsigned bank) {
  uint32_t reg, res;

  if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
    reg = FLASH_F4_CR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F7) {
    reg = FLASH_F7_CR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
    reg = STM32L4_FLASH_CR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
             sl->flash_type == STLINK_FLASH_TYPE_G4) {
    reg = STM32Gx_FLASH_CR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
    reg = STM32WB_FLASH_CR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
    reg = (bank == BANK_1) ? FLASH_H7_CR1 : FLASH_H7_CR2;
  } else {
    reg = (bank == BANK_1) ? FLASH_CR : FLASH_CR2;
  }

  stlink_read_debug32(sl, reg, &res);

#if DEBUG_FLASH
  fprintf(stdout, "CR:0x%x\n", res);
#endif
  return (res);
}

static inline unsigned int is_flash_locked(stlink_t *sl) {
  /* return non zero for true */
  uint32_t cr_lock_shift;
  uint32_t cr_reg;
  uint32_t n;

  if ((sl->flash_type == STLINK_FLASH_TYPE_F0) ||
      (sl->flash_type == STLINK_FLASH_TYPE_F1_XL)) {
    cr_reg = FLASH_CR;
    cr_lock_shift = FLASH_CR_LOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
    cr_reg = FLASH_F4_CR;
    cr_lock_shift = FLASH_F4_CR_LOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F7) {
    cr_reg = FLASH_F7_CR;
    cr_lock_shift = FLASH_F7_CR_LOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L0) {
    cr_reg = get_stm32l0_flash_base(sl) + FLASH_PECR_OFF;
    cr_lock_shift = STM32L0_FLASH_PELOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
    cr_reg = STM32L4_FLASH_CR;
    cr_lock_shift = STM32L4_FLASH_CR_LOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
             sl->flash_type == STLINK_FLASH_TYPE_G4) {
    cr_reg = STM32Gx_FLASH_CR;
    cr_lock_shift = STM32Gx_FLASH_CR_LOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
    cr_reg = STM32WB_FLASH_CR;
    cr_lock_shift = STM32WB_FLASH_CR_LOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
    cr_reg = FLASH_H7_CR1;
    cr_lock_shift = FLASH_H7_CR_LOCK;
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

  if (sl->flash_type == STLINK_FLASH_TYPE_F0) {
    key_reg = FLASH_KEYR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
    key_reg = FLASH_KEYR;
    key2_reg = FLASH_KEYR2;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
    key_reg = FLASH_F4_KEYR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F7) {
    key_reg = FLASH_F7_KEYR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L0) {
    key_reg = get_stm32l0_flash_base(sl) + FLASH_PEKEYR_OFF;
    flash_key1 = FLASH_L0_PEKEY1;
    flash_key2 = FLASH_L0_PEKEY2;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
    key_reg = STM32L4_FLASH_KEYR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
             sl->flash_type == STLINK_FLASH_TYPE_G4) {
    key_reg = STM32Gx_FLASH_KEYR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
    key_reg = STM32WB_FLASH_KEYR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
    key_reg = FLASH_H7_KEYR1;
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
      key2_reg = FLASH_H7_KEYR2;
    }
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
static int unlock_flash_if(stlink_t *sl) {
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

static void lock_flash(stlink_t *sl) {
  uint32_t cr_lock_shift, cr_reg, n, cr2_reg = 0;
  uint32_t cr_mask = 0xffffffffu;

  if (sl->flash_type == STLINK_FLASH_TYPE_F0) {
    cr_reg = FLASH_CR;
    cr_lock_shift = FLASH_CR_LOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
    cr_reg = FLASH_CR;
    cr2_reg = FLASH_CR2;
    cr_lock_shift = FLASH_CR_LOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
    cr_reg = FLASH_F4_CR;
    cr_lock_shift = FLASH_F4_CR_LOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F7) {
    cr_reg = FLASH_F7_CR;
    cr_lock_shift = FLASH_F7_CR_LOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L0) {
    cr_reg = get_stm32l0_flash_base(sl) + FLASH_PECR_OFF;
    cr_lock_shift = STM32L0_FLASH_PELOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
    cr_reg = STM32L4_FLASH_CR;
    cr_lock_shift = STM32L4_FLASH_CR_LOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
             sl->flash_type == STLINK_FLASH_TYPE_G4) {
    cr_reg = STM32Gx_FLASH_CR;
    cr_lock_shift = STM32Gx_FLASH_CR_LOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
    cr_reg = STM32WB_FLASH_CR;
    cr_lock_shift = STM32WB_FLASH_CR_LOCK;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
    cr_reg = FLASH_H7_CR1;
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
      cr2_reg = FLASH_H7_CR2;
    }
    cr_lock_shift = FLASH_H7_CR_LOCK;
    cr_mask = ~(1u << FLASH_H7_CR_SER);
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

static bool is_flash_option_locked(stlink_t *sl) {
  uint32_t optlock_shift, optcr_reg;
  int active_bit_level = 1;
  uint32_t n;

  switch (sl->flash_type) {
  case STLINK_FLASH_TYPE_F0:
  case STLINK_FLASH_TYPE_F1_XL:
    optcr_reg = FLASH_CR;
    optlock_shift = FLASH_CR_OPTWRE;
    active_bit_level = 0; /* bit is "option write enable", not lock */
    break;
  case STLINK_FLASH_TYPE_F4:
    optcr_reg = FLASH_F4_OPTCR;
    optlock_shift = FLASH_F4_OPTCR_LOCK;
    break;
  case STLINK_FLASH_TYPE_F7:
    optcr_reg = FLASH_F7_OPTCR;
    optlock_shift = FLASH_F7_OPTCR_LOCK;
    break;
  case STLINK_FLASH_TYPE_L0:
    optcr_reg = get_stm32l0_flash_base(sl) + FLASH_PECR_OFF;
    optlock_shift = STM32L0_FLASH_OPTLOCK;
    break;
  case STLINK_FLASH_TYPE_L4:
    optcr_reg = STM32L4_FLASH_CR;
    optlock_shift = STM32L4_FLASH_CR_OPTLOCK;
    break;
  case STLINK_FLASH_TYPE_G0:
  case STLINK_FLASH_TYPE_G4:
    optcr_reg = STM32Gx_FLASH_CR;
    optlock_shift = STM32Gx_FLASH_CR_OPTLOCK;
    break;
  case STLINK_FLASH_TYPE_WB:
    optcr_reg = STM32WB_FLASH_CR;
    optlock_shift = STM32WB_FLASH_CR_OPTLOCK;
    break;
  case STLINK_FLASH_TYPE_H7:
    optcr_reg = FLASH_H7_OPTCR;
    optlock_shift = FLASH_H7_OPTCR_OPTLOCK;
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

static int lock_flash_option(stlink_t *sl) {
  uint32_t optlock_shift, optcr_reg, n, optcr2_reg = 0;
  int active_bit_level = 1;

  switch (sl->flash_type) {
  case STLINK_FLASH_TYPE_F0:
  case STLINK_FLASH_TYPE_F1_XL:
    optcr_reg = FLASH_CR;
    optlock_shift = FLASH_CR_OPTWRE;
    active_bit_level = 0;
    break;
  case STLINK_FLASH_TYPE_F4:
    optcr_reg = FLASH_F4_OPTCR;
    optlock_shift = FLASH_F4_OPTCR_LOCK;
    break;
  case STLINK_FLASH_TYPE_F7:
    optcr_reg = FLASH_F7_OPTCR;
    optlock_shift = FLASH_F7_OPTCR_LOCK;
    break;
  case STLINK_FLASH_TYPE_L0:
    optcr_reg = get_stm32l0_flash_base(sl) + FLASH_PECR_OFF;
    optlock_shift = STM32L0_FLASH_OPTLOCK;
    break;
  case STLINK_FLASH_TYPE_L4:
    optcr_reg = STM32L4_FLASH_CR;
    optlock_shift = STM32L4_FLASH_CR_OPTLOCK;
    break;
  case STLINK_FLASH_TYPE_G0:
  case STLINK_FLASH_TYPE_G4:
    optcr_reg = STM32Gx_FLASH_CR;
    optlock_shift = STM32Gx_FLASH_CR_OPTLOCK;
    break;
  case STLINK_FLASH_TYPE_WB:
    optcr_reg = STM32WB_FLASH_CR;
    optlock_shift = STM32WB_FLASH_CR_OPTLOCK;
    break;
  case STLINK_FLASH_TYPE_H7:
    optcr_reg = FLASH_H7_OPTCR;
    optlock_shift = FLASH_H7_OPTCR_OPTLOCK;
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK)
      optcr2_reg = FLASH_H7_OPTCR2;
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

static int unlock_flash_option(stlink_t *sl) {
  uint32_t optkey_reg, optkey2_reg = 0;
  uint32_t optkey1 = FLASH_OPTKEY1;
  uint32_t optkey2 = FLASH_OPTKEY2;

  switch (sl->flash_type) {
  case STLINK_FLASH_TYPE_F0:
  case STLINK_FLASH_TYPE_F1_XL:
    optkey_reg = FLASH_OPTKEYR;
    optkey1 = FLASH_F0_OPTKEY1;
    optkey2 = FLASH_F0_OPTKEY2;
    break;
  case STLINK_FLASH_TYPE_F4:
    optkey_reg = FLASH_F4_OPT_KEYR;
    break;
  case STLINK_FLASH_TYPE_F7:
    optkey_reg = FLASH_F7_OPT_KEYR;
    break;
  case STLINK_FLASH_TYPE_L0:
    optkey_reg = get_stm32l0_flash_base(sl) + FLASH_OPTKEYR_OFF;
    optkey1 = FLASH_L0_OPTKEY1;
    optkey2 = FLASH_L0_OPTKEY2;
    break;
  case STLINK_FLASH_TYPE_L4:
    optkey_reg = STM32L4_FLASH_OPTKEYR;
    break;
  case STLINK_FLASH_TYPE_G0:
  case STLINK_FLASH_TYPE_G4:
    optkey_reg = STM32Gx_FLASH_OPTKEYR;
    break;
  case STLINK_FLASH_TYPE_WB:
    optkey_reg = STM32WB_FLASH_OPT_KEYR;
    break;
  case STLINK_FLASH_TYPE_H7:
    optkey_reg = FLASH_H7_OPT_KEYR;
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK)
      optkey2_reg = FLASH_H7_OPT_KEYR2;
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

static int unlock_flash_option_if(stlink_t *sl) {
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

static void set_flash_cr_pg(stlink_t *sl, unsigned bank) {
  uint32_t cr_reg, x;

  x = read_flash_cr(sl, bank);

  if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
    cr_reg = FLASH_F4_CR;
    x |= 1 << FLASH_CR_PG;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F7) {
    cr_reg = FLASH_F7_CR;
    x |= 1 << FLASH_CR_PG;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
    cr_reg = STM32L4_FLASH_CR;
    x &= ~STM32L4_FLASH_CR_OPBITS;
    x |= (1 << STM32L4_FLASH_CR_PG);
  } else if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
             sl->flash_type == STLINK_FLASH_TYPE_G4) {
    cr_reg = STM32Gx_FLASH_CR;
    x |= (1 << FLASH_CR_PG);
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
    cr_reg = STM32WB_FLASH_CR;
    x |= (1 << FLASH_CR_PG);
  } else if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
    cr_reg = (bank == BANK_1) ? FLASH_H7_CR1 : FLASH_H7_CR2;
    x |= (1 << FLASH_H7_CR_PG);
  } else {
    cr_reg = FLASH_CR;
    x = (1 << FLASH_CR_PG);
  }

  stlink_write_debug32(sl, cr_reg, x);
}

static void clear_flash_cr_pg(stlink_t *sl, unsigned bank) {
  uint32_t cr_reg, n;
  uint32_t bit = FLASH_CR_PG;

  if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
    cr_reg = FLASH_F4_CR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F7) {
    cr_reg = FLASH_F7_CR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
    cr_reg = STM32L4_FLASH_CR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
             sl->flash_type == STLINK_FLASH_TYPE_G4) {
    cr_reg = STM32Gx_FLASH_CR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
    cr_reg = STM32WB_FLASH_CR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
    cr_reg = (bank == BANK_1) ? FLASH_H7_CR1 : FLASH_H7_CR2;
    bit = FLASH_H7_CR_PG;
  } else {
    cr_reg = FLASH_CR;
  }

  n = read_flash_cr(sl, bank) & ~(1 << bit);
  stlink_write_debug32(sl, cr_reg, n);
}

static void set_flash_cr_per(stlink_t *sl, unsigned bank) {
  uint32_t cr_reg, val;

  if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
      sl->flash_type == STLINK_FLASH_TYPE_G4) {
    cr_reg = STM32Gx_FLASH_CR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
    cr_reg = STM32WB_FLASH_CR;
  } else {
    cr_reg = (bank == BANK_1) ? FLASH_CR : FLASH_CR2;
  }

  stlink_read_debug32(sl, cr_reg, &val);
  val |= (1 << FLASH_CR_PER);
  stlink_write_debug32(sl, cr_reg, val);
}

static void clear_flash_cr_per(stlink_t *sl, unsigned bank) {
  uint32_t cr_reg;

  if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
      sl->flash_type == STLINK_FLASH_TYPE_G4) {
    cr_reg = STM32Gx_FLASH_CR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
    cr_reg = STM32WB_FLASH_CR;
  } else {
    cr_reg = (bank == BANK_1) ? FLASH_CR : FLASH_CR2;
  }

  const uint32_t n = read_flash_cr(sl, bank) & ~(1 << FLASH_CR_PER);
  stlink_write_debug32(sl, cr_reg, n);
}

static void set_flash_cr_mer(stlink_t *sl, bool v, unsigned bank) {
  uint32_t val, cr_reg, cr_mer, cr_pg;

  if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
    cr_reg = FLASH_F4_CR;
    cr_mer = 1 << FLASH_CR_MER;
    cr_pg = 1 << FLASH_CR_PG;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F7) {
    cr_reg = FLASH_F7_CR;
    cr_mer = 1 << FLASH_CR_MER;
    cr_pg = 1 << FLASH_CR_PG;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
    cr_reg = STM32L4_FLASH_CR;
    cr_mer = (1 << STM32L4_FLASH_CR_MER1) | (1 << STM32L4_FLASH_CR_MER2);
    cr_pg = (1 << STM32L4_FLASH_CR_PG);
  } else if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
             sl->flash_type == STLINK_FLASH_TYPE_G4) {
    cr_reg = STM32Gx_FLASH_CR;
    cr_mer = (1 << STM32Gx_FLASH_CR_MER1);

    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
      cr_mer |= (1 << STM32Gx_FLASH_CR_MER2);
    }

    cr_pg = (1 << FLASH_CR_PG);
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
    cr_reg = STM32WB_FLASH_CR;
    cr_mer = (1 << FLASH_CR_MER);
    cr_pg = (1 << FLASH_CR_PG);
  } else if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
    cr_reg = (bank == BANK_1) ? FLASH_H7_CR1 : FLASH_H7_CR2;
    cr_mer = (1 << FLASH_H7_CR_BER);
    cr_pg = (1 << FLASH_H7_CR_PG);
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

static void set_flash_cr_strt(stlink_t *sl, unsigned bank) {
  uint32_t val, cr_reg, cr_strt;

  if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
    cr_reg = FLASH_F4_CR;
    cr_strt = 1 << FLASH_F4_CR_STRT;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F7) {
    cr_reg = FLASH_F7_CR;
    cr_strt = 1 << FLASH_F7_CR_STRT;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
    cr_reg = STM32L4_FLASH_CR;
    cr_strt = (1 << STM32L4_FLASH_CR_STRT);
  } else if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
             sl->flash_type == STLINK_FLASH_TYPE_G4) {
    cr_reg = STM32Gx_FLASH_CR;
    cr_strt = (1 << STM32Gx_FLASH_CR_STRT);
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
    cr_reg = STM32WB_FLASH_CR;
    cr_strt = (1 << STM32WB_FLASH_CR_STRT);
  } else if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
    cr_reg = (bank == BANK_1) ? FLASH_H7_CR1 : FLASH_H7_CR2;
    cr_strt = 1 << FLASH_H7_CR_START(sl->chip_id);
  } else {
    cr_reg = (bank == BANK_1) ? FLASH_CR : FLASH_CR2;
    cr_strt = (1 << FLASH_CR_STRT);
  }

  stlink_read_debug32(sl, cr_reg, &val);
  val |= cr_strt;
  stlink_write_debug32(sl, cr_reg, val);
}

static inline uint32_t read_flash_sr(stlink_t *sl, unsigned bank) {
  uint32_t res, sr_reg;

  if ((sl->flash_type == STLINK_FLASH_TYPE_F0) ||
      (sl->flash_type == STLINK_FLASH_TYPE_F1_XL)) {
    sr_reg = (bank == BANK_1) ? FLASH_SR : FLASH_SR2;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L0) {
    sr_reg = get_stm32l0_flash_base(sl) + FLASH_SR_OFF;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
    sr_reg = FLASH_F4_SR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F7) {
    sr_reg = FLASH_F7_SR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
    sr_reg = STM32L4_FLASH_SR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
             sl->flash_type == STLINK_FLASH_TYPE_G4) {
    sr_reg = STM32Gx_FLASH_SR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
    sr_reg = STM32WB_FLASH_SR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
    sr_reg = (bank == BANK_1) ? FLASH_H7_SR1 : FLASH_H7_SR2;
  } else {
    ELOG("method 'read_flash_sr' is unsupported\n");
    return (-1);
  }

  stlink_read_debug32(sl, sr_reg, &res);
  return (res);
}

static inline int write_flash_sr(stlink_t *sl, unsigned bank, uint32_t val) {
  uint32_t sr_reg;

  if ((sl->flash_type == STLINK_FLASH_TYPE_F0) ||
      (sl->flash_type == STLINK_FLASH_TYPE_F1_XL)) {
    sr_reg = (bank == BANK_1) ? FLASH_SR : FLASH_SR2;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L0) {
    sr_reg = get_stm32l0_flash_base(sl) + FLASH_SR_OFF;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
    sr_reg = FLASH_F4_SR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F7) {
    sr_reg = FLASH_F7_SR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
    sr_reg = STM32L4_FLASH_SR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
             sl->flash_type == STLINK_FLASH_TYPE_G4) {
    sr_reg = STM32Gx_FLASH_SR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
    sr_reg = STM32WB_FLASH_SR;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
    sr_reg = (bank == BANK_1) ? FLASH_H7_SR1 : FLASH_H7_SR2;
  } else {
    ELOG("method 'write_flash_sr' is unsupported\n");
    return (-1);
  }

  return stlink_write_debug32(sl, sr_reg, val);
}

static inline unsigned int is_flash_busy(stlink_t *sl) {
  uint32_t sr_busy_shift;
  unsigned int res;

  if ((sl->flash_type == STLINK_FLASH_TYPE_F0) ||
      (sl->flash_type == STLINK_FLASH_TYPE_F1_XL) ||
      (sl->flash_type == STLINK_FLASH_TYPE_L0)) {
    sr_busy_shift = FLASH_SR_BSY;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
    sr_busy_shift = FLASH_F4_SR_BSY;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F7) {
    sr_busy_shift = FLASH_F7_SR_BSY;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
    sr_busy_shift = STM32L4_FLASH_SR_BSY;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
             sl->flash_type == STLINK_FLASH_TYPE_G4) {
    sr_busy_shift = STM32Gx_FLASH_SR_BSY;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
    sr_busy_shift = STM32WB_FLASH_SR_BSY;
  } else if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
    sr_busy_shift = FLASH_H7_SR_QW;
  } else {
    ELOG("method 'is_flash_busy' is unsupported\n");
    return (-1);
  }

  res = read_flash_sr(sl, BANK_1) & (1 << sr_busy_shift);

  if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL ||
      (sl->flash_type == STLINK_FLASH_TYPE_H7 &&
       sl->chip_flags & CHIP_F_HAS_DUAL_BANK)) {
    res |= read_flash_sr(sl, BANK_2) & (1 << sr_busy_shift);
  }

  return (res);
}

static void wait_flash_busy(stlink_t *sl) {
  // TODO: add some delays here
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

static void clear_flash_error(stlink_t *sl) {
  switch (sl->flash_type) {
  case STLINK_FLASH_TYPE_F0:
    write_flash_sr(sl, BANK_1, FLASH_SR_ERROR_MASK);
    break;
  case STLINK_FLASH_TYPE_F4:
    write_flash_sr(sl, BANK_1, FLASH_F4_SR_ERROR_MASK);
    break;
  case STLINK_FLASH_TYPE_F7:
    write_flash_sr(sl, BANK_1, FLASH_F7_SR_ERROR_MASK);
    break;
  case STLINK_FLASH_TYPE_G0:
  case STLINK_FLASH_TYPE_G4:
    write_flash_sr(sl, BANK_1, STM32Gx_FLASH_SR_ERROR_MASK);
    break;
  case STLINK_FLASH_TYPE_L0:
    write_flash_sr(sl, BANK_1, STM32L0_FLASH_SR_ERROR_MASK);
    break;
  case STLINK_FLASH_TYPE_L4:
    write_flash_sr(sl, BANK_1, STM32L4_FLASH_SR_ERROR_MASK);
    break;
  case STLINK_FLASH_TYPE_H7:
    write_flash_sr(sl, BANK_1, FLASH_H7_SR_ERROR_MASK);
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
      write_flash_sr(sl, BANK_2, FLASH_H7_SR_ERROR_MASK);
    }
    break;
  case STLINK_FLASH_TYPE_WB:
    write_flash_sr(sl, BANK_1, STM32WB_FLASH_SR_ERROR_MASK);
    break;
  default:
    break;
  }
}

static int check_flash_error(stlink_t *sl) {
  uint32_t res = 0;
  uint32_t WRPERR, PROGERR, PGAERR;

  WRPERR = PROGERR = PGAERR = 0;

  switch (sl->flash_type) {
  case STLINK_FLASH_TYPE_F0:
  case STLINK_FLASH_TYPE_F1_XL:
    res = read_flash_sr(sl, BANK_1) & FLASH_SR_ERROR_MASK;
    if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
      res |= read_flash_sr(sl, BANK_2) & FLASH_SR_ERROR_MASK;
    }
    WRPERR = (1 << FLASH_SR_WRPRT_ERR);
    PROGERR = (1 << FLASH_SR_PG_ERR);
    break;
  case STLINK_FLASH_TYPE_F4:
    res = read_flash_sr(sl, BANK_1) & FLASH_F4_SR_ERROR_MASK;
    WRPERR = (1 << FLASH_F4_SR_WRPERR);
    PGAERR = (1 << FLASH_F4_SR_PGAERR);
    break;
  case STLINK_FLASH_TYPE_F7:
    res = read_flash_sr(sl, BANK_1) & FLASH_F7_SR_ERROR_MASK;
    WRPERR = (1 << FLASH_F7_SR_WRP_ERR);
    PROGERR = (1 << FLASH_F7_SR_PGP_ERR);
    break;
  case STLINK_FLASH_TYPE_G0:
  case STLINK_FLASH_TYPE_G4:
    res = read_flash_sr(sl, BANK_1) & STM32Gx_FLASH_SR_ERROR_MASK;
    WRPERR = (1 << STM32Gx_FLASH_SR_WRPERR);
    PROGERR = (1 << STM32Gx_FLASH_SR_PROGERR);
    PGAERR = (1 << STM32Gx_FLASH_SR_PGAERR);
    break;
  case STLINK_FLASH_TYPE_L0:
    res = read_flash_sr(sl, BANK_1) & STM32L0_FLASH_SR_ERROR_MASK;
    WRPERR = (1 << STM32L0_FLASH_SR_WRPERR);
    PROGERR = (1 << STM32L0_FLASH_SR_NOTZEROERR);
    PGAERR = (1 << STM32L0_FLASH_SR_PGAERR);
    break;
  case STLINK_FLASH_TYPE_L4:
    res = read_flash_sr(sl, BANK_1) & STM32L4_FLASH_SR_ERROR_MASK;
    WRPERR = (1 << STM32L4_FLASH_SR_WRPERR);
    PROGERR = (1 << STM32L4_FLASH_SR_PROGERR);
    PGAERR = (1 << STM32L4_FLASH_SR_PGAERR);
    break;
  case STLINK_FLASH_TYPE_H7:
    res = read_flash_sr(sl, BANK_1) & FLASH_H7_SR_ERROR_MASK;
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
      res |= read_flash_sr(sl, BANK_2) & FLASH_H7_SR_ERROR_MASK;
    }
    WRPERR = (1 << FLASH_H7_SR_WRPERR);
    break;
  case STLINK_FLASH_TYPE_WB:
    res = read_flash_sr(sl, BANK_1) & STM32WB_FLASH_SR_ERROR_MASK;
    WRPERR = (1 << STM32WB_FLASH_SR_WRPERR);
    PROGERR = (1 << STM32WB_FLASH_SR_PROGERR);
    PGAERR = (1 << STM32WB_FLASH_SR_PGAERR);
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

    if (res) {
      ELOG("Flash programming error: %#010x\n", res);
    }
    return (-1);
  }

  return (0);
}

static void stop_wdg_in_debug(stlink_t *sl) {
  uint32_t dbgmcu_cr;
  uint32_t set;
  uint32_t value;

  switch (sl->flash_type) {
  case STLINK_FLASH_TYPE_F0:
  case STLINK_FLASH_TYPE_F1_XL:
  case STLINK_FLASH_TYPE_G4:
    dbgmcu_cr = STM32F0_DBGMCU_CR;
    set = (1 << STM32F0_DBGMCU_CR_IWDG_STOP) |
          (1 << STM32F0_DBGMCU_CR_WWDG_STOP);
    break;
  case STLINK_FLASH_TYPE_F4:
  case STLINK_FLASH_TYPE_F7:
  case STLINK_FLASH_TYPE_L4:
    dbgmcu_cr = STM32F4_DBGMCU_APB1FZR1;
    set = (1 << STM32F4_DBGMCU_APB1FZR1_IWDG_STOP) |
          (1 << STM32F4_DBGMCU_APB1FZR1_WWDG_STOP);
    break;
  case STLINK_FLASH_TYPE_L0:
  case STLINK_FLASH_TYPE_G0:
    dbgmcu_cr = STM32L0_DBGMCU_APB1_FZ;
    set = (1 << STM32L0_DBGMCU_APB1_FZ_IWDG_STOP) |
          (1 << STM32L0_DBGMCU_APB1_FZ_WWDG_STOP);
    break;
  case STLINK_FLASH_TYPE_H7:
    dbgmcu_cr = STM32H7_DBGMCU_APB1HFZ;
    set = (1 << STM32H7_DBGMCU_APB1HFZ_IWDG_STOP);
    break;
  case STLINK_FLASH_TYPE_WB:
    dbgmcu_cr = STM32WB_DBGMCU_APB1FZR1;
    set = (1 << STM32WB_DBGMCU_APB1FZR1_IWDG_STOP) |
          (1 << STM32WB_DBGMCU_APB1FZR1_WWDG_STOP);
    break;
  default:
    return;
  }

  if (!stlink_read_debug32(sl, dbgmcu_cr, &value)) {
    stlink_write_debug32(sl, dbgmcu_cr, value | set);
  }
}

static void set_dma_state(stlink_t *sl, flash_loader_t *fl, int bckpRstr) {
  uint32_t rcc, rcc_dma_mask, value;

  rcc = rcc_dma_mask = value = 0;

  switch (sl->flash_type) {
  case STLINK_FLASH_TYPE_F0:
  case STLINK_FLASH_TYPE_F1_XL:
    rcc = STM32F1_RCC_AHBENR;
    rcc_dma_mask = STM32F1_RCC_DMAEN;
    break;
  case STLINK_FLASH_TYPE_F4:
  case STLINK_FLASH_TYPE_F7:
    rcc = STM32F4_RCC_AHB1ENR;
    rcc_dma_mask = STM32F4_RCC_DMAEN;
    break;
  case STLINK_FLASH_TYPE_G0:
    rcc = STM32G0_RCC_AHBENR;
    rcc_dma_mask = STM32G0_RCC_DMAEN;
    break;
  case STLINK_FLASH_TYPE_G4:
  case STLINK_FLASH_TYPE_L4:
    rcc = STM32G4_RCC_AHB1ENR;
    rcc_dma_mask = STM32G4_RCC_DMAEN;
    break;
  case STLINK_FLASH_TYPE_L0:
    rcc = STM32L0_RCC_AHBENR;
    rcc_dma_mask = STM32L0_RCC_DMAEN;
    break;
  case STLINK_FLASH_TYPE_H7:
    rcc = STM32H7_RCC_AHB1ENR;
    rcc_dma_mask = STM32H7_RCC_DMAEN;
    break;
  case STLINK_FLASH_TYPE_WB:
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

static inline void write_flash_ar(stlink_t *sl, uint32_t n, unsigned bank) {
  stlink_write_debug32(sl, (bank == BANK_1) ? FLASH_AR : FLASH_AR2, n);
}

static inline void write_flash_cr_psiz(stlink_t *sl, uint32_t n,
                                       unsigned bank) {
  uint32_t cr_reg, psize_shift;
  uint32_t x = read_flash_cr(sl, bank);

  if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
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

static inline void write_flash_cr_snb(stlink_t *sl, uint32_t n, unsigned bank) {
  uint32_t cr_reg, snb_mask, snb_shift, ser_shift;
  uint32_t x = read_flash_cr(sl, bank);

  if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
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

static inline void write_flash_cr_bker_pnb(stlink_t *sl, uint32_t n) {
  stlink_write_debug32(sl, STM32L4_FLASH_SR,
                       0xFFFFFFFF & ~(1 << STM32L4_FLASH_SR_BSY));
  uint32_t x = read_flash_cr(sl, BANK_1);
  x &= ~STM32L4_FLASH_CR_OPBITS;
  x &= ~STM32L4_FLASH_CR_PAGEMASK;
  x &= ~(1 << STM32L4_FLASH_CR_MER1);
  x &= ~(1 << STM32L4_FLASH_CR_MER2);
  x |= (n << STM32L4_FLASH_CR_PNB);
  x |= (uint32_t)(1lu << STM32L4_FLASH_CR_PER);
#if DEBUG_FLASH
  fprintf(stdout, "BKER:PNB:0x%x 0x%x\n", x, n);
#endif
  stlink_write_debug32(sl, STM32L4_FLASH_CR, x);
}

// Delegates to the backends...

void stlink_close(stlink_t *sl) {
  DLOG("*** stlink_close ***\n");

  if (!sl) {
    return;
  }

  sl->backend->close(sl);
  free(sl);
}

int stlink_exit_debug_mode(stlink_t *sl) {
  DLOG("*** stlink_exit_debug_mode ***\n");

  if (sl->flash_type != STLINK_FLASH_TYPE_UNKNOWN &&
      sl->core_stat != TARGET_RESET) {
    // stop debugging if the target has been identified
    stlink_write_debug32(sl, STLINK_REG_DHCSR, STLINK_REG_DHCSR_DBGKEY);
  }

  return (sl->backend->exit_debug_mode(sl));
}

int stlink_enter_swd_mode(stlink_t *sl) {
  DLOG("*** stlink_enter_swd_mode ***\n");
  return (sl->backend->enter_swd_mode(sl));
}

// Force the core into the debug mode -> halted state.
int stlink_force_debug(stlink_t *sl) {
  DLOG("*** stlink_force_debug_mode ***\n");
  int res = sl->backend->force_debug(sl);
  if (res) {
     return (res);
  }
  // Stop the watchdogs in the halted state for suppress target reboot
  stop_wdg_in_debug(sl);
  return (0);
}

int stlink_exit_dfu_mode(stlink_t *sl) {
  DLOG("*** stlink_exit_dfu_mode ***\n");
  return (sl->backend->exit_dfu_mode(sl));
}

int stlink_core_id(stlink_t *sl) {
  int ret;

  DLOG("*** stlink_core_id ***\n");
  ret = sl->backend->core_id(sl);

  if (ret == -1) {
    ELOG("Failed to read core_id\n");
    return (ret);
  }

  if (sl->verbose > 2) {
    stlink_print_data(sl);
  }

  DLOG("core_id = 0x%08x\n", sl->core_id);
  return (ret);
}

// stlink_chip_id() is called by stlink_load_device_params()
// do not call this procedure directly.
int stlink_chip_id(stlink_t *sl, uint32_t *chip_id) {
  int ret;
  cortex_m3_cpuid_t cpu_id;

  // Read the CPU ID to determine where to read the core id
  if (stlink_cpu_id(sl, &cpu_id) ||
      cpu_id.implementer_id != STLINK_REG_CMx_CPUID_IMPL_ARM) {
    ELOG("Can not connect to target. Please use \'connect under reset\' and "
         "try again\n");
    return -1;
  }

  /*
   * the chip_id register in the reference manual have
   * DBGMCU_IDCODE / DBG_IDCODE name
   *
   */

  if ((sl->core_id == STM32H7_CORE_ID || sl->core_id == STM32H7_CORE_ID_JTAG) &&
      cpu_id.part == STLINK_REG_CMx_CPUID_PARTNO_CM7) {
    // STM32H7 chipid in 0x5c001000 (RM0433 pg3189)
    ret = stlink_read_debug32(sl, 0x5c001000, chip_id);
  } else if (cpu_id.part == STLINK_REG_CMx_CPUID_PARTNO_CM0 ||
             cpu_id.part == STLINK_REG_CMx_CPUID_PARTNO_CM0P) {
    // STM32F0 (RM0091, pg914; RM0360, pg713)
    // STM32L0 (RM0377, pg813; RM0367, pg915; RM0376, pg917)
    // STM32G0 (RM0444, pg1367)
    ret = stlink_read_debug32(sl, 0x40015800, chip_id);
  } else if (cpu_id.part == STLINK_REG_CMx_CPUID_PARTNO_CM33) {
    // STM32L5 (RM0438, pg2157)
    ret = stlink_read_debug32(sl, 0xE0044000, chip_id);
  } else /* СM3, СM4, CM7 */ {
    // default chipid address

    // STM32F1 (RM0008, pg1087; RM0041, pg681)
    // STM32F2 (RM0033, pg1326)
    // STM32F3 (RM0316, pg1095; RM0313, pg874)
    // STM32F7 (RM0385, pg1676; RM0410, pg1912)
    // STM32L1 (RM0038, pg861)
    // STM32L4 (RM0351, pg1840; RM0394, pg1560)
    // STM32G4 (RM0440, pg2086)
    // STM32WB (RM0434, pg1406)
    ret = stlink_read_debug32(sl, 0xE0042000, chip_id);
  }

  if (ret || !(*chip_id)) {
    *chip_id = 0;
    ret = ret?ret:-1;
    ELOG("Could not find chip id!\n");
  } else {
    *chip_id = (*chip_id) & 0xfff;

    // Fix chip_id for F4 rev A errata, read CPU ID, as CoreID is the same for
    // F2/F4
    if (*chip_id == 0x411 && cpu_id.part == STLINK_REG_CMx_CPUID_PARTNO_CM4) {
      *chip_id = 0x413;
    }
  }

  return (ret);
}

/**
 * Cortex M tech ref manual, CPUID register description
 * @param sl stlink context
 * @param cpuid pointer to the result object
 */
int stlink_cpu_id(stlink_t *sl, cortex_m3_cpuid_t *cpuid) {
  uint32_t raw;

  if (stlink_read_debug32(sl, STLINK_REG_CM3_CPUID, &raw)) {
    cpuid->implementer_id = 0;
    cpuid->variant = 0;
    cpuid->part = 0;
    cpuid->revision = 0;
    return (-1);
  }

  cpuid->implementer_id = (raw >> 24) & 0x7f;
  cpuid->variant = (raw >> 20) & 0xf;
  cpuid->part = (raw >> 4) & 0xfff;
  cpuid->revision = raw & 0xf;
  return (0);
}

/**
 * Reads and decodes the flash parameters, as dynamically as possible
 * @param sl
 * @return 0 for success, or -1 for unsupported core type.
 */
int stlink_load_device_params(stlink_t *sl) {
  // This seems to normally work so is unnecessary info for a normal user.
  // Demoted to debug. -- REW
  DLOG("Loading device parameters....\n");
  const struct stlink_chipid_params *params = NULL;
  stlink_core_id(sl);
  uint32_t flash_size;

  if (stlink_chip_id(sl, &sl->chip_id)) {
    return (-1);
  }

  params = stlink_chipid_get_params(sl->chip_id);

  if (params == NULL) {
    WLOG("unknown chip id! %#x\n", sl->chip_id);
    return (-1);
  }

  if (params->flash_type == STLINK_FLASH_TYPE_UNKNOWN) {
    WLOG("Invalid flash type, please check device declaration\n");
    sl->flash_size = 0;
    return (0);
  }

  // These are fixed...
  sl->flash_base = STM32_FLASH_BASE;
  sl->sram_base = STM32_SRAM_BASE;
  stlink_read_debug32(sl, (params->flash_size_reg) & ~3, &flash_size);

  if (params->flash_size_reg & 2) {
    flash_size = flash_size >> 16;
  }

  flash_size = flash_size & 0xffff;

  if ((sl->chip_id == STLINK_CHIPID_STM32_L1_MD ||
       sl->chip_id == STLINK_CHIPID_STM32_F1_VL_MD_LD ||
       sl->chip_id == STLINK_CHIPID_STM32_L1_MD_PLUS) &&
      (flash_size == 0)) {
    sl->flash_size = 128 * 1024;
  } else if (sl->chip_id == STLINK_CHIPID_STM32_L1_CAT2) {
    sl->flash_size = (flash_size & 0xff) * 1024;
  } else if ((sl->chip_id & 0xFFF) == STLINK_CHIPID_STM32_L1_MD_PLUS_HD) {
    // 0 is 384k and 1 is 256k
    if (flash_size == 0) {
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
  sl->option_base = params->option_base;
  sl->option_size = params->option_size;
  sl->chip_flags = params->flags;

  // medium and low devices have the same chipid. ram size depends on flash
  // size. STM32F100xx datasheet Doc ID 16455 Table 2
  if (sl->chip_id == STLINK_CHIPID_STM32_F1_VL_MD_LD &&
      sl->flash_size < 64 * 1024) {
    sl->sram_size = 0x1000;
  }

  if (sl->chip_id == STLINK_CHIPID_STM32_G4_CAT3) {
    uint32_t flash_optr;
    stlink_read_debug32(sl, STM32Gx_FLASH_OPTR, &flash_optr);

    if (!(flash_optr & (1 << STM32G4_FLASH_OPTR_DBANK))) {
      sl->flash_pgsz <<= 1;
    }
  }

  // H7 devices with small flash has one bank
  if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK &&
      sl->flash_type == STLINK_FLASH_TYPE_H7) {
    if ((sl->flash_size / sl->flash_pgsz) <= 1)
      sl->chip_flags &= ~CHIP_F_HAS_DUAL_BANK;
  }

  ILOG("%s: %u KiB SRAM, %u KiB flash in at least %u %s pages.\n",
       params->description, (unsigned)(sl->sram_size / 1024),
       (unsigned)(sl->flash_size / 1024),
       (sl->flash_pgsz < 1024) ? (unsigned)(sl->flash_pgsz)
                               : (unsigned)(sl->flash_pgsz / 1024),
       (sl->flash_pgsz < 1024) ? "byte" : "KiB");

  return (0);
}

int stlink_jtag_reset(stlink_t *sl, int value) {
  DLOG("*** stlink_jtag_reset %d ***\n", value);
  return (sl->backend->jtag_reset(sl, value));
}

int stlink_soft_reset(stlink_t *sl, int halt_on_reset) {
  int ret;
  unsigned timeout;
  uint32_t dhcsr, dfsr;

  DLOG("*** stlink_soft_reset %s***\n", halt_on_reset ? "(halt) " : "");

  // halt core and enable debugging (if not already done)
  // C_DEBUGEN is required to Halt on reset (DDI0337E, p. 10-6)
  stlink_write_debug32(sl, STLINK_REG_DHCSR,
                       STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_HALT |
                           STLINK_REG_DHCSR_C_DEBUGEN);

  // enable Halt on reset by set VC_CORERESET and TRCENA (DDI0337E, p. 10-10)
  if (halt_on_reset) {
    stlink_write_debug32(
        sl, STLINK_REG_CM3_DEMCR,
        STLINK_REG_CM3_DEMCR_TRCENA | STLINK_REG_CM3_DEMCR_VC_HARDERR |
            STLINK_REG_CM3_DEMCR_VC_BUSERR | STLINK_REG_CM3_DEMCR_VC_CORERESET);

    // clear VCATCH in the DFSR register
    stlink_write_debug32(sl, STLINK_REG_DFSR, STLINK_REG_DFSR_VCATCH);
  } else {
    stlink_write_debug32(sl, STLINK_REG_CM3_DEMCR,
                         STLINK_REG_CM3_DEMCR_TRCENA |
                             STLINK_REG_CM3_DEMCR_VC_HARDERR |
                             STLINK_REG_CM3_DEMCR_VC_BUSERR);
  }

  // clear S_RESET_ST in the DHCSR register
  stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr);

  // soft reset (core reset) by SYSRESETREQ (DDI0337E, p. 8-23)
  ret = stlink_write_debug32(sl, STLINK_REG_AIRCR,
                             STLINK_REG_AIRCR_VECTKEY |
                                 STLINK_REG_AIRCR_SYSRESETREQ);
  if (ret) {
    ELOG("Soft reset failed: error write to AIRCR\n");
    return (ret);
  }

  // waiting for a reset within 500ms
  // DDI0337E, p. 10-4, Debug Halting Control and Status Register
  timeout = time_ms() + 500;
  while (time_ms() < timeout) {
    // DDI0337E, p. 10-4, Debug Halting Control and Status Register
    dhcsr = STLINK_REG_DHCSR_S_RESET_ST;
    stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr);
    if ((dhcsr & STLINK_REG_DHCSR_S_RESET_ST) == 0) {
      if (halt_on_reset) {
        // waiting halt by the SYSRESETREQ exception
        // DDI0403E, p. C1-699, Debug Fault Status Register
        dfsr = 0;
        stlink_read_debug32(sl, STLINK_REG_DFSR, &dfsr);
        if ((dfsr & STLINK_REG_DFSR_VCATCH) == 0) {
          continue;
        }
      }
      timeout = 0;
      break;
    }
  }

  // reset DFSR register. DFSR is power-on reset only (DDI0337H, p. 7-5)
  stlink_write_debug32(sl, STLINK_REG_DFSR, STLINK_REG_DFSR_CLEAR);

  if (timeout) {
    ELOG("Soft reset failed: timeout\n");
    return (-1);
  }

  return (0);
}

int stlink_reset(stlink_t *sl, enum reset_type type) {
  uint32_t dhcsr;
  unsigned timeout;

  DLOG("*** stlink_reset ***\n");

  sl->core_stat = TARGET_RESET;

  if (type == RESET_AUTO) {
    // clear S_RESET_ST in DHCSR register for reset state detection
    stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr);
  }

  if (type == RESET_HARD || type == RESET_AUTO) {
    // hardware target reset
    if (sl->version.stlink_v > 1) {
      stlink_jtag_reset(sl, STLINK_DEBUG_APIV2_DRIVE_NRST_LOW);
      // minimum reset pulse duration of 20 us (RM0008, 8.1.2 Power reset)
      usleep(100);
      stlink_jtag_reset(sl, STLINK_DEBUG_APIV2_DRIVE_NRST_HIGH);
    }
    sl->backend->reset(sl);
    usleep(10000);
  }

  if (type == RESET_AUTO) {
    /* Check if the S_RESET_ST bit is set in DHCSR
     * This means that a reset has occurred
     * DDI0337E, p. 10-4, Debug Halting Control and Status Register */

    dhcsr = 0;
    int res = stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr);
    if ((dhcsr & STLINK_REG_DHCSR_S_RESET_ST) == 0 && !res) {
      // reset not done yet
      // try reset through AIRCR so that NRST does not need to be connected

      WLOG("NRST is not connected\n");
      DLOG("Using reset through SYSRESETREQ\n");
      return stlink_soft_reset(sl, 0);
    }

    // waiting for reset the S_RESET_ST bit within 500ms
    timeout = time_ms() + 500;
    while (time_ms() < timeout) {
      dhcsr = STLINK_REG_DHCSR_S_RESET_ST;
      stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr);
      if ((dhcsr & STLINK_REG_DHCSR_S_RESET_ST) == 0) {
        return (0);
      }
    }

    return (-1);
  }

  if (type == RESET_SOFT || type == RESET_SOFT_AND_HALT) {
    return stlink_soft_reset(sl, (type == RESET_SOFT_AND_HALT));
  }

  return (0);
}

int stlink_run(stlink_t *sl, enum run_type type) {
  struct stlink_reg rr;
  DLOG("*** stlink_run ***\n");

  /* Make sure we are in Thumb mode
   * Cortex-M chips don't support ARM mode instructions
   * xPSR may be incorrect if the vector table has invalid data */
  stlink_read_reg(sl, 16, &rr);
  if ((rr.xpsr & (1 << 24)) == 0) {
    ILOG("Go to Thumb mode\n");
    stlink_write_reg(sl, rr.xpsr | (1 << 24), 16);
  }

  return (sl->backend->run(sl, type));
}

int stlink_set_swdclk(stlink_t *sl, int freq_khz) {
  DLOG("*** set_swdclk ***\n");
  return (sl->backend->set_swdclk(sl, freq_khz));
}

int stlink_status(stlink_t *sl) {
  int ret;

  DLOG("*** stlink_status ***\n");
  ret = sl->backend->status(sl);
  stlink_core_stat(sl);
  return (ret);
}

/**
 * Decode the version bits, originally from -sg, verified with usb
 * @param sl stlink context, assumed to contain valid data in the buffer
 * @param slv output parsed version object
 */
void _parse_version(stlink_t *sl, stlink_version_t *slv) {
  sl->version.flags = 0;

  if (sl->version.stlink_v < 3) {
    uint32_t b0 = sl->q_buf[0]; // lsb
    uint32_t b1 = sl->q_buf[1];
    uint32_t b2 = sl->q_buf[2];
    uint32_t b3 = sl->q_buf[3];
    uint32_t b4 = sl->q_buf[4];
    uint32_t b5 = sl->q_buf[5]; // msb

    // b0 b1                       || b2 b3  | b4 b5
    // 4b        | 6b     | 6b     || 2B     | 2B
    // stlink_v  | jtag_v | swim_v || st_vid | stlink_pid

    slv->stlink_v = (b0 & 0xf0) >> 4;
    slv->jtag_v = ((b0 & 0x0f) << 2) | ((b1 & 0xc0) >> 6);
    slv->swim_v = b1 & 0x3f;
    slv->st_vid = (b3 << 8) | b2;
    slv->stlink_pid = (b5 << 8) | b4;

    // ST-LINK/V1 from J11 switch to api-v2 (and support SWD)
    if (slv->stlink_v == 1) {
      slv->jtag_api =
          slv->jtag_v > 11 ? STLINK_JTAG_API_V2 : STLINK_JTAG_API_V1;
    } else {
      slv->jtag_api = STLINK_JTAG_API_V2;

      // preferred API to get last R/W status from J15
      if (sl->version.jtag_v >= 15) {
        sl->version.flags |= STLINK_F_HAS_GETLASTRWSTATUS2;
      }

      if (sl->version.jtag_v >= 13) {
        sl->version.flags |= STLINK_F_HAS_TRACE;
        sl->max_trace_freq = STLINK_V2_MAX_TRACE_FREQUENCY;
      }
    }
  } else {
    // V3 uses different version format, for reference see OpenOCD source
    // (that was written from docs available from ST under NDA):
    // https://github.com/ntfreak/openocd/blob/a6dacdff58ef36fcdac00c53ec27f19de1fbce0d/src/jtag/drivers/stlink_usb.c#L965
    slv->stlink_v = sl->q_buf[0];
    slv->swim_v = sl->q_buf[1];
    slv->jtag_v = sl->q_buf[2];
    slv->st_vid = (uint32_t)((sl->q_buf[9] << 8) | sl->q_buf[8]);
    slv->stlink_pid = (uint32_t)((sl->q_buf[11] << 8) | sl->q_buf[10]);
    slv->jtag_api = STLINK_JTAG_API_V3;
    /* preferred API to get last R/W status */
    sl->version.flags |= STLINK_F_HAS_GETLASTRWSTATUS2;
    sl->version.flags |= STLINK_F_HAS_TRACE;
    sl->max_trace_freq = STLINK_V3_MAX_TRACE_FREQUENCY;
  }

  return;
}

int stlink_version(stlink_t *sl) {
  DLOG("*** looking up stlink version\n");

  if (sl->backend->version(sl)) {
    return (-1);
  }

  _parse_version(sl, &sl->version);

  DLOG("st vid         = 0x%04x (expect 0x%04x)\n", sl->version.st_vid,
       STLINK_USB_VID_ST);
  DLOG("stlink pid     = 0x%04x\n", sl->version.stlink_pid);
  DLOG("stlink version = 0x%x\n", sl->version.stlink_v);
  DLOG("jtag version   = 0x%x\n", sl->version.jtag_v);
  DLOG("swim version   = 0x%x\n", sl->version.swim_v);

  if (sl->version.jtag_v == 0) {
    WLOG("    warning: stlink doesn't support JTAG/SWD interface\n");
  }

  return (0);
}

int stlink_target_voltage(stlink_t *sl) {
  int voltage = -1;
  DLOG("*** reading target voltage\n");

  if (sl->backend->target_voltage != NULL) {
    voltage = sl->backend->target_voltage(sl);

    if (voltage != -1) {
      DLOG("target voltage = %imV\n", voltage);
    } else {
      DLOG("error reading target voltage\n");
    }
  } else {
    DLOG("reading voltage not supported by backend\n");
  }

  return (voltage);
}

int stlink_read_debug32(stlink_t *sl, uint32_t addr, uint32_t *data) {
  int ret;

  ret = sl->backend->read_debug32(sl, addr, data);
  if (!ret)
    DLOG("*** stlink_read_debug32 %#010x at %#010x\n", *data, addr);

  return (ret);
}

int stlink_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data) {
  DLOG("*** stlink_write_debug32 %#010x to %#010x\n", data, addr);
  return sl->backend->write_debug32(sl, addr, data);
}

int stlink_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
  DLOG("*** stlink_write_mem32 %u bytes to %#x\n", len, addr);

  if (len % 4 != 0) {
    ELOG("Data length doesn't have a 32 bit alignment: +%d byte.\n", len % 4);
    return (-1);
  }

  return (sl->backend->write_mem32(sl, addr, len));
}

int stlink_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
  DLOG("*** stlink_read_mem32 ***\n");

  if (len % 4 != 0) { // !!! never ever: fw gives just wrong values
    ELOG("Data length doesn't have a 32 bit alignment: +%d byte.\n", len % 4);
    return (-1);
  }

  return (sl->backend->read_mem32(sl, addr, len));
}

int stlink_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len) {
  DLOG("*** stlink_write_mem8 ***\n");
  return (sl->backend->write_mem8(sl, addr, len));
}

int stlink_read_all_regs(stlink_t *sl, struct stlink_reg *regp) {
  DLOG("*** stlink_read_all_regs ***\n");
  return (sl->backend->read_all_regs(sl, regp));
}

int stlink_read_all_unsupported_regs(stlink_t *sl, struct stlink_reg *regp) {
  DLOG("*** stlink_read_all_unsupported_regs ***\n");
  return (sl->backend->read_all_unsupported_regs(sl, regp));
}

int stlink_write_reg(stlink_t *sl, uint32_t reg, int idx) {
  DLOG("*** stlink_write_reg\n");
  return (sl->backend->write_reg(sl, reg, idx));
}

int stlink_read_reg(stlink_t *sl, int r_idx, struct stlink_reg *regp) {
  DLOG("*** stlink_read_reg\n");
  DLOG(" (%d) ***\n", r_idx);

  if (r_idx > 20 || r_idx < 0) {
    fprintf(stderr, "Error: register index must be in [0..20]\n");
    return (-1);
  }

  return (sl->backend->read_reg(sl, r_idx, regp));
}

int stlink_read_unsupported_reg(stlink_t *sl, int r_idx,
                                struct stlink_reg *regp) {
  int r_convert;

  DLOG("*** stlink_read_unsupported_reg\n");
  DLOG(" (%d) ***\n", r_idx);

  /* Convert to values used by STLINK_REG_DCRSR */
  if (r_idx >= 0x1C &&
      r_idx <= 0x1F) { // primask, basepri, faultmask, or control
    r_convert = 0x14;
  } else if (r_idx == 0x40) { // FPSCR
    r_convert = 0x21;
  } else if (r_idx >= 0x20 && r_idx < 0x40) {
    r_convert = 0x40 + (r_idx - 0x20);
  } else {
    fprintf(stderr, "Error: register address must be in [0x1C..0x40]\n");
    return (-1);
  }

  return (sl->backend->read_unsupported_reg(sl, r_convert, regp));
}

int stlink_write_unsupported_reg(stlink_t *sl, uint32_t val, int r_idx,
                                 struct stlink_reg *regp) {
  int r_convert;

  DLOG("*** stlink_write_unsupported_reg\n");
  DLOG(" (%d) ***\n", r_idx);

  /* Convert to values used by STLINK_REG_DCRSR */
  if (r_idx >= 0x1C &&
      r_idx <= 0x1F) {        /* primask, basepri, faultmask, or control */
    r_convert = r_idx;        // the backend function handles this
  } else if (r_idx == 0x40) { // FPSCR
    r_convert = 0x21;
  } else if (r_idx >= 0x20 && r_idx < 0x40) {
    r_convert = 0x40 + (r_idx - 0x20);
  } else {
    fprintf(stderr, "Error: register address must be in [0x1C..0x40]\n");
    return (-1);
  }

  return (sl->backend->write_unsupported_reg(sl, val, r_convert, regp));
}

bool stlink_is_core_halted(stlink_t *sl) {
  stlink_status(sl);
  return (sl->core_stat == TARGET_HALTED);
}

int stlink_step(stlink_t *sl) {
  DLOG("*** stlink_step ***\n");
  return (sl->backend->step(sl));
}

int stlink_current_mode(stlink_t *sl) {
  int mode = sl->backend->current_mode(sl);

  switch (mode) {
  case STLINK_DEV_DFU_MODE:
    DLOG("stlink current mode: dfu\n");
    return (mode);
  case STLINK_DEV_DEBUG_MODE:
    DLOG("stlink current mode: debug (jtag or swd)\n");
    return (mode);
  case STLINK_DEV_MASS_MODE:
    DLOG("stlink current mode: mass\n");
    return (mode);
  }

  DLOG("stlink mode: unknown!\n");
  return (STLINK_DEV_UNKNOWN_MODE);
}

int stlink_trace_enable(stlink_t *sl, uint32_t frequency) {
  DLOG("*** stlink_trace_enable ***\n");
  return (sl->backend->trace_enable(sl, frequency));
}

int stlink_trace_disable(stlink_t *sl) {
  DLOG("*** stlink_trace_disable ***\n");
  return (sl->backend->trace_disable(sl));
}

int stlink_trace_read(stlink_t *sl, uint8_t *buf, size_t size) {
  return (sl->backend->trace_read(sl, buf, size));
}

// End of delegates....  Common code below here...

// same as above with entrypoint.

void stlink_run_at(stlink_t *sl, stm32_addr_t addr) {
  stlink_write_reg(sl, addr, 15); /* pc register */
  stlink_run(sl, RUN_NORMAL);

  while (stlink_is_core_halted(sl)) {
    usleep(3000000);
  }
}

// this function is called by stlink_status()
// do not call stlink_core_stat() directly, always use stlink_status()
void stlink_core_stat(stlink_t *sl) {
  switch (sl->core_stat) {
  case TARGET_RUNNING:
    DLOG("  core status: running\n");
    return;
  case TARGET_HALTED:
    DLOG("  core status: halted\n");
    return;
  case TARGET_RESET:
    DLOG("  core status: reset\n");
    return;
  case TARGET_DEBUG_RUNNING:
    DLOG("  core status: debug running\n");
    return;
  default:
    DLOG("  core status: unknown\n");
  }
}

void stlink_print_data(stlink_t *sl) {
  if (sl->q_len <= 0 || sl->verbose < UDEBUG) {
    return;
  }

  if (sl->verbose > 2) {
    DLOG("data_len = %d 0x%x\n", sl->q_len, sl->q_len);
  }

  for (int i = 0; i < sl->q_len; i++) {
    if (i % 16 == 0) {
      /*
      if (sl->q_data_dir == Q_DATA_OUT) {
          fprintf(stdout, "\n<- 0x%08x ", sl->q_addr + i);
      } else {
          fprintf(stdout, "\n-> 0x%08x ", sl->q_addr + i);
      }
      */
    }
    // DLOG(" %02x", (unsigned int) sl->q_buf[i]);
    fprintf(stderr, " %02x", (unsigned int)sl->q_buf[i]);
  }
  // DLOG("\n\n");
  fprintf(stderr, "\n");
}

/* Memory mapped file */
typedef struct mapped_file {
  uint8_t *base;
  size_t len;
} mapped_file_t;

#define MAPPED_FILE_INITIALIZER                                                \
  { NULL, 0 }

static int map_file(mapped_file_t *mf, const char *path) {
  int error = -1;
  struct stat st;

  const int fd = open(path, O_RDONLY | O_BINARY);

  if (fd == -1) {
    fprintf(stderr, "open(%s) == -1\n", path);
    return (-1);
  }

  if (fstat(fd, &st) == -1) {
    fprintf(stderr, "fstat(%s) == -1\n", path);
    goto on_error;
  }

  if (sizeof(st.st_size) != sizeof(size_t)) {
    // on 32 bit systems, check if there is an overflow
    if (st.st_size > (off_t)SSIZE_MAX) {
      fprintf(stderr, "mmap() size_t overflow for file %s\n", path);
      goto on_error;
    }
  }

  mf->base =
      (uint8_t *)mmap(NULL, (size_t)(st.st_size), PROT_READ, MAP_SHARED, fd, 0);

  if (mf->base == MAP_FAILED) {
    fprintf(stderr, "mmap() == MAP_FAILED for file %s\n", path);
    goto on_error;
  }

  mf->len = (size_t)st.st_size;
  error = 0; // success

on_error:
  close(fd);
  return (error);
}

static void unmap_file(mapped_file_t *mf) {
  munmap((void *)mf->base, mf->len);
  mf->base = (unsigned char *)MAP_FAILED;
  mf->len = 0;
}

/* Limit the block size to compare to 0x1800 as anything larger will stall the
 * STLINK2 Maybe STLINK V1 needs smaller value!
 */
static int check_file(stlink_t *sl, mapped_file_t *mf, stm32_addr_t addr) {
  size_t off;
  size_t n_cmp = sl->flash_pgsz;

  if (n_cmp > 0x1800) {
    n_cmp = 0x1800;
  }

  for (off = 0; off < mf->len; off += n_cmp) {
    size_t aligned_size;

    size_t cmp_size = n_cmp; // adjust last page size

    if ((off + n_cmp) > mf->len) {
      cmp_size = mf->len - off;
    }

    aligned_size = cmp_size;

    if (aligned_size & (4 - 1)) {
      aligned_size = (cmp_size + 4) & ~(4 - 1);
    }

    stlink_read_mem32(sl, addr + (uint32_t)off, (uint16_t)aligned_size);

    if (memcmp(sl->q_buf, mf->base + off, cmp_size)) {
      return (-1);
    }
  }

  return (0);
}

static void md5_calculate(mapped_file_t *mf) {
  // calculate md5 checksum of given binary file
  Md5Context md5Context;
  MD5_HASH md5Hash;
  Md5Initialise(&md5Context);
  Md5Update(&md5Context, mf->base, (uint32_t)mf->len);
  Md5Finalise(&md5Context, &md5Hash);
  printf("md5 checksum: ");

  for (int i = 0; i < (int)sizeof(md5Hash); i++) {
    printf("%x", md5Hash.bytes[i]);
  }

  printf(", ");
}

static void stlink_checksum(mapped_file_t *mp) {
  /* checksum that backward compatible with official ST tools */
  uint32_t sum = 0;
  uint8_t *mp_byte = (uint8_t *)mp->base;

  for (size_t i = 0; i < mp->len; ++i) {
    sum += mp_byte[i];
  }

  printf("stlink checksum: 0x%08x\n", sum);
}

static void stlink_fwrite_finalize(stlink_t *sl, stm32_addr_t addr) {
  unsigned int val;
  // set PC to the reset routine
  stlink_read_debug32(sl, addr + 4, &val);
  stlink_write_reg(sl, val, 15);
  stlink_run(sl, RUN_NORMAL);
}

int stlink_mwrite_sram(stlink_t *sl, uint8_t *data, uint32_t length,
                       stm32_addr_t addr) {
  // write the file in sram at addr

  int error = -1;
  size_t off;
  size_t len;

  // check addr range is inside the sram
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
    fprintf(stderr, "unaligned addr\n");
    goto on_error;
  }

  len = length;

  if (len & 3) {
    len -= len & 3;
  }

  // do the copy by 1kB blocks
  for (off = 0; off < len; off += 1024) {
    size_t size = 1024;

    if ((off + size) > len) {
      size = len - off;
    }

    memcpy(sl->q_buf, data + off, size);

    if (size & 3) {
      size += 2;
    } // round size if needed

    stlink_write_mem32(sl, addr + (uint32_t)off, (uint16_t)size);
  }

  if (length > len) {
    memcpy(sl->q_buf, data + len, length - len);
    stlink_write_mem8(sl, addr + (uint32_t)len, (uint16_t)(length - len));
  }

  error = 0; // success
  stlink_fwrite_finalize(sl, addr);

on_error:
  return (error);
}

int stlink_fwrite_sram(stlink_t *sl, const char *path, stm32_addr_t addr) {
  // write the file in sram at addr

  int error = -1;
  size_t off;
  size_t len;
  mapped_file_t mf = MAPPED_FILE_INITIALIZER;

  if (map_file(&mf, path) == -1) {
    fprintf(stderr, "map_file() == -1\n");
    return (-1);
  }

  printf("file %s ", path);
  md5_calculate(&mf);
  stlink_checksum(&mf);

  // check if addr range is inside the SRAM
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
    fprintf(stderr, "unaligned addr\n");
    goto on_error;
  }

  len = mf.len;

  if (len & 3) {
    len -= len & 3;
  }

  // do the copy by 1kB blocks
  for (off = 0; off < len; off += 1024) {
    size_t size = 1024;

    if ((off + size) > len) {
      size = len - off;
    }

    memcpy(sl->q_buf, mf.base + off, size);

    if (size & 3) {
      size += 2;
    } // round size if needed

    stlink_write_mem32(sl, addr + (uint32_t)off, (uint16_t)size);
  }

  if (mf.len > len) {
    memcpy(sl->q_buf, mf.base + len, mf.len - len);
    stlink_write_mem8(sl, addr + (uint32_t)len, (uint16_t)(mf.len - len));
  }

  // check the file has been written
  if (check_file(sl, &mf, addr) == -1) {
    fprintf(stderr, "check_file() == -1\n");
    goto on_error;
  }

  error = 0; // success
  stlink_fwrite_finalize(sl, addr);

on_error:
  unmap_file(&mf);
  return (error);
}

typedef bool (*save_block_fn)(void *arg, uint8_t *block, ssize_t len);

static int stlink_read(stlink_t *sl, stm32_addr_t addr, size_t size,
                       save_block_fn fn, void *fn_arg) {

  int error = -1;

  if (size < 1) {
    size = sl->flash_size;
  }

  if (size > sl->flash_size) {
    size = sl->flash_size;
  }

  size_t cmp_size = (sl->flash_pgsz > 0x1800) ? 0x1800 : sl->flash_pgsz;

  for (size_t off = 0; off < size; off += cmp_size) {
    size_t aligned_size;

    // adjust last page size
    if ((off + cmp_size) > size) {
      cmp_size = size - off;
    }

    aligned_size = cmp_size;

    if (aligned_size & (4 - 1)) {
      aligned_size = (cmp_size + 4) & ~(4 - 1);
    }

    stlink_read_mem32(sl, addr + (uint32_t)off, (uint16_t)aligned_size);

    if (!fn(fn_arg, sl->q_buf, aligned_size)) {
      goto on_error;
    }
  }

  error = 0; // success

on_error:
  return (error);
}

struct stlink_fread_worker_arg {
  int fd;
};

static bool stlink_fread_worker(void *arg, uint8_t *block, ssize_t len) {
  struct stlink_fread_worker_arg *the_arg =
      (struct stlink_fread_worker_arg *)arg;

  if (write(the_arg->fd, block, len) != len) {
    fprintf(stderr, "write() != aligned_size\n");
    return (false);
  } else {
    return (true);
  }
}

struct stlink_fread_ihex_worker_arg {
  FILE *file;
  uint32_t addr;
  uint32_t lba;
  uint8_t buf[16];
  uint8_t buf_pos;
};

static bool
stlink_fread_ihex_newsegment(struct stlink_fread_ihex_worker_arg *the_arg) {
  uint32_t addr = the_arg->addr;
  uint8_t sum = 2 + 4 + (uint8_t)((addr & 0xFF000000) >> 24) +
                (uint8_t)((addr & 0x00FF0000) >> 16);

  if (17 != fprintf(the_arg->file, ":02000004%04X%02X\r\n",
                    (addr & 0xFFFF0000) >> 16, (uint8_t)(0x100 - sum))) {
    return (false);
  }

  the_arg->lba = (addr & 0xFFFF0000);
  return (true);
}

static bool
stlink_fread_ihex_writeline(struct stlink_fread_ihex_worker_arg *the_arg) {
  uint8_t count = the_arg->buf_pos;

  if (count == 0) {
    return (true);
  }

  uint32_t addr = the_arg->addr;

  if (the_arg->lba != (addr & 0xFFFF0000)) { // segment changed
    if (!stlink_fread_ihex_newsegment(the_arg)) {
      return (false);
    }
  }

  uint8_t sum = count + (uint8_t)((addr & 0x0000FF00) >> 8) +
                (uint8_t)(addr & 0x000000FF);

  if (9 != fprintf(the_arg->file, ":%02X%04X00", count, (addr & 0x0000FFFF))) {
    return (false);
  }

  for (uint8_t i = 0; i < count; ++i) {
    uint8_t b = the_arg->buf[i];
    sum += b;

    if (2 != fprintf(the_arg->file, "%02X", b)) {
      return (false);
    }
  }

  if (4 != fprintf(the_arg->file, "%02X\r\n", (uint8_t)(0x100 - sum))) {
    return (false);
  }

  the_arg->addr += count;
  the_arg->buf_pos = 0;

  return (true);
}

static bool stlink_fread_ihex_init(struct stlink_fread_ihex_worker_arg *the_arg,
                                   int fd, stm32_addr_t addr) {
  the_arg->file = fdopen(fd, "w");
  the_arg->addr = addr;
  the_arg->lba = 0;
  the_arg->buf_pos = 0;

  return (the_arg->file != NULL);
}

static bool stlink_fread_ihex_worker(void *arg, uint8_t *block, ssize_t len) {
  struct stlink_fread_ihex_worker_arg *the_arg =
      (struct stlink_fread_ihex_worker_arg *)arg;

  for (ssize_t i = 0; i < len; ++i) {
    if (the_arg->buf_pos == sizeof(the_arg->buf)) { // line is full
      if (!stlink_fread_ihex_writeline(the_arg)) {
        return (false);
      }
    }

    the_arg->buf[the_arg->buf_pos++] = block[i];
  }

  return (true);
}

static bool
stlink_fread_ihex_finalize(struct stlink_fread_ihex_worker_arg *the_arg) {
  if (!stlink_fread_ihex_writeline(the_arg)) {
    return (false);
  }

  // FIXME: do we need the Start Linear Address?

  if (13 != fprintf(the_arg->file, ":00000001FF\r\n")) { // EoF
    return (false);
  }

  return (0 == fclose(the_arg->file));
}

int stlink_fread(stlink_t *sl, const char *path, bool is_ihex,
                 stm32_addr_t addr, size_t size) {
  // read size bytes from addr to file
  ILOG("read from address %#010x size %u\n", addr, (unsigned)size);

  int error;
  int fd = open(path, O_RDWR | O_TRUNC | O_CREAT | O_BINARY, 00700);

  if (fd == -1) {
    fprintf(stderr, "open(%s) == -1\n", path);
    return (-1);
  }

  if (is_ihex) {
    struct stlink_fread_ihex_worker_arg arg;

    if (stlink_fread_ihex_init(&arg, fd, addr)) {
      error = stlink_read(sl, addr, size, &stlink_fread_ihex_worker, &arg);

      if (!stlink_fread_ihex_finalize(&arg)) {
        error = -1;
      }
    } else {
      error = -1;
    }
  } else {
    struct stlink_fread_worker_arg arg = {fd};
    error = stlink_read(sl, addr, size, &stlink_fread_worker, &arg);
  }

  close(fd);
  return (error);
}

int write_buffer_to_sram(stlink_t *sl, flash_loader_t *fl, const uint8_t *buf,
                         size_t size) {
  // write the buffer right after the loader
  int ret = 0;
  size_t chunk = size & ~0x3;
  size_t rem = size & 0x3;

  if (chunk) {
    memcpy(sl->q_buf, buf, chunk);
    ret = stlink_write_mem32(sl, fl->buf_addr, (uint16_t)chunk);
  }

  if (rem && !ret) {
    memcpy(sl->q_buf, buf + chunk, rem);
    ret = stlink_write_mem8(sl, (fl->buf_addr) + (uint32_t)chunk, (uint16_t)rem);
  }

  return (ret);
}

uint32_t calculate_F4_sectornum(uint32_t flashaddr) {
  uint32_t offset = 0;
  flashaddr &= ~STM32_FLASH_BASE; // page now holding the actual flash address

  if (flashaddr >= 0x100000) {
    offset = 12;
    flashaddr -= 0x100000;
  }

  if (flashaddr < 0x4000) {
    return (offset + 0);
  } else if (flashaddr < 0x8000) {
    return (offset + 1);
  } else if (flashaddr < 0xc000) {
    return (offset + 2);
  } else if (flashaddr < 0x10000) {
    return (offset + 3);
  } else if (flashaddr < 0x20000) {
    return (offset + 4);
  } else {
    return (offset + (flashaddr / 0x20000) + 4);
  }
}

uint32_t calculate_F7_sectornum(uint32_t flashaddr) {
  flashaddr &= ~STM32_FLASH_BASE; // Page now holding the actual flash address

  if (flashaddr < 0x20000) {
    return (flashaddr / 0x8000);
  } else if (flashaddr < 0x40000) {
    return (4);
  } else {
    return ((flashaddr / 0x40000) + 4);
  }
}

uint32_t calculate_H7_sectornum(stlink_t *sl, uint32_t flashaddr,
                                unsigned bank) {
  flashaddr &=
      ~((bank == BANK_1)
            ? STM32_FLASH_BASE
            : STM32_H7_FLASH_BANK2_BASE); // sector holding the flash address
  return (flashaddr / sl->flash_pgsz);
}

// returns BKER:PNB for the given page address
uint32_t calculate_L4_page(stlink_t *sl, uint32_t flashaddr) {
  uint32_t bker = 0;
  uint32_t flashopt;
  stlink_read_debug32(sl, STM32L4_FLASH_OPTR, &flashopt);
  flashaddr -= STM32_FLASH_BASE;

  if (sl->chip_id == STLINK_CHIPID_STM32_L4 ||
      sl->chip_id == STLINK_CHIPID_STM32_L496x_L4A6x ||
      sl->chip_id == STLINK_CHIPID_STM32_L4Rx) {
    // this chip use dual banked flash
    if (flashopt & (uint32_t)(1lu << STM32L4_FLASH_OPTR_DUALBANK)) {
      uint32_t banksize = (uint32_t)sl->flash_size / 2;

      if (flashaddr >= banksize) {
        flashaddr -= banksize;
        bker = 0x100;
      }
    }
  }

  // For 1MB chips without the dual-bank option set, the page address will
  // overflow into the BKER bit, which gives us the correct bank:page value.
  return (bker | flashaddr / (uint32_t)sl->flash_pgsz);
}

uint32_t stlink_calculate_pagesize(stlink_t *sl, uint32_t flashaddr) {
  if ((sl->chip_id == STLINK_CHIPID_STM32_F2) ||
      (sl->chip_id == STLINK_CHIPID_STM32_F4) ||
      (sl->chip_id == STLINK_CHIPID_STM32_F4_DE) ||
      (sl->chip_id == STLINK_CHIPID_STM32_F4_LP) ||
      (sl->chip_id == STLINK_CHIPID_STM32_F4_HD) ||
      (sl->chip_id == STLINK_CHIPID_STM32_F411xx) ||
      (sl->chip_id == STLINK_CHIPID_STM32_F446) ||
      (sl->chip_id == STLINK_CHIPID_STM32_F4_DSI) ||
      (sl->chip_id == STLINK_CHIPID_STM32_F72xxx) ||
      (sl->chip_id == STLINK_CHIPID_STM32_F412)) {
    uint32_t sector = calculate_F4_sectornum(flashaddr);

    if (sector >= 12) {
      sector -= 12;
    }

    if (sector < 4) {
      sl->flash_pgsz = 0x4000;
    } else if (sector < 5) {
      sl->flash_pgsz = 0x10000;
    } else {
      sl->flash_pgsz = 0x20000;
    }
  } else if (sl->chip_id == STLINK_CHIPID_STM32_F7 ||
             sl->chip_id == STLINK_CHIPID_STM32_F76xxx) {
    uint32_t sector = calculate_F7_sectornum(flashaddr);

    if (sector < 4) {
      sl->flash_pgsz = 0x8000;
    } else if (sector < 5) {
      sl->flash_pgsz = 0x20000;
    } else {
      sl->flash_pgsz = 0x40000;
    }
  }

  return ((uint32_t)sl->flash_pgsz);
}

/**
 * Erase a page of flash, assumes sl is fully populated with things like
 * chip/core ids
 * @param sl stlink context
 * @param flashaddr an address in the flash page to erase
 * @return 0 on success -ve on failure
 */
int stlink_erase_flash_page(stlink_t *sl, stm32_addr_t flashaddr) {
  // wait for ongoing op to finish
  wait_flash_busy(sl);
  // clear flash IO errors
  clear_flash_error(sl);

  if (sl->flash_type == STLINK_FLASH_TYPE_F4 ||
      sl->flash_type == STLINK_FLASH_TYPE_F7 ||
      sl->flash_type == STLINK_FLASH_TYPE_L4) {
    // unlock if locked
    unlock_flash_if(sl);

    // select the page to erase
    if ((sl->chip_id == STLINK_CHIPID_STM32_L4) ||
        (sl->chip_id == STLINK_CHIPID_STM32_L43x_L44x) ||
        (sl->chip_id == STLINK_CHIPID_STM32_L45x_L46x) ||
        (sl->chip_id == STLINK_CHIPID_STM32_L496x_L4A6x) ||
        (sl->chip_id == STLINK_CHIPID_STM32_L4Rx)) {
      // calculate the actual bank+page from the address
      uint32_t page = calculate_L4_page(sl, flashaddr);

      fprintf(stderr, "EraseFlash - Page:0x%x Size:0x%x ", page,
              stlink_calculate_pagesize(sl, flashaddr));

      write_flash_cr_bker_pnb(sl, page);
    } else if (sl->chip_id == STLINK_CHIPID_STM32_F7 ||
               sl->chip_id == STLINK_CHIPID_STM32_F76xxx) {
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
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L0) {

    uint32_t val;
    uint32_t flash_regs_base = get_stm32l0_flash_base(sl);

    // check if the locks are set
    stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);

    if ((val & (1 << 0)) || (val & (1 << 1))) {
      // disable pecr protection
      stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF,
                           FLASH_L0_PEKEY1);
      stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF,
                           FLASH_L0_PEKEY2);

      // check pecr.pelock is cleared
      stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);

      if (val & (1 << 0)) {
        WLOG("pecr.pelock not clear (%#x)\n", val);
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
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB ||
             sl->flash_type == STLINK_FLASH_TYPE_G0 ||
             sl->flash_type == STLINK_FLASH_TYPE_G4) {
    uint32_t val;
    unlock_flash_if(sl);
    set_flash_cr_per(sl, BANK_1); // set the 'enable Flash erase' bit

    // set the page to erase
    if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
      uint32_t flash_page =
          ((flashaddr - STM32_FLASH_BASE) / (uint32_t)(sl->flash_pgsz));
      stlink_read_debug32(sl, STM32WB_FLASH_CR, &val);

      // sec 3.10.5 - PNB[7:0] is offset by 3.
      val &= ~(0xFF << 3); // Clear previously set page number (if any)
      val |= ((flash_page & 0xFF) << 3);

      stlink_write_debug32(sl, STM32WB_FLASH_CR, val);
    } else if (sl->flash_type == STLINK_FLASH_TYPE_G0) {
      uint32_t flash_page =
          ((flashaddr - STM32_FLASH_BASE) / (uint32_t)(sl->flash_pgsz));
      stlink_read_debug32(sl, STM32Gx_FLASH_CR, &val);
      // sec 3.7.5 - PNB[5:0] is offset by 3. PER is 0x2.
      val &= ~(0x3F << 3);
      val |= ((flash_page & 0x3F) << 3) | (1 << FLASH_CR_PER);
      stlink_write_debug32(sl, STM32Gx_FLASH_CR, val);
    } else if (sl->flash_type == STLINK_FLASH_TYPE_G4) {
      uint32_t flash_page =
          ((flashaddr - STM32_FLASH_BASE) / (uint32_t)(sl->flash_pgsz));
      stlink_read_debug32(sl, STM32Gx_FLASH_CR, &val);
      // sec 3.7.5 - PNB[6:0] is offset by 3. PER is 0x2.
      val &= ~(0x7F << 3);
      val |= ((flash_page & 0x7F) << 3) | (1 << FLASH_CR_PER);
      stlink_write_debug32(sl, STM32Gx_FLASH_CR, val);
    }

    set_flash_cr_strt(sl, BANK_1);  // set the 'start operation' bit
    wait_flash_busy(sl);            // wait for the 'busy' bit to clear
    clear_flash_cr_per(sl, BANK_1); // clear the 'enable page erase' bit
    lock_flash(sl);
  } else if (sl->flash_type == STLINK_FLASH_TYPE_F0 ||
             sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
    unsigned bank = (flashaddr < STM32_F1_FLASH_BANK2_BASE) ? BANK_1 : BANK_2;
    unlock_flash_if(sl);
    clear_flash_cr_pg(sl, bank);         // clear the pg bit
    set_flash_cr_per(sl, bank);          // set the page erase bit
    write_flash_ar(sl, flashaddr, bank); // select the page to erase
    set_flash_cr_strt(sl,
                      bank); // start erase operation, reset by hw with busy bit
    wait_flash_busy(sl);
    clear_flash_cr_per(sl, bank); // clear the page erase bit
    lock_flash(sl);
  } else if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
    unsigned bank = (flashaddr < STM32_H7_FLASH_BANK2_BASE) ? BANK_1 : BANK_2;
    unlock_flash_if(sl); // unlock if locked
    uint32_t sector = calculate_H7_sectornum(
        sl, flashaddr, bank); // calculate the actual page from the address
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

// Check if an address and size are within the flash
int stlink_check_address_range_validity(stlink_t *sl, stm32_addr_t addr, size_t size) {
  if (addr < sl->flash_base || addr >= (sl->flash_base + sl->flash_size)) {
    ELOG("Invalid address, it should be within 0x%08x - 0x%08lx\n", sl->flash_base, (sl->flash_base + sl->flash_size -1));
    return (-1);
  }
  if ((addr + size) > (sl->flash_base + sl->flash_size)) {
    ELOG("The size exceeds the size of the flash (0x%08lx bytes available)\n", (sl->flash_base + sl->flash_size - addr));
    return (-1);
  }
  return 0;
}

// Check if an address is aligned with the beginning of a page
int stlink_check_address_alignment(stlink_t *sl, stm32_addr_t addr) {
  stm32_addr_t page = sl->flash_base;

  while (page < addr) {
    page += stlink_calculate_pagesize(sl, page);
  }

  if (page != addr) {
    return -1;
  }

  return 0;
}

int stlink_erase_flash_section(stlink_t *sl, stm32_addr_t base_addr, size_t size, bool align_size) {
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
    size_t page_size = stlink_calculate_pagesize(sl, addr);

    // Check if size is aligned with a page, unless we want to completely erase the last page
    if ((addr + page_size) > (base_addr + size) && !align_size) {
      ELOG("Invalid size (not aligned with a page). Page size at address %#x is %#lx\n", addr, page_size);
      return -1;
    }

    if (stlink_erase_flash_page(sl, addr)) {
      WLOG("Failed to erase_flash_page(%#x) == -1\n", addr);
      return (-1);
    }

    fprintf(stdout, "-> Flash page at %#x erased (size: %#lx)\n", addr, page_size);
    fflush(stdout);

    // check the next page is within the range to erase
    addr += page_size;
  } while (addr < (base_addr + size));

  fprintf(stdout, "\n");
  return 0;
}

int stlink_erase_flash_mass(stlink_t *sl) {
  int err = 0;

  // TODO: User MER bit to mass-erase WB series.
  if (sl->flash_type == STLINK_FLASH_TYPE_L0 ||
      sl->flash_type == STLINK_FLASH_TYPE_WB) {

    err = stlink_erase_flash_section(sl, sl->flash_base, sl->flash_size, false);

  } else {
    wait_flash_busy(sl);
    clear_flash_error(sl);
    unlock_flash_if(sl);

    if (sl->flash_type == STLINK_FLASH_TYPE_H7 &&
        sl->chip_id != STLINK_CHIPID_STM32_H7Ax) {
      // set parallelism
      write_flash_cr_psiz(sl, 3 /*64it*/, BANK_1);
      if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
        write_flash_cr_psiz(sl, 3 /*64bit*/, BANK_2);
      }
    }

    set_flash_cr_mer(sl, 1, BANK_1); // set the mass erase bit
    set_flash_cr_strt(
        sl, BANK_1); // start erase operation, reset by hw with busy bit

    if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL ||
        (sl->flash_type == STLINK_FLASH_TYPE_H7 &&
         sl->chip_flags & CHIP_F_HAS_DUAL_BANK)) {
      set_flash_cr_mer(sl, 1, BANK_2); // set the mass erase bit in bank 2
      set_flash_cr_strt(sl, BANK_2);   // start erase operation in bank 2
    }

    wait_flash_busy_progress(sl);
    lock_flash(sl);

    // reset the mass erase bit
    set_flash_cr_mer(sl, 0, BANK_1);
    if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL ||
        (sl->flash_type == STLINK_FLASH_TYPE_H7 &&
         sl->chip_flags & CHIP_F_HAS_DUAL_BANK)) {
      set_flash_cr_mer(sl, 0, BANK_2);
    }

    err = check_flash_error(sl);
  }

  return (err);
}

int stlink_fcheck_flash(stlink_t *sl, const char *path, stm32_addr_t addr) {
  // check the contents of path are at addr

  int res;
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
int stlink_verify_write_flash(stlink_t *sl, stm32_addr_t address, uint8_t *data,
                              unsigned length) {
  size_t off;
  size_t cmp_size = (sl->flash_pgsz > 0x1800) ? 0x1800 : sl->flash_pgsz;
  ILOG("Starting verification of write complete\n");

  for (off = 0; off < length; off += cmp_size) {
    size_t aligned_size;

    // adjust last page size
    if ((off + cmp_size) > length) {
      cmp_size = length - off;
    }

    aligned_size = cmp_size;

    if (aligned_size & (4 - 1)) {
      aligned_size = (cmp_size + 4) & ~(4 - 1);
    }

    stlink_read_mem32(sl, address + (uint32_t)off, (uint16_t)aligned_size);

    if (memcmp(sl->q_buf, data + off, cmp_size)) {
      ELOG("Verification of flash failed at offset: %u\n", (unsigned int)off);
      return (-1);
    }
  }

  ILOG("Flash written and verified! jolly good!\n");
  return (0);
}

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

int stlink_flashloader_start(stlink_t *sl, flash_loader_t *fl) {
  // disable DMA
  set_dma_state(sl, fl, 0);

  // wait for ongoing op to finish
  wait_flash_busy(sl);
  // Clear errors
  clear_flash_error(sl);

  if ((sl->flash_type == STLINK_FLASH_TYPE_F4) ||
      (sl->flash_type == STLINK_FLASH_TYPE_F7) ||
      (sl->flash_type == STLINK_FLASH_TYPE_L4)) {
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

    if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
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
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB ||
             sl->flash_type == STLINK_FLASH_TYPE_G0 ||
             sl->flash_type == STLINK_FLASH_TYPE_G4) {
    ILOG("Starting Flash write for WB/G0/G4\n");

    unlock_flash_if(sl);         // unlock flash if necessary
    set_flash_cr_pg(sl, BANK_1); // set PG 'allow programming' bit
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L0) {
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
  } else if ((sl->flash_type == STLINK_FLASH_TYPE_F0) ||
             (sl->flash_type == STLINK_FLASH_TYPE_F1_XL)) {
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
    if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
      set_flash_cr_pg(sl, BANK_2);
    }
  } else if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
    ILOG("Starting Flash write for H7\n");

    unlock_flash_if(sl);         // unlock the cr
    set_flash_cr_pg(sl, BANK_1); // set programming mode
    if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK) {
      set_flash_cr_pg(sl, BANK_2);
    }
    if (sl->chip_id != STLINK_CHIPID_STM32_H7Ax) {
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
  if ((sl->flash_type == STLINK_FLASH_TYPE_F4) ||
      (sl->flash_type == STLINK_FLASH_TYPE_F7) ||
      (sl->flash_type == STLINK_FLASH_TYPE_L4)) {
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
  } else if (sl->flash_type == STLINK_FLASH_TYPE_WB ||
             sl->flash_type == STLINK_FLASH_TYPE_G0 ||
             sl->flash_type == STLINK_FLASH_TYPE_G4) {
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
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L0) {
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
  } else if ((sl->flash_type == STLINK_FLASH_TYPE_F0) ||
             (sl->flash_type == STLINK_FLASH_TYPE_F1_XL)) {
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
  } else if (sl->flash_type == STLINK_FLASH_TYPE_H7) {
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

  if ((sl->flash_type == STLINK_FLASH_TYPE_F0) ||
      (sl->flash_type == STLINK_FLASH_TYPE_F1_XL) ||
      (sl->flash_type == STLINK_FLASH_TYPE_F4) ||
      (sl->flash_type == STLINK_FLASH_TYPE_F7) ||
      (sl->flash_type == STLINK_FLASH_TYPE_L4) ||
      (sl->flash_type == STLINK_FLASH_TYPE_WB) ||
      (sl->flash_type == STLINK_FLASH_TYPE_G0) ||
      (sl->flash_type == STLINK_FLASH_TYPE_G4) ||
      (sl->flash_type == STLINK_FLASH_TYPE_H7)) {

    clear_flash_cr_pg(sl, BANK_1);
    if ((sl->flash_type == STLINK_FLASH_TYPE_H7 &&
        sl->chip_flags & CHIP_F_HAS_DUAL_BANK) ||
        sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
      clear_flash_cr_pg(sl, BANK_2);
    }
    lock_flash(sl);
  } else if (sl->flash_type == STLINK_FLASH_TYPE_L0) {
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

int stlink_write_flash(stlink_t *sl, stm32_addr_t addr, uint8_t *base,
                       uint32_t len, uint8_t eraseonly) {
  int ret;
  flash_loader_t fl;
  ILOG("Attempting to write %d (%#x) bytes to stm32 address: %u (%#x)\n", len,
       len, addr, addr);
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
         (unsigned)(sl->flash_pgsz));
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

// TODO: length not checked
static uint8_t stlink_parse_hex(const char *hex) {
  uint8_t d[2];

  for (int i = 0; i < 2; ++i) {
    char c = *(hex + i);

    if (c >= '0' && c <= '9') {
      d[i] = c - '0';
    } else if (c >= 'A' && c <= 'F') {
      d[i] = c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
      d[i] = c - 'a' + 10;
    } else {
      return (0); // error
    }
  }

  return ((d[0] << 4) | (d[1]));
}

int stlink_parse_ihex(const char *path, uint8_t erased_pattern, uint8_t **mem,
                      size_t *size, uint32_t *begin) {
  int res = 0;
  *begin = UINT32_MAX;
  uint8_t *data = NULL;
  uint32_t end = 0;
  bool eof_found = false;

  for (int scan = 0; (res == 0) && (scan < 2); ++scan) {
    // parse file two times - first to find memory range, second - to fill it
    if (scan == 1) {
      if (!eof_found) {
        ELOG("No EoF recond\n");
        res = -1;
        break;
      }

      if (*begin >= end) {
        ELOG("No data found in file\n");
        res = -1;
        break;
      }

      *size = (end - *begin) + 1;
      data = calloc(*size, 1); // use calloc to get NULL if out of memory

      if (!data) {
        ELOG("Cannot allocate %u bytes\n", (unsigned)(*size));
        res = -1;
        break;
      }

      memset(data, erased_pattern, *size);
    }

    FILE *file = fopen(path, "r");

    if (!file) {
      ELOG("Cannot open file\n");
      res = -1;
      break;
    }

    uint32_t lba = 0;
    char line[1 + 5 * 2 + 255 * 2 + 2];

    while (fgets(line, sizeof(line), file)) {
      if (line[0] == '\n' || line[0] == '\r') {
        continue;
      } // skip empty lines

      if (line[0] != ':') { // no marker - wrong file format
        ELOG("Wrong file format - no marker\n");
        res = -1;
        break;
      }

      size_t l = strlen(line);

      while (l > 0 && (line[l - 1] == '\n' || line[l - 1] == '\r')) {
        --l;
      } // trim EoL

      if ((l < 11) ||
          (l ==
           (sizeof(line) - 1))) { // line too short or long - wrong file format
        ELOG("Wrong file format - wrong line length\n");
        res = -1;
        break;
      }

      uint8_t chksum = 0; // check sum

      for (size_t i = 1; i < l; i += 2) {
        chksum += stlink_parse_hex(line + i);
      }

      if (chksum != 0) {
        ELOG("Wrong file format - checksum mismatch\n");
        res = -1;
        break;
      }

      uint8_t reclen = stlink_parse_hex(line + 1);

      if (((uint32_t)reclen + 5) * 2 + 1 != l) {
        ELOG("Wrong file format - record length mismatch\n");
        res = -1;
        break;
      }

      uint16_t offset = ((uint16_t)stlink_parse_hex(line + 3) << 8) |
                        ((uint16_t)stlink_parse_hex(line + 5));
      uint8_t rectype = stlink_parse_hex(line + 7);

      switch (rectype) {
      case 0: /* Data */
        if (scan == 0) {
          uint32_t b = lba + offset;
          uint32_t e = b + reclen - 1;

          if (b < *begin) {
            *begin = b;
          }

          if (e > end) {
            end = e;
          }
        } else {
          for (uint8_t i = 0; i < reclen; ++i) {
            uint8_t b = stlink_parse_hex(line + 9 + i * 2);
            uint32_t addr = lba + offset + i;

            if (addr >= *begin && addr <= end) {
              data[addr - *begin] = b;
            }
          }
        }
        break;
      case 1: /* EoF */
        eof_found = true;
        break;
      case 2: /* Extended Segment Address, unexpected */
        res = -1;
        break;
      case 3: /* Start Segment Address, unexpected */
        res = -1;
        break;
      case 4: /* Extended Linear Address */
        if (reclen == 2) {
          lba = ((uint32_t)stlink_parse_hex(line + 9) << 24) |
                ((uint32_t)stlink_parse_hex(line + 11) << 16);
        } else {
          ELOG("Wrong file format - wrong LBA length\n");
          res = -1;
        }
        break;
      case 5: /* Start Linear Address - expected, but ignore */
        break;
      default:
        ELOG("Wrong file format - unexpected record type %d\n", rectype);
        res = -1;
      }

      if (res != 0) {
        break;
      }
    }

    fclose(file);
  }

  if (res == 0) {
    *mem = data;
  } else {
    free(data);
  }

  return (res);
}

uint8_t stlink_get_erased_pattern(stlink_t *sl) {
  if (sl->flash_type == STLINK_FLASH_TYPE_L0) {
    return (0x00);
  } else {
    return (0xff);
  }
}

int stlink_mwrite_flash(stlink_t *sl, uint8_t *data, uint32_t length,
                        stm32_addr_t addr) {
  /* Write the block in flash at addr */
  int err;
  unsigned int num_empty, idx;
  uint8_t erased_pattern = stlink_get_erased_pattern(sl);

  /*
   * This optimisation may cause unexpected garbage data remaining.
   * Therfore it is turned off by default.
   */
  if (sl->opt) {
    idx = (unsigned int)length;

    for (num_empty = 0; num_empty != length; ++num_empty)
      if (data[--idx] != erased_pattern) {
        break;
      }

    num_empty -= (num_empty & 3); // Round down to words

    if (num_empty != 0) {
      ILOG("Ignoring %d bytes of 0x%02x at end of file\n", num_empty,
           erased_pattern);
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
                           (num_empty == length) ? (uint32_t)length
                                                 : (uint32_t)length - num_empty,
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
int stlink_fwrite_flash(stlink_t *sl, const char *path, stm32_addr_t addr) {
  /* Write the file in flash at addr */
  int err;
  unsigned int num_empty, idx;
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
    idx = (unsigned int)mf.len;

    for (num_empty = 0; num_empty != mf.len; ++num_empty) {
      if (mf.base[--idx] != erased_pattern) {
        break;
      }
    }

    num_empty -= (num_empty & 3); // round down to words

    if (num_empty != 0) {
      ILOG("Ignoring %d bytes of 0x%02x at end of file\n", num_empty,
           erased_pattern);
    }
  } else {
    num_empty = 0;
  }

  /*
   * TODO: investigate a kind of weird behaviour here:
   * If the file is identified to be all-empty and four-bytes aligned,
   * still flash the whole file even if ignoring message is printed.
   */
  err = stlink_write_flash(sl, addr, mf.base,
                           (num_empty == mf.len) ? (uint32_t)mf.len
                                                 : (uint32_t)mf.len - num_empty,
                           num_empty == mf.len);
  stlink_fwrite_finalize(sl, addr);
  unmap_file(&mf);
  return (err);
}

/**
 * Write option bytes
 * @param sl
 * @param base option bytes to write
 * @param addr of the memory mapped option bytes
 * @param len of options bytes to write
 * @return 0 on success, -ve on failure.
 */
static int stlink_write_option_bytes_f0(
    stlink_t *sl, uint8_t* base, stm32_addr_t addr, uint32_t len) {
  int ret = 0;

  if (len < 12 || addr != STM32_F0_OPTION_BYTES_BASE) {
	WLOG("Only full write of option bytes area is supported\n");
    return -1;
  }

  clear_flash_error(sl);

  WLOG("Erasing option bytes\n");

  /* erase option bytes */
  stlink_write_debug32(sl, FLASH_CR, (1 << FLASH_CR_OPTER) | (1 << FLASH_CR_OPTWRE));
  ret = stlink_write_debug32(sl, FLASH_CR, (1 << FLASH_CR_OPTER) | (1 << FLASH_CR_STRT) | (1 << FLASH_CR_OPTWRE));
  if (ret) {
    return ret;
  }

  wait_flash_busy(sl);

  ret = check_flash_error(sl);
  if (ret) {
    return ret;
  }

  WLOG("Writing option bytes to %#10x\n", addr);

  /* Set the Option PG bit to enable programming */
  stlink_write_debug32(sl, FLASH_CR, (1 << FLASH_CR_OPTPG) | (1 << FLASH_CR_OPTWRE));

  /* Use flash loader for write OP
   * because flash memory writable by half word */
  flash_loader_t fl;
  ret = stlink_flash_loader_init(sl, &fl);
  if (ret) {
    return ret;
  }
  ret = stlink_flash_loader_run(sl, &fl, addr, base, len);
  if (ret) {
    return ret;
  }

  /* Reload option bytes */
  stlink_write_debug32(sl, FLASH_CR, (1 << FLASH_CR_OBL_LAUNCH));

  return check_flash_error(sl);
}

/**
 * Write option bytes
 * @param sl
 * @param addr of the memory mapped option bytes
 * @param base option bytes to write
 * @return 0 on success, -ve on failure.
 */
static int stlink_write_option_bytes_gx(stlink_t *sl, uint8_t *base,
                                        stm32_addr_t addr, uint32_t len) {
  /* Write options bytes */
  uint32_t val;
  int ret = 0;
  (void)len;
  uint32_t data;

  clear_flash_error(sl);

  write_uint32((unsigned char *)&data, *(uint32_t *)(base));
  WLOG("Writing option bytes %#10x to %#10x\n", data, addr);
  stlink_write_debug32(sl, STM32Gx_FLASH_OPTR, data);

  // Set Options Start bit
  stlink_read_debug32(sl, STM32Gx_FLASH_CR, &val);
  val |= (1 << STM32Gx_FLASH_CR_OPTSTRT);
  stlink_write_debug32(sl, STM32Gx_FLASH_CR, val);

  wait_flash_busy(sl);

  ret = check_flash_error(sl);

  // Reload options
  stlink_read_debug32(sl, STM32Gx_FLASH_CR, &val);
  val |= (1 << STM32Gx_FLASH_CR_OBL_LAUNCH);
  stlink_write_debug32(sl, STM32Gx_FLASH_CR, val);

  return (ret);
}

/**
 * Write option bytes
 * @param sl
 * @param addr of the memory mapped option bytes
 * @param base option bytes to write
 * @return 0 on success, -ve on failure.
 */
static int stlink_write_option_bytes_l0(stlink_t *sl, uint8_t *base,
                                        stm32_addr_t addr, uint32_t len) {
  uint32_t flash_base = get_stm32l0_flash_base(sl);
  uint32_t val;
  uint32_t data;
  int ret = 0;

  // Clear errors
  clear_flash_error(sl);

  while (len != 0) {
    write_uint32((unsigned char *)&data,
                 *(uint32_t *)(base)); // write options bytes

    WLOG("Writing option bytes %#10x to %#10x\n", data, addr);
    stlink_write_debug32(sl, addr, data);
    wait_flash_busy(sl);

    if ((ret = check_flash_error(sl))) {
      break;
    }

    len -= 4;
    addr += 4;
    base += 4;
  }

  // Reload options
  stlink_read_debug32(sl, flash_base + FLASH_PECR_OFF, &val);
  val |= (1 << STM32L0_FLASH_OBL_LAUNCH);
  stlink_write_debug32(sl, flash_base + FLASH_PECR_OFF, val);

  return (ret);
}

/**
 * Write option bytes
 * @param sl
 * @param addr of the memory mapped option bytes
 * @param base option bytes to write
 * @return 0 on success, -ve on failure.
 */
static int stlink_write_option_bytes_l4(stlink_t *sl, uint8_t *base,
                                        stm32_addr_t addr, uint32_t len) {

  uint32_t val;
  int ret = 0;
  (void)addr;
  (void)len;

  // Clear errors
  clear_flash_error(sl);

  // write options bytes
  uint32_t data;
  write_uint32((unsigned char *)&data, *(uint32_t *)(base));
  WLOG("Writing option bytes 0x%04x\n", data);
  stlink_write_debug32(sl, STM32L4_FLASH_OPTR, data);

  // set options start bit
  stlink_read_debug32(sl, STM32L4_FLASH_CR, &val);
  val |= (1 << STM32L4_FLASH_CR_OPTSTRT);
  stlink_write_debug32(sl, STM32L4_FLASH_CR, val);

  wait_flash_busy(sl);
  ret = check_flash_error(sl);

  // apply options bytes immediate
  stlink_read_debug32(sl, STM32L4_FLASH_CR, &val);
  val |= (1 << STM32L4_FLASH_CR_OBL_LAUNCH);
  stlink_write_debug32(sl, STM32L4_FLASH_CR, val);

  return (ret);
}

/**
 * Write option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
static int stlink_write_option_bytes_f4(stlink_t *sl, uint8_t *base,
                                        stm32_addr_t addr, uint32_t len) {
  uint32_t option_byte;
  int ret = 0;
  (void)addr;
  (void)len;

  // Clear errors
  clear_flash_error(sl);

  write_uint32((unsigned char *)&option_byte, *(uint32_t *)(base));

  // write option byte, ensuring we dont lock opt, and set strt bit
  stlink_write_debug32(sl, FLASH_F4_OPTCR,
                       (option_byte & ~(1 << FLASH_F4_OPTCR_LOCK)) |
                           (1 << FLASH_F4_OPTCR_START));

  wait_flash_busy(sl);
  ret = check_flash_error(sl);

  // option bytes are reloaded at reset only, no obl. */
  return (ret);
}

/**
 * Write option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
static int stlink_write_option_bytes_f7(stlink_t *sl, uint8_t *base,
                                        stm32_addr_t addr, uint32_t len) {
  uint32_t option_byte;
  int ret = 0;

  // Clear errors
  clear_flash_error(sl);

  ILOG("Asked to write option byte %#10x to %#010x.\n", *(uint32_t *)(base),
       addr);
  write_uint32((unsigned char *)&option_byte, *(uint32_t *)(base));
  ILOG("Write %d option bytes %#010x to %#010x!\n", len, option_byte, addr);

  if (addr == 0) {
    addr = FLASH_F7_OPTCR;
    ILOG("No address provided, using %#10x\n", addr);
  }

  if (addr == FLASH_F7_OPTCR) {
    /* write option byte, ensuring we dont lock opt, and set strt bit */
    stlink_write_debug32(sl, FLASH_F7_OPTCR,
                         (option_byte & ~(1 << FLASH_F7_OPTCR_LOCK)) |
                             (1 << FLASH_F7_OPTCR_START));
  } else if (addr == FLASH_F7_OPTCR1) {
    // Read FLASH_F7_OPTCR
    uint32_t oldvalue;
    stlink_read_debug32(sl, FLASH_F7_OPTCR, &oldvalue);
    /* write option byte */
    stlink_write_debug32(sl, FLASH_F7_OPTCR1, option_byte);
    // Write FLASH_F7_OPTCR lock and start address
    stlink_write_debug32(sl, FLASH_F7_OPTCR,
                         (oldvalue & ~(1 << FLASH_F7_OPTCR_LOCK)) |
                             (1 << FLASH_F7_OPTCR_START));
  } else {
    WLOG("WIP: write %#010x to address %#010x\n", option_byte, addr);
    stlink_write_debug32(sl, addr, option_byte);
  }

  wait_flash_busy(sl);

  ret = check_flash_error(sl);
  if (!ret)
    ILOG("Wrote %d option bytes %#010x to %#010x!\n", len, *(uint32_t *)base,
         addr);

  /* option bytes are reloaded at reset only, no obl. */

  return ret;
}

/**
 * Write STM32H7xx option bytes
 * @param sl
 * @param base option bytes to write
 * @param addr of the memory mapped option bytes
 * @param len number of bytes to write (must be multiple of 4)
 * @return 0 on success, -ve on failure.
 */
static int stlink_write_option_bytes_h7(stlink_t *sl, uint8_t *base,
                                        stm32_addr_t addr, uint32_t len) {
  uint32_t val;
  uint32_t data;

  // Wait until previous flash option has completed
  wait_flash_busy(sl);

  // Clear previous error
  stlink_write_debug32(sl, FLASH_H7_OPTCCR,
                       1 << FLASH_H7_OPTCCR_CLR_OPTCHANGEERR);

  while (len != 0) {
    switch (addr) {
    case FLASH_H7_REGS_ADDR + 0x20: // FLASH_OPTSR_PRG
    case FLASH_H7_REGS_ADDR + 0x2c: // FLASH_PRAR_PRG1
    case FLASH_H7_REGS_ADDR + 0x34: // FLASH_SCAR_PRG1
    case FLASH_H7_REGS_ADDR + 0x3c: // FLASH_WPSN_PRG1
    case FLASH_H7_REGS_ADDR + 0x44: // FLASH_BOOT_PRG
      /* Write to FLASH_xxx_PRG registers */
      write_uint32((unsigned char *)&data,
                   *(uint32_t *)(base)); // write options bytes

      WLOG("Writing option bytes %#10x to %#10x\n", data, addr);

      /* Skip if the value in the CUR register is identical */
      stlink_read_debug32(sl, addr - 4, &val);
      if (val == data) {
        break;
      }

      /* Write new option byte values and start modification */
      stlink_write_debug32(sl, addr, data);
      stlink_read_debug32(sl, FLASH_H7_OPTCR, &val);
      val |= (1 << FLASH_H7_OPTCR_OPTSTART);
      stlink_write_debug32(sl, FLASH_H7_OPTCR, val);

      /* Wait for the option bytes modification to complete */
      do {
        stlink_read_debug32(sl, FLASH_H7_OPTSR_CUR, &val);
      } while ((val & (1 << FLASH_H7_OPTSR_OPT_BUSY)) != 0);

      /* Check for errors */
      if ((val & (1 << FLASH_H7_OPTSR_OPTCHANGEERR)) != 0) {
        stlink_write_debug32(sl, FLASH_H7_OPTCCR,
                             1 << FLASH_H7_OPTCCR_CLR_OPTCHANGEERR);
        return -1;
      }
      break;

    default:
      /* Skip non-programmable registers */
      break;
    }

    len -= 4;
    addr += 4;
    base += 4;
  }

  return 0;
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register_Gx(stlink_t *sl,
                                           uint32_t *option_byte) {
  return stlink_read_debug32(sl, STM32Gx_FLASH_OPTR, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_bytes_Gx(stlink_t *sl, uint32_t *option_byte) {
  return stlink_read_option_control_register_Gx(sl, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register_f2(stlink_t *sl,
                                           uint32_t *option_byte) {
  return stlink_read_debug32(sl, FLASH_F2_OPT_CR, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_bytes_f2(stlink_t *sl, uint32_t *option_byte) {
  return stlink_read_option_control_register_f2(sl, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register_f4(stlink_t *sl,
                                           uint32_t *option_byte) {
  return stlink_read_debug32(sl, FLASH_F4_OPTCR, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_bytes_f4(stlink_t *sl, uint32_t *option_byte) {
  return stlink_read_option_control_register_f4(sl, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register_f7(stlink_t *sl,
                                           uint32_t *option_byte) {
  DLOG("@@@@ Read option control register byte from %#10x\n", FLASH_F7_OPTCR);
  return stlink_read_debug32(sl, FLASH_F7_OPTCR, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register_f0(stlink_t *sl,
                                           uint32_t *option_byte) {
  DLOG("@@@@ Read option control register byte from %#10x\n", FLASH_OBR);
  return stlink_read_debug32(sl, FLASH_OBR, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register1_f7(stlink_t *sl,
                                            uint32_t *option_byte) {
  DLOG("@@@@ Read option control register 1 byte from %#10x\n",
       FLASH_F7_OPTCR1);
  return stlink_read_debug32(sl, FLASH_F7_OPTCR1, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_bytes_boot_add_f7(stlink_t *sl, uint32_t *option_byte) {
  DLOG("@@@@ Read option byte boot address\n");
  return stlink_read_option_control_register1_f7(sl, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 *
 * Since multiple bytes can be read, we read and print all but one here
 * and then return the last one just like other devices
 */
int stlink_read_option_bytes_f7(stlink_t *sl, uint32_t *option_byte) {
  int err = -1;
  for (uint32_t counter = 0; counter < (sl->option_size / 4 - 1); counter++) {
    err = stlink_read_debug32(sl, sl->option_base + counter * sizeof(uint32_t),
                              option_byte);
    if (err == -1) {
      return err;
    } else {
      printf("%08x\n", *option_byte);
    }
  }

  return stlink_read_debug32(
      sl,
      sl->option_base + (uint32_t)(sl->option_size / 4 - 1) * sizeof(uint32_t),
      option_byte);
}

/**
 * Read first option bytes
 * @param sl
 * @param option_byte option value
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_bytes_generic(stlink_t *sl, uint32_t *option_byte) {
  DLOG("@@@@ Read option bytes boot address from %#10x\n", sl->option_base);
  return stlink_read_debug32(sl, sl->option_base, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
// int stlink_read_option_bytes_boot_add_generic(stlink_t *sl, uint32_t*
// option_byte) {
//    DLOG("@@@@ Read option bytes boot address from %#10x\n", sl->option_base);
//    return stlink_read_debug32(sl, sl->option_base, option_byte);
//}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
// int stlink_read_option_control_register_generic(stlink_t *sl, uint32_t*
// option_byte) {
//    DLOG("@@@@ Read option control register byte from %#10x\n",
//    sl->option_base); return stlink_read_debug32(sl, sl->option_base,
//    option_byte);
//}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
// int stlink_read_option_control_register1_generic(stlink_t *sl, uint32_t*
// option_byte) {
//    DLOG("@@@@ Read option control register 1 byte from %#10x\n",
//    sl->option_base); return stlink_read_debug32(sl, sl->option_base,
//    option_byte);
//}

/**
 * Read option bytes
 * @param sl
 * @param option_byte option value
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_bytes32(stlink_t *sl, uint32_t *option_byte) {
  if (sl->option_base == 0) {
    ELOG("Option bytes read is currently not supported for connected chip\n");
    return (-1);
  }

  switch (sl->chip_id) {
  case STLINK_CHIPID_STM32_F2:
    return stlink_read_option_bytes_f2(sl, option_byte);
  case STLINK_CHIPID_STM32_F4:
  case STLINK_CHIPID_STM32_F446:
    return stlink_read_option_bytes_f4(sl, option_byte);
  case STLINK_CHIPID_STM32_F76xxx:
    return stlink_read_option_bytes_f7(sl, option_byte);
  case STLINK_CHIPID_STM32_G0_CAT1:
  case STLINK_CHIPID_STM32_G0_CAT2:
  case STLINK_CHIPID_STM32_G4_CAT2:
  case STLINK_CHIPID_STM32_G4_CAT3:
    return stlink_read_option_bytes_Gx(sl, option_byte);
  default:
    return stlink_read_option_bytes_generic(sl, option_byte);
  }
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte option value
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_bytes_boot_add32(stlink_t *sl, uint32_t *option_byte) {
  if (sl->option_base == 0) {
    ELOG("Option bytes boot address read is currently not supported for "
         "connected chip\n");
    return -1;
  }

  switch (sl->flash_type) {
  case STLINK_FLASH_TYPE_F7:
    return stlink_read_option_bytes_boot_add_f7(sl, option_byte);
  default:
    return -1;
    // return stlink_read_option_bytes_boot_add_generic(sl, option_byte);
  }
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte option value
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register32(stlink_t *sl, uint32_t *option_byte) {
  if (sl->option_base == 0) {
    ELOG("Option bytes read is currently not supported for connected chip\n");
    return -1;
  }

  switch (sl->flash_type) {
  case STLINK_FLASH_TYPE_F0:
  case STLINK_FLASH_TYPE_F1_XL:
    return stlink_read_option_control_register_f0(sl, option_byte);
  case STLINK_FLASH_TYPE_F7:
    return stlink_read_option_control_register_f7(sl, option_byte);
  default:
    return -1;
  }
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte option value
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register1_32(stlink_t *sl,
                                            uint32_t *option_byte) {
  if (sl->option_base == 0) {
    ELOG("Option bytes read is currently not supported for connected chip\n");
    return -1;
  }

  switch (sl->flash_type) {
  case STLINK_FLASH_TYPE_F7:
    return stlink_read_option_control_register1_f7(sl, option_byte);
  default:
    return -1;
    // return stlink_read_option_control_register1_generic(sl, option_byte);
  }
}

/**
 * Write option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
int stlink_write_option_bytes32(stlink_t *sl, uint32_t option_byte) {
  WLOG("About to write option byte %#10x to %#10x.\n", option_byte,
       sl->option_base);
  return stlink_write_option_bytes(sl, sl->option_base, (uint8_t *)&option_byte,
                                   4);
}

/**
 * Write option bytes
 * @param sl
 * @param addr of the memory mapped option bytes
 * @param base option bytes to write
 * @return 0 on success, -ve on failure.
 */
int stlink_write_option_bytes(stlink_t *sl, stm32_addr_t addr, uint8_t *base,
                              uint32_t len) {
  int ret = -1;

  if (sl->option_base == 0) {
    ELOG(
        "Option bytes writing is currently not supported for connected chip\n");
    return (-1);
  }

  if ((addr < sl->option_base) || addr > sl->option_base + sl->option_size) {
    ELOG("Option bytes start address out of Option bytes range\n");
    return (-1);
  }

  if (addr + len > sl->option_base + sl->option_size) {
    ELOG("Option bytes data too long\n");
    return (-1);
  }

  wait_flash_busy(sl);

  if (unlock_flash_if(sl)) {
    ELOG("Flash unlock failed! System reset required to be able to unlock it "
         "again!\n");
    return (-1);
  }

  if (unlock_flash_option_if(sl)) {
    ELOG("Flash option unlock failed!\n");
    return (-1);
  }

  switch (sl->flash_type) {
  case STLINK_FLASH_TYPE_F0:
  case STLINK_FLASH_TYPE_F1_XL:
    ret = stlink_write_option_bytes_f0(sl, base, addr, len);
    break;
  case STLINK_FLASH_TYPE_F4:
    ret = stlink_write_option_bytes_f4(sl, base, addr, len);
    break;
  case STLINK_FLASH_TYPE_F7:
    ret = stlink_write_option_bytes_f7(sl, base, addr, len);
    break;
  case STLINK_FLASH_TYPE_L0:
    ret = stlink_write_option_bytes_l0(sl, base, addr, len);
    break;
  case STLINK_FLASH_TYPE_L4:
    ret = stlink_write_option_bytes_l4(sl, base, addr, len);
    break;
  case STLINK_FLASH_TYPE_G0:
  case STLINK_FLASH_TYPE_G4:
    ret = stlink_write_option_bytes_gx(sl, base, addr, len);
    break;
  case STLINK_FLASH_TYPE_H7:
    ret = stlink_write_option_bytes_h7(sl, base, addr, len);
    break;
  default:
    ELOG("Option bytes writing is currently not implemented for connected "
         "chip\n");
    break;
  }

  if (ret) {
    ELOG("Flash option write failed!\n");
  } else {
    ILOG("Wrote %d option bytes to %#010x!\n", len, addr);
  }

  /* Re-lock flash. */
  lock_flash_option(sl);
  lock_flash(sl);

  return ret;
}

/**
 * Write option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
static int
stlink_write_option_control_register_f7(stlink_t *sl,
                                        uint32_t option_control_register) {
  int ret = 0;

  // Clear errors
  clear_flash_error(sl);

  ILOG("Asked to write option control register 1 %#10x to %#010x.\n",
       option_control_register, FLASH_F7_OPTCR);

  /* write option byte, ensuring we dont lock opt, and set strt bit */
  stlink_write_debug32(sl, FLASH_F7_OPTCR,
                       (option_control_register & ~(1 << FLASH_F7_OPTCR_LOCK)) |
                           (1 << FLASH_F7_OPTCR_START));

  wait_flash_busy(sl);

  ret = check_flash_error(sl);
  if (!ret)
    ILOG("Wrote option bytes %#010x to %#010x!\n", option_control_register,
         FLASH_F7_OPTCR);

  return ret;
}


/**
 * Write option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
static int
stlink_write_option_control_register_f0(stlink_t *sl,
                                        uint32_t option_control_register) {
  int ret = 0;
  uint16_t opt_val[8];
  unsigned protection, optiondata;
  uint16_t user_options, user_data, rdp;
  unsigned option_offset, user_data_offset;

  ILOG("Asked to write option control register %#10x to %#010x.\n",
       option_control_register, FLASH_OBR);

  /* Clear errors */
  clear_flash_error(sl);

  /* Retrieve current values */
  ret = stlink_read_debug32(sl, FLASH_OBR, &optiondata);
  if (ret) {
    return ret;
  }
  ret = stlink_read_debug32(sl, FLASH_WRPR, &protection);
  if (ret) {
    return ret;
  }

  /* Translate OBR value to flash store structure
   * F0: RM0091, Option byte description, pp. 75-78
   * F1: PM0075, Option byte description, pp. 19-22
   * F3: RM0316, Option byte description, pp. 85-87 */
  switch(sl->chip_id)
  {
  case 0x422: /* STM32F30x */
  case 0x432: /* STM32F37x */
  case 0x438: /* STM32F303x6/8 and STM32F328 */
  case 0x446: /* STM32F303xD/E and STM32F398xE */
  case 0x439: /* STM32F302x6/8 */
  case 0x440: /* STM32F05x */
  case 0x444: /* STM32F03x */
  case 0x445: /* STM32F04x */
  case 0x448: /* STM32F07x */
  case 0x442: /* STM32F09x */
    option_offset = 6;
    user_data_offset = 16;
    rdp = 0x55AA;
    break;
  default:
    option_offset = 0;
    user_data_offset = 10;
    rdp = 0x5AA5;
    break;
  }

  user_options = (option_control_register >> option_offset >> 2) & 0xFFFF;
  user_data = (option_control_register >> user_data_offset) & 0xFFFF;

#define VAL_WITH_COMPLEMENT(v) (uint16_t)(((v)&0xFF) | (((~(v))<<8)&0xFF00))

  opt_val[0] = (option_control_register & (1 << 1/*OPT_READOUT*/)) ? 0xFFFF : rdp;
  opt_val[1] = VAL_WITH_COMPLEMENT(user_options);
  opt_val[2] = VAL_WITH_COMPLEMENT(user_data);
  opt_val[3] = VAL_WITH_COMPLEMENT(user_data >> 8);
  opt_val[4] = VAL_WITH_COMPLEMENT(protection);
  opt_val[5] = VAL_WITH_COMPLEMENT(protection >> 8);
  opt_val[6] = VAL_WITH_COMPLEMENT(protection >> 16);
  opt_val[7] = VAL_WITH_COMPLEMENT(protection >> 24);

#undef VAL_WITH_COMPLEMENT

  /* Write bytes and check errors */
  ret = stlink_write_option_bytes_f0(sl, (uint8_t*)opt_val, STM32_F0_OPTION_BYTES_BASE, sizeof(opt_val));
  if (ret)
    return ret;

  ret = check_flash_error(sl);
  if (!ret) {
    ILOG("Wrote option bytes %#010x to %#010x!\n", option_control_register,
         FLASH_OBR);
  }

  return ret;
}

/**
 * Write option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
static int
stlink_write_option_control_register1_f7(stlink_t *sl,
                                         uint32_t option_control_register1) {
  int ret = 0;

  // Clear errors
  clear_flash_error(sl);

  ILOG("Asked to write option control register 1 %#010x to %#010x.\n",
       option_control_register1, FLASH_F7_OPTCR1);

  /* write option byte, ensuring we dont lock opt, and set strt bit */
  uint32_t current_control_register_value;
  stlink_read_debug32(sl, FLASH_F7_OPTCR, &current_control_register_value);

  /* write option byte */
  stlink_write_debug32(sl, FLASH_F7_OPTCR1, option_control_register1);
  stlink_write_debug32(
      sl, FLASH_F7_OPTCR,
      (current_control_register_value & ~(1 << FLASH_F7_OPTCR_LOCK)) |
          (1 << FLASH_F7_OPTCR_START));

  wait_flash_busy(sl);

  ret = check_flash_error(sl);
  if (!ret)
    ILOG("Wrote option bytes %#010x to %#010x!\n", option_control_register1,
         FLASH_F7_OPTCR1);

  return ret;
}

/**
 * Write option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
static int
stlink_write_option_bytes_boot_add_f7(stlink_t *sl,
                                      uint32_t option_byte_boot_add) {
  ILOG("Asked to write option byte boot add %#010x.\n", option_byte_boot_add);
  return stlink_write_option_control_register1_f7(sl, option_byte_boot_add);
}

/**
 * Write option bytes
 * @param sl
 * @param option bytes boot address to write
 * @return 0 on success, -ve on failure.
 */
int stlink_write_option_bytes_boot_add32(stlink_t *sl,
                                         uint32_t option_bytes_boot_add) {
  int ret = -1;

  wait_flash_busy(sl);

  if (unlock_flash_if(sl)) {
    ELOG("Flash unlock failed! System reset required to be able to unlock it "
         "again!\n");
    return -1;
  }

  if (unlock_flash_option_if(sl)) {
    ELOG("Flash option unlock failed!\n");
    return -1;
  }

  switch (sl->flash_type) {
  case STLINK_FLASH_TYPE_F7:
    ret = stlink_write_option_bytes_boot_add_f7(sl, option_bytes_boot_add);
    break;
  default:
    ELOG("Option bytes boot address writing is currently not implemented for "
         "connected chip\n");
    break;
  }

  if (ret)
    ELOG("Flash option write failed!\n");
  else
    ILOG("Wrote option bytes boot address %#010x!\n", option_bytes_boot_add);

  /* Re-lock flash. */
  lock_flash_option(sl);
  lock_flash(sl);

  return ret;
}

/**
 * Write option bytes
 * @param sl
 * @param option bytes boot address to write
 * @return 0 on success, -ve on failure.
 */
int stlink_write_option_control_register32(stlink_t *sl,
                                           uint32_t option_control_register) {
  int ret = -1;

  wait_flash_busy(sl);

  if (unlock_flash_if(sl)) {
    ELOG("Flash unlock failed! System reset required to be able to unlock it "
         "again!\n");
    return -1;
  }

  if (unlock_flash_option_if(sl)) {
    ELOG("Flash option unlock failed!\n");
    return -1;
  }

  switch (sl->flash_type) {
  case STLINK_FLASH_TYPE_F0:
  case STLINK_FLASH_TYPE_F1_XL:
    ret = stlink_write_option_control_register_f0(sl, option_control_register);
    break;
  case STLINK_FLASH_TYPE_F7:
    ret = stlink_write_option_control_register_f7(sl, option_control_register);
    break;
  default:
    ELOG("Option control register writing is currently not implemented for "
         "connected chip\n");
    break;
  }

  if (ret)
    ELOG("Flash option write failed!\n");
  else
    ILOG("Wrote option control register %#010x!\n", option_control_register);

  /* Re-lock flash. */
  lock_flash_option(sl);
  lock_flash(sl);

  return ret;
}

/**
 * Write option bytes
 * @param sl
 * @param option bytes boot address to write
 * @return 0 on success, -ve on failure.
 */
int stlink_write_option_control_register1_32(
    stlink_t *sl, uint32_t option_control_register1) {
  int ret = -1;

  wait_flash_busy(sl);

  if (unlock_flash_if(sl)) {
    ELOG("Flash unlock failed! System reset required to be able to unlock it "
         "again!\n");
    return -1;
  }

  if (unlock_flash_option_if(sl)) {
    ELOG("Flash option unlock failed!\n");
    return -1;
  }

  switch (sl->flash_type) {
  case STLINK_FLASH_TYPE_F7:
    ret =
        stlink_write_option_control_register1_f7(sl, option_control_register1);
    break;
  default:
    ELOG("Option control register 1 writing is currently not implemented for "
         "connected chip\n");
    break;
  }

  if (ret)
    ELOG("Flash option write failed!\n");
  else
    ILOG("Wrote option control register 1 %#010x!\n", option_control_register1);

  lock_flash_option(sl);
  lock_flash(sl);

  return (ret);
}

/**
 * Write the given binary file with option bytes
 * @param sl
 * @param path readable file path, should be binary image
 * @param addr of the memory mapped option bytes
 * @return 0 on success, -ve on failure.
 */
int stlink_fwrite_option_bytes(stlink_t *sl, const char *path,
                               stm32_addr_t addr) {
  /* Write the file in flash at addr */
  int err;
  mapped_file_t mf = MAPPED_FILE_INITIALIZER;

  if (map_file(&mf, path) == -1) {
    ELOG("map_file() == -1\n");
    return (-1);
  }

  printf("file %s ", path);
  md5_calculate(&mf);
  stlink_checksum(&mf);

  err = stlink_write_option_bytes(sl, addr, mf.base, (uint32_t)mf.len);
  stlink_fwrite_finalize(sl, addr);
  unmap_file(&mf);

  return (err);
}

int stlink_target_connect(stlink_t *sl, enum connect_type connect) {
  if (connect == CONNECT_UNDER_RESET) {
    stlink_enter_swd_mode(sl);

    stlink_jtag_reset(sl, STLINK_DEBUG_APIV2_DRIVE_NRST_LOW);

    // try to halt the core before reset
    // this is useful if the NRST pin is not connected
    sl->backend->force_debug(sl);

    // minimum reset pulse duration of 20 us (RM0008, 8.1.2 Power reset)
    usleep(20);

    stlink_jtag_reset(sl, STLINK_DEBUG_APIV2_DRIVE_NRST_HIGH);

    // try to halt the core after reset
    unsigned timeout = time_ms() + 10;
    while (time_ms() < timeout) {
      sl->backend->force_debug(sl);
      usleep(100);
    }

    // check NRST connection
    uint32_t dhcsr = 0;
    stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr);
    if ((dhcsr & STLINK_REG_DHCSR_S_RESET_ST) == 0) {
      WLOG("NRST is not connected\n");
    }

    // addition soft reset for halt before the first instruction
    stlink_soft_reset(sl, 1 /* halt on reset */);
  }

  if (stlink_current_mode(sl) != STLINK_DEV_DEBUG_MODE &&
        stlink_enter_swd_mode(sl)) {
    printf("Failed to enter SWD mode\n");
    return -1;
  }

  if (connect == CONNECT_NORMAL) {
    stlink_reset(sl, RESET_AUTO);
  }

  return stlink_load_device_params(sl);
}
