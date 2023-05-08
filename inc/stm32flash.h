#ifndef STM32FLASH_H
#define STM32FLASH_H

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

#define FLASH_ACR_OFF ((uint32_t)0x00)
#define FLASH_PECR_OFF ((uint32_t)0x04)
#define FLASH_PDKEYR_OFF ((uint32_t)0x08)
#define FLASH_PEKEYR_OFF ((uint32_t)0x0c)
#define FLASH_PRGKEYR_OFF ((uint32_t)0x10)
#define FLASH_OPTKEYR_OFF ((uint32_t)0x14)
#define FLASH_SR_OFF ((uint32_t)0x18)
#define FLASH_OBR_OFF ((uint32_t)0x1c)
#define FLASH_WRPR_OFF ((uint32_t)0x20)

// == STM32F0 ==
#define FLASH_F0_OPTKEY1 0x45670123
#define FLASH_F0_OPTKEY2 0xcdef89ab

// == STM32F2 ==
#define FLASH_F2_REGS_ADDR ((uint32_t)0x40023c00)
#define FLASH_F2_KEYR (FLASH_F2_REGS_ADDR + 0x04)
#define FLASH_F2_OPT_KEYR (FLASH_F2_REGS_ADDR + 0x08)
#define FLASH_F2_SR (FLASH_F2_REGS_ADDR + 0x0c)
#define FLASH_F2_CR (FLASH_F2_REGS_ADDR + 0x10)
#define FLASH_F2_OPT_CR (FLASH_F2_REGS_ADDR + 0x14)
#define FLASH_F2_OPT_LOCK_BIT (1u << 0)

// F2 Flash control register
#define FLASH_F2_CR_STRT 16
#define FLASH_F2_CR_LOCK 31
#define FLASH_F2_CR_SER 1
#define FLASH_F2_CR_SNB 3
#define FLASH_F2_CR_SNB_MASK 0x78

// F2 Flash status register
#define FLASH_F2_SR_BSY 16

// == STM32F4 ==
// F4 Flash registers
#define FLASH_F4_REGS_ADDR ((uint32_t)0x40023c00)
#define FLASH_F4_KEYR (FLASH_F4_REGS_ADDR + 0x04)
#define FLASH_F4_OPT_KEYR (FLASH_F4_REGS_ADDR + 0x08)
#define FLASH_F4_SR (FLASH_F4_REGS_ADDR + 0x0c)
#define FLASH_F4_CR (FLASH_F4_REGS_ADDR + 0x10)
#define FLASH_F4_OPTCR (FLASH_F4_REGS_ADDR + 0x14)
#define FLASH_F4_OPTCR_LOCK 0
#define FLASH_F4_OPTCR_START 1

// F4 Flash control register
#define FLASH_F4_CR_STRT 16
#define FLASH_F4_CR_LOCK 31
#define FLASH_F4_CR_SER 1
#define FLASH_F4_CR_SNB 3
#define FLASH_F4_CR_SNB_MASK 0xf8

// F4 Flash status register
#define FLASH_F4_SR_ERROR_MASK 0x000000F0
#define FLASH_F4_SR_PGAERR 5
#define FLASH_F4_SR_WRPERR 4
#define FLASH_F4_SR_BSY 16

// == STM32F7 ==
// F7 Flash registers
#define FLASH_F7_REGS_ADDR ((uint32_t)0x40023c00)
#define FLASH_F7_KEYR (FLASH_F7_REGS_ADDR + 0x04)
#define FLASH_F7_OPT_KEYR (FLASH_F7_REGS_ADDR + 0x08)
#define FLASH_F7_SR (FLASH_F7_REGS_ADDR + 0x0c)
#define FLASH_F7_CR (FLASH_F7_REGS_ADDR + 0x10)
#define FLASH_F7_OPTCR (FLASH_F7_REGS_ADDR + 0x14)
#define FLASH_F7_OPTCR1 (FLASH_F7_REGS_ADDR + 0x18)
#define FLASH_F7_OPTCR_LOCK 0
#define FLASH_F7_OPTCR_START 1
#define FLASH_F7_OPTCR1_BOOT_ADD0 0
#define FLASH_F7_OPTCR1_BOOT_ADD1 16

// F7 Flash control register
#define FLASH_F7_CR_STRT 16
#define FLASH_F7_CR_LOCK 31
#define FLASH_F7_CR_SER 1
#define FLASH_F7_CR_SNB 3
#define FLASH_F7_CR_SNB_MASK 0xf8

