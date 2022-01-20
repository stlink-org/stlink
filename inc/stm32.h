/*
 * File: stm32.h
 *
 * STM32-specific defines & identification parametres
 */

#ifndef STM32_H
#define STM32_H

/* STM32 Cortex-M core ids (CPUTAPID) */
enum stm32_core_id {
    STM32_CORE_ID_M0_SWD        = 0x0bb11477,   // (RM0091 Section 32.5.3) F0 SW-DP
                                                // (RM0444 Section 40.5.3) G0 SW-DP
    STM32_CORE_ID_M0P_SWD       = 0x0bc11477,   // (RM0385 Section 27.5.3) L0 SW-DP
    STM32_CORE_ID_M3_r1p1_SWD   = 0x1ba01477,   // (RM0008 Section 31.8.3) F1 SW-DP
    STM32_CORE_ID_M3_r1p1_JTAG  = 0x3ba00477,   // (RM0008 Section 31.6.3) F1 JTAG
    STM32_CORE_ID_M3_r2p0_SWD   = 0x2ba01477,   // (RM0033 Section 32.8.3) F2 SW-DP
                                                // (RM0038 Section 30.8.3) L1 SW-DP
    STM32_CORE_ID_M3_r2p0_JTAG  = 0x0ba00477,   // (RM0033 Section 32.6.3) F2 JTAG
                                                // (RM0038 Section 30.6.2) L1 JTAG
    STM32_CORE_ID_M4_r0p1_SWD   = 0x1ba01477,   // (RM0316 Section 33.8.3) F3 SW-DP
                                                // (RM0351 Section 48.8.3) L4 SW-DP
                                                // (RM0432 Section 57.8.3) L4+ SW-DP
    STM32_CORE_ID_M4_r0p1_JTAG  = 0x4ba00477,   // (RM0316 Section 33.6.3) F3 JTAG
                                                // (RM0351 Section 48.6.3) L4 JTAG
                                                // (RM0432 Section 57.6.3) L4+ JTAG
    STM32_CORE_ID_M4F_r0p1_SWD  = 0x2ba01477,   // (RM0090 Section 38.8.3) F4 SW-DP
                                                // (RM0090 Section 47.8.3) G4 SW-DP
    STM32_CORE_ID_M4F_r0p1_JTAG = 0x4ba00477,   // (RM0090 Section 38.6.3) F4 JTAG
                                                // (RM0090 Section 47.6.3) G4 JTAG
    STM32_CORE_ID_M7F_SWD       = 0x5ba02477,   // (RM0385 Section 40.8.3) F7 SW-DP
    STM32_CORE_ID_M7F_JTAG      = 0x5ba00477,   // (RM0385 Section 40.6.3) F7 JTAG
    STM32_CORE_ID_M7F_H7_SWD    = 0x6ba02477,   // (RM0433 Section 60.4.1) H7 SW-DP
    STM32_CORE_ID_M7F_H7_JTAG   = 0x6ba00477,   // (RM0433 Section 60.4.1) H7 JTAG
    STM32_CORE_ID_M33_SWD       = 0x0be02477,   // (RM0438 Section 52.2.10) L5 SW-DP
                                                // (RM0456 Section 65.3.3) U5 SW-DP
    STM32_CORE_ID_M33_JTAGD     = 0x0be01477,   // (RM0438 Section 52.2.10) L5 JTAG-DP
                                                // (RM0456 Section 65.3.3) U5 JTAG-DP
    STM32_CORE_ID_M33_JTAG      = 0x0ba04477,   // (RM0438 Section 52.2.8) L5 JTAG
                                                // (RM0456 Section 56.3.1) U5 JTAG
};

/* STM32 flash types */
enum stm32_flash_type {
    STM32_FLASH_TYPE_UNKNOWN   =  0,
    STM32_FLASH_TYPE_F0_F1_F3  =  1,
    STM32_FLASH_TYPE_F1_XL     =  2,
    STM32_FLASH_TYPE_F2_F4     =  3,
    STM32_FLASH_TYPE_F7        =  4,
    STM32_FLASH_TYPE_G0        =  5,
    STM32_FLASH_TYPE_G4        =  6,
    STM32_FLASH_TYPE_H7        =  7,
    STM32_FLASH_TYPE_L0_L1     =  8,
    STM32_FLASH_TYPE_L4_L4P    =  9,
    STM32_FLASH_TYPE_L5_U5     = 10,
    STM32_FLASH_TYPE_WB_WL     = 11,
};

/* STM32 chip-ids */
// See DBGMCU_IDCODE register (0xE0042000) in appropriate programming manual
// stm32 chipids, only lower 12 bits...

enum stm32_chipids {
    STM32_CHIPID_UNKNOWN          = 0x000,