// F7 Flash status register
#define FLASH_F7_SR_BSY 16
#define FLASH_F7_SR_ERS_ERR 7 /* Erase Sequence Error */
#define FLASH_F7_SR_PGP_ERR 6 /* Programming parallelism error */
#define FLASH_F7_SR_PGA_ERR 5 /* Programming alignment error */
#define FLASH_F7_SR_WRP_ERR 4 /* Write protection error */
#define FLASH_F7_SR_OP_ERR 1  /* Operation error */
#define FLASH_F7_SR_EOP 0     /* End of operation */
#define FLASH_F7_SR_ERROR_MASK                               \
  ((1 << FLASH_F7_SR_ERS_ERR) | (1 << FLASH_F7_SR_PGP_ERR) | \
   (1 << FLASH_F7_SR_PGA_ERR) | (1 << FLASH_F7_SR_WRP_ERR) | \
   (1 << FLASH_F7_SR_OP_ERR))

// == STM32G0/G4 ==
// G0/G4 Flash registers (RM0440, p.146)
#define FLASH_Gx_REGS_ADDR ((uint32_t)0x40022000)
#define FLASH_Gx_ACR (FLASH_Gx_REGS_ADDR + 0x00)
#define FLASH_Gx_KEYR (FLASH_Gx_REGS_ADDR + 0x08)
#define FLASH_Gx_OPTKEYR (FLASH_Gx_REGS_ADDR + 0x0c)
#define FLASH_Gx_SR (FLASH_Gx_REGS_ADDR + 0x10)
#define FLASH_Gx_CR (FLASH_Gx_REGS_ADDR + 0x14)
#define FLASH_Gx_ECCR (FLASH_Gx_REGS_ADDR + 0x18)
#define FLASH_Gx_OPTR (FLASH_Gx_REGS_ADDR + 0x20)

// G0/G4 Flash control register
#define FLASH_Gx_CR_PG (0)          /* Program */
#define FLASH_Gx_CR_PER (1)         /* Page erase */
#define FLASH_Gx_CR_MER1 (2)        /* Mass erase */
#define FLASH_Gx_CR_PNB (3)         /* Page number */
#define FLASH_G0_CR_PNG_LEN (5)     /* STM32G0: 5 page number bits */
#define FLASH_G4_CR_PNG_LEN (7)     /* STM32G4: 7 page number bits */
#define FLASH_Gx_CR_MER2 (15)       /* Mass erase (2nd bank)*/
#define FLASH_Gx_CR_STRT (16)       /* Start */
#define FLASH_Gx_CR_OPTSTRT (17)    /* Start of modification of option bytes */
#define FLASH_Gx_CR_FSTPG (18)      /* Fast programming */
#define FLASH_Gx_CR_EOPIE (24)      /* End of operation interrupt enable */
#define FLASH_Gx_CR_ERRIE (25)      /* Error interrupt enable */
#define FLASH_Gx_CR_OBL_LAUNCH (27) /* Forces the option byte loading */
#define FLASH_Gx_CR_OPTLOCK (30)    /* Options Lock */
#define FLASH_Gx_CR_LOCK (31)       /* FLASH_CR Lock */

// G0/G4 Flash status register
#define FLASH_Gx_SR_ERROR_MASK (0x3fa)
#define FLASH_Gx_SR_PROGERR (3)
#define FLASH_Gx_SR_WRPERR (4)
#define FLASH_Gx_SR_PGAERR (5)
#define FLASH_Gx_SR_BSY (16) /* FLASH_SR Busy */
#define FLASH_Gx_SR_EOP (0)  /* FLASH_EOP End of Operation */

// == STM32G0 == (RM0444 Table 1, sec. 3.7)
// Mostly the same as G4 chips, but the notation
// varies a bit after the 'OPTR' register.
#define FLASH_G0_REGS_ADDR (FLASH_Gx_REGS_ADDR)
#define FLASH_G0_PCROP1ASR (FLASH_G0_REGS_ADDR + 0x24)
#define FLASH_G0_PCROP1AER (FLASH_G0_REGS_ADDR + 0x28)
#define FLASH_G0_WRP1AR (FLASH_G0_REGS_ADDR + 0x2c)
#define FLASH_G0_WRP1BR (FLASH_G0_REGS_ADDR + 0x30)
#define FLASH_G0_PCROP1BSR (FLASH_G0_REGS_ADDR + 0x34)
#define FLASH_G0_PCROP1BER (FLASH_G0_REGS_ADDR + 0x38)
#define FLASH_G0_SECR (FLASH_G0_REGS_ADDR + 0x80)

// == STM32G4 == (RM0440 Table 17, sec. 3.7.19)

#define FLASH_G4_OPTR_DBANK (22) /* FLASH option register FLASH_OPTR Dual-Bank Mode */

// There are a few extra registers because 'cat 3' devices can have
// two Flash banks.
#define FLASH_G4_REGS_ADDR (FLASH_Gx_REGS_ADDR)
#define FLASH_G4_PDKEYR (FLASH_G4_REGS_ADDR + 0x04)
#define FLASH_G4_PCROP1SR (FLASH_G4_REGS_ADDR + 0x24)
#define FLASH_G4_PCROP1ER (FLASH_G4_REGS_ADDR + 0x28)
#define FLASH_G4_WRP1AR (FLASH_G4_REGS_ADDR + 0x2c)
#define FLASH_G4_WRP1BR (FLASH_G4_REGS_ADDR + 0x30)
#define FLASH_G4_PCROP2SR (FLASH_G4_REGS_ADDR + 0x44)
#define FLASH_G4_PCROP2ER (FLASH_G4_REGS_ADDR + 0x48)
#define FLASH_G4_WRP2AR (FLASH_G4_REGS_ADDR + 0x4c)
#define FLASH_G4_WRP2BR (FLASH_G4_REGS_ADDR + 0x50)
#define FLASH_G4_SEC1R (FLASH_G4_REGS_ADDR + 0x70)
#define FLASH_G4_SEC2R (FLASH_G4_REGS_ADDR + 0x74)

// == STM32H7 ==
// H7 Flash registers
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

#define FLASH_H7_OPTCR_OPTLOCK 0
#define FLASH_H7_OPTCR_OPTSTART 1
#define FLASH_H7_OPTCR_MER 4

#define FLASH_H7_OPTSR_OPT_BUSY 0
#define FLASH_H7_OPTSR_OPTCHANGEERR 30

#define FLASH_H7_OPTCCR_CLR_OPTCHANGEERR 30

// H7 Flash control register
#define FLASH_H7_CR_LOCK 0
#define FLASH_H7_CR_PG 1
#define FLASH_H7_CR_SER 2
#define FLASH_H7_CR_BER 3
#define FLASH_H7_CR_PSIZE 4
#define FLASH_H7_CR_START(chipid) (chipid == STM32_CHIPID_H7Ax ? 5 : 7)
#define FLASH_H7_CR_SNB 8
#define FLASH_H7_CR_SNB_MASK 0x700

// H7 Flash status register
#define FLASH_H7_SR_QW 2
#define FLASH_H7_SR_WRPERR 17
#define FLASH_H7_SR_PGSERR 18
#define FLASH_H7_SR_STRBERR 19
#define FLASH_H7_SR_ERROR_MASK                                                 \
  ((1 << FLASH_H7_SR_PGSERR) | (1 << FLASH_H7_SR_STRBERR) |                    \
   (1 << FLASH_H7_SR_WRPERR))

// == STM32L0/L1/L4/L5 ==
// Lx Flash registers
#define FLASH_Lx_REGS_ADDR ((uint32_t)0x40023c00)
#define FLASH_Lx_ACR (FLASH_Lx_REGS_ADDR + 0x00)
#define FLASH_Lx_PECR (FLASH_Lx_REGS_ADDR + 0x04)
#define FLASH_Lx_PDKEYR (FLASH_Lx_REGS_ADDR + 0x08)
#define FLASH_Lx_PEKEYR (FLASH_Lx_REGS_ADDR + 0x0c)
#define FLASH_Lx_PRGKEYR (FLASH_Lx_REGS_ADDR + 0x10)
#define FLASH_Lx_OPTKEYR (FLASH_Lx_REGS_ADDR + 0x14)
#define FLASH_Lx_SR (FLASH_Lx_REGS_ADDR + 0x18)
#define FLASH_Lx_OBR (FLASH_Lx_REGS_ADDR + 0x1c)
#define FLASH_Lx_WRPR (FLASH_Lx_REGS_ADDR + 0x20)

// == STM32L0 ==
// L0 Flash registers
#define FLASH_L0_PEKEY1 0x89abcdef
#define FLASH_L0_PEKEY2 0x02030405

#define FLASH_L0_PRGKEY1 0x8c9daebf
#define FLASH_L0_PRGKEY2 0x13141516