    STM32_CHIPID_F1_MD            = 0x410, /* medium density */
    STM32_CHIPID_F2               = 0x411,
    STM32_CHIPID_F1_LD            = 0x412, /* low density */
    STM32_CHIPID_F4               = 0x413,
    STM32_CHIPID_F1_HD            = 0x414, /* high density */
    STM32_CHIPID_L4               = 0x415,
    STM32_CHIPID_L1_MD            = 0x416, /* medium density */
    STM32_CHIPID_L0               = 0x417,
    STM32_CHIPID_F1_CONN          = 0x418, /* connectivity line */
    STM32_CHIPID_F4_HD            = 0x419, /* high density */
    STM32_CHIPID_F1_VL_MD_LD      = 0x420, /* value line medium & low density */
    STM32_CHIPID_F446             = 0x421,
    STM32_CHIPID_F3               = 0x422,
    STM32_CHIPID_F4_LP            = 0x423,
    STM32_CHIPID_L0_CAT2          = 0x425,
    STM32_CHIPID_L1_MD_PLUS       = 0x427, /* medium density plus */
    STM32_CHIPID_F1_VL_HD         = 0x428, /* value line high density */
    STM32_CHIPID_L1_CAT2          = 0x429,
    STM32_CHIPID_F1_XLD           = 0x430, /* extra low density plus */
    STM32_CHIPID_F411xx           = 0x431,
    STM32_CHIPID_F37x             = 0x432,
    STM32_CHIPID_F4_DE            = 0x433,
    STM32_CHIPID_F4_DSI           = 0x434,
    STM32_CHIPID_L43x_L44x        = 0x435,
    STM32_CHIPID_L1_MD_PLUS_HD    = 0x436, /* medium density plus & high density */
    STM32_CHIPID_L152_RE          = 0x437,
    STM32_CHIPID_F334             = 0x438,
    STM32_CHIPID_F3xx_SMALL       = 0x439,
    STM32_CHIPID_F0               = 0x440,
    STM32_CHIPID_F412             = 0x441,
    STM32_CHIPID_F09x             = 0x442,
    STM32_CHIPID_F0xx_SMALL       = 0x444,
    STM32_CHIPID_F04              = 0x445,
    STM32_CHIPID_F303_HD          = 0x446, /* high density */
    STM32_CHIPID_L0_CAT5          = 0x447,
    STM32_CHIPID_F0_CAN           = 0x448,
    STM32_CHIPID_F7               = 0x449, /* Nucleo F746ZG board */
    STM32_CHIPID_H74xxx           = 0x450, /* RM0433, p.3189 */
    STM32_CHIPID_F76xxx           = 0x451,
    STM32_CHIPID_F72xxx           = 0x452, /* Nucleo F722ZE board */
    STM32_CHIPID_G0_CAT4          = 0x456, /* G051/G061 */
    STM32_CHIPID_L011             = 0x457,
    STM32_CHIPID_F410             = 0x458,
    STM32_CHIPID_G0_CAT2          = 0x460, /* G070/G071/G081 */
    STM32_CHIPID_L496x_L4A6x      = 0x461,
    STM32_CHIPID_L45x_L46x        = 0x462,
    STM32_CHIPID_F413             = 0x463,
    STM32_CHIPID_L41x_L42x        = 0x464,
    STM32_CHIPID_G0_CAT1          = 0x466, /* G030/G031/G041 */
    STM32_CHIPID_G0_CAT3          = 0x467, /* G0B1/G0C1 */
    STM32_CHIPID_G4_CAT2          = 0x468, /* RM0440, section 46.6.1 "MCU device ID code" */
    STM32_CHIPID_G4_CAT3          = 0x469,
    STM32_CHIPID_L4Rx             = 0x470, /* RM0432, p.2247, found on the STM32L4R9I-DISCO board */
    STM32_CHIPID_L4PX             = 0x471, /* RM0432, p.2247 */
    STM32_CHIPID_G4_CAT4          = 0x479,
    STM32_CHIPID_H7Ax             = 0x480, /* RM0455, p.2863 */
    STM32_CHIPID_H72x             = 0x483, /* RM0468, p.3199 */
    STM32_CHIPID_WB55             = 0x495,
    STM32_CHIPID_WLE              = 0x497,
};

/* Constant STM32 option bytes base memory address */
#define STM32_F4_OPTION_BYTES_BASE ((uint32_t)0x40023c14)

#define STM32_H7_OPTION_BYTES_BASE ((uint32_t)0x5200201c)

#define STM32_L0_OPTION_BYTES_BASE ((uint32_t)0x1ff80000)
#define STM32_L1_OPTION_BYTES_BASE ((uint32_t)0x1ff80000)

#define STM32_F7_OPTION_BYTES_BASE ((uint32_t)0x1fff0000)

#define STM32_G0_OPTION_BYTES_BASE ((uint32_t)0x1fff7800)
#define STM32_L4_OPTION_BYTES_BASE ((uint32_t)0x1fff7800)

#define STM32_F2_OPTION_BYTES_BASE ((uint32_t)0x1fffc000)

#define STM32_F0_OPTION_BYTES_BASE ((uint32_t)0x1ffff800)
#define STM32_F1_OPTION_BYTES_BASE ((uint32_t)0x1ffff800)
#define STM32_F3_OPTION_BYTES_BASE ((uint32_t)0x1ffff800)
#define STM32_G4_OPTION_BYTES_BASE ((uint32_t)0x1ffff800)

/* ============ */
/* Old defines from common.c are below */
/* ============ */

/* Constant STM32 memory address */
#define STM32_SRAM_BASE            ((uint32_t)0x20000000)
#define STM32_FLASH_BASE           ((uint32_t)0x08000000)

#define STM32_F1_FLASH_BANK2_BASE  ((uint32_t)0x08080000)
#define STM32_H7_FLASH_BANK2_BASE  ((uint32_t)0x08100000)

/* stm32f FPEC flash controller interface, pm0063 manual */
// STM32F05x is identical, based on RM0091 (DM00031936, Doc ID 018940 Rev 2, August 2012)
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
#define FLASH_H7_CR_START(chipid) (chipid == STM32_CHIPID_H7Ax ? 5 : 7)
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

#endif // STM32_H