#define FLASH_L0_OPTKEY1 0xFBEAD9C8
#define FLASH_L0_OPTKEY2 0x24252627

#define FLASH_L0_REGS_ADDR ((uint32_t)0x40022000)

#define FLASH_L0_PELOCK (0)
#define FLASH_L0_OPTLOCK (2)
#define FLASH_L0_OBL_LAUNCH (18)

// L0 Flash status register
#define FLASH_L0_SR_ERROR_MASK 0x00013f00
#define FLASH_L0_SR_WRPERR 8
#define FLASH_L0_SR_PGAERR 9
#define FLASH_L0_SR_NOTZEROERR 16

// == STM32L1 ==
// L1 Flash registers
#define FLASH_L1_FPRG 10
#define FLASH_L1_PROG 3

// L1 Flash status register
#define FLASH_L1_SR_ERROR_MASK 0x00003f00
#define FLASH_L1_SR_WRPERR 8
#define FLASH_L1_SR_PGAERR 9

// == STM32L4 ==
// L4 Flash registers
// L4 register base is at FLASH_REGS_ADDR (0x40022000)
#define FLASH_L4_KEYR (FLASH_REGS_ADDR + 0x08)
#define FLASH_L4_OPTKEYR (FLASH_REGS_ADDR + 0x0c)
#define FLASH_L4_SR (FLASH_REGS_ADDR + 0x10)
#define FLASH_L4_CR (FLASH_REGS_ADDR + 0x14)
#define FLASH_L4_OPTR (FLASH_REGS_ADDR + 0x20)

// L4 Flash status register
#define FLASH_L4_SR_ERROR_MASK 0x3f8 /* SR [9:3] */
#define FLASH_L4_SR_PROGERR 3
#define FLASH_L4_SR_WRPERR 4
#define FLASH_L4_SR_PGAERR 5
#define FLASH_L4_SR_BSY 16

// L4 Flash control register
#define FLASH_L4_CR_LOCK 31       /* Lock control register */
#define FLASH_L4_CR_OPTLOCK 30    /* Lock option bytes */
#define FLASH_L4_CR_PG 0          /* Program */
#define FLASH_L4_CR_PER 1         /* Page erase */
#define FLASH_L4_CR_MER1 2        /* Bank 1 erase */
#define FLASH_L4_CR_MER2 15       /* Bank 2 erase */
#define FLASH_L4_CR_STRT 16       /* Start command */
#define FLASH_L4_CR_OPTSTRT 17    /* Start writing option bytes */
#define FLASH_L4_CR_BKER 11       /* Bank select for page erase */
#define FLASH_L4_CR_PNB 3         /* Page number (8 bits) */
#define FLASH_L4_CR_OBL_LAUNCH 27 /* Option bytes reload */
// Bits requesting flash operations (useful when we want to clear them)
#define FLASH_L4_CR_OPBITS                                                \
  (uint32_t)((1lu << FLASH_L4_CR_PG) | (1lu << FLASH_L4_CR_PER) |    \
             (1lu << FLASH_L4_CR_MER1) | (1lu << FLASH_L4_CR_MER1))
// Page is fully specified by BKER and PNB
#define FLASH_L4_CR_PAGEMASK (uint32_t)(0x1fflu << FLASH_L4_CR_PNB)

#define FLASH_L4_OPTR_DUALBANK 21

// == STM32L5 == (RM0438, p.241)
// L5 Flash registers
#define FLASH_L5_REGS_ADDR ((uint32_t)0x40022000)
#define FLASH_L5_ACR (FLASH_L5_REGS_ADDR + 0x00)
#define FLASH_L5_NSKEYR (FLASH_L5_REGS_ADDR + 0x08)
#define FLASH_L5_OPTKEYR (FLASH_L5_REGS_ADDR + 0x10)
#define FLASH_L5_NSSR (FLASH_L5_REGS_ADDR + 0x20)
#define FLASH_L5_NSCR (FLASH_L5_REGS_ADDR + 0x28)
#define FLASH_L5_ECCR (FLASH_L5_REGS_ADDR + 0x30)
#define FLASH_L5_OPTR (FLASH_L5_REGS_ADDR + 0x40)

// FLASH_NSCR control registers (RM0438, p. 242)
#define FLASH_L5_NSCR_NSPG 0        /* Program */
#define FLASH_L5_NSCR_NSPER 1       /* Page erase */
#define FLASH_L5_NSCR_NSMER1 2      /* Bank 1 erase */
#define FLASH_L5_NSCR_NSPNB 3       /* Page number (7 bits) */
#define FLASH_L5_NSCR_NSBKER 11     /* Bank select for page erase */
#define FLASH_L5_NSCR_NSMER2 15     /* Bank 2 erase */
#define FLASH_L5_NSCR_NSSTRT 16     /* Start command */
#define FLASH_L5_NSCR_NSOPTSTRT 17  /* Start writing option bytes */
#define FLASH_L5_NSCR_NSEOPIE 24
#define FLASH_L5_NSCR_NSERRIE 25
#define FLASH_L5_NSCR_OBL_LAUNCH 27 /* Option bytes reload */
#define FLASH_L5_NSCR_OPTLOCK 30    /* Lock option bytes */
#define FLASH_L5_NSCR_NSLOCK 31     /* Lock control register */

// FLASH_NSSR status register (RM0438, p. 241)
#define FLASH_L5_NSSR_NSEOP 0       /* End of Operation */
#define FLASH_L5_NSSR_NSOPERR 1
#define FLASH_L5_NSSR_NSPROGERR 3
#define FLASH_L5_NSSR_NSWRPERR 4
#define FLASH_L5_NSSR_NSPGAERR 5
#define FLASH_L5_NSSR_NSSIZERR 6
#define FLASH_L5_NSSR_NSPGSERR 7
#define FLASH_L5_NSSR_OPTWERR 12
#define FLASH_L5_NSSR_BSY 16        /* Busy */
#define FLASH_L5_NSSR_ERROR_MASK (0x20fa)

// == STM32WB == (RM0434)
// WB Flash registers
#define FLASH_WB_REGS_ADDR ((uint32_t)0x58004000)
#define FLASH_WB_ACR (FLASH_WB_REGS_ADDR + 0x00)
#define FLASH_WB_KEYR (FLASH_WB_REGS_ADDR + 0x08)
#define FLASH_WB_OPT_KEYR (FLASH_WB_REGS_ADDR + 0x0c)
#define FLASH_WB_SR (FLASH_WB_REGS_ADDR + 0x10)
#define FLASH_WB_CR (FLASH_WB_REGS_ADDR + 0x14)
#define FLASH_WB_ECCR (FLASH_WB_REGS_ADDR + 0x18)
#define FLASH_WB_OPTR (FLASH_WB_REGS_ADDR + 0x20)
#define FLASH_WB_PCROP1ASR (FLASH_WB_REGS_ADDR + 0x24)
#define FLASH_WB_PCROP1AER (FLASH_WB_REGS_ADDR + 0x28)
#define FLASH_WB_WRP1AR (FLASH_WB_REGS_ADDR + 0x2c)
#define FLASH_WB_WRP1BR (FLASH_WB_REGS_ADDR + 0x30)
#define FLASH_WB_PCROP1BSR (FLASH_WB_REGS_ADDR + 0x34)
#define FLASH_WB_PCROP1BER (FLASH_WB_REGS_ADDR + 0x38)
#define FLASH_WB_IPCCBR (FLASH_WB_REGS_ADDR + 0x3c)
#define FLASH_WB_C2ACR (FLASH_WB_REGS_ADDR + 0x5c)
#define FLASH_WB_C2SR (FLASH_WB_REGS_ADDR + 0x60)
#define FLASH_WB_C2CR (FLASH_WB_REGS_ADDR + 0x64)
#define FLASH_WB_SFR (FLASH_WB_REGS_ADDR + 0x80)
#define FLASH_WB_SRRVR (FLASH_WB_REGS_ADDR + 0x84)

// WB Flash control register
#define FLASH_WB_CR_STRT (16)       /* Start */
#define FLASH_WB_CR_OPTSTRT (17)    /* Start writing option bytes */
#define FLASH_WB_CR_OBL_LAUNCH (27) /* Forces the option byte loading */
#define FLASH_WB_CR_OPTLOCK (30)    /* Option Lock */
#define FLASH_WB_CR_LOCK (31)       /* Lock */

// WB Flash status register
#define FLASH_WB_SR_ERROR_MASK (0x3f8) /* SR [9:3] */
#define FLASH_WB_SR_PROGERR (3)        /* Programming alignment error */
#define FLASH_WB_SR_WRPERR (4)         /* Write protection error */
#define FLASH_WB_SR_PGAERR (5)         /* Programming error */
#define FLASH_WB_SR_BSY (16)           /* Busy */

#endif // STM32FLASH_H
