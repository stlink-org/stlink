#define DEBUG_FLASH 0
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stlink.h>
#include <logging.h>
#include <md5.h>

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

/* stm32f FPEC flash controller interface, pm0063 manual */
// TODO - all of this needs to be abstracted out....
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

// STM32F10x_XL has two flash memory banks with separate registers to control the second bank.
#define FLASH_KEYR2 (FLASH_REGS_ADDR + 0x44)
#define FLASH_SR2 (FLASH_REGS_ADDR + 0x4c)
#define FLASH_CR2 (FLASH_REGS_ADDR + 0x50)
#define FLASH_AR2 (FLASH_REGS_ADDR + 0x54)
#define FLASH_BANK2_START_ADDR 0x08080000

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
#define FLASH_CR_STRT 6
#define FLASH_CR_LOCK 7
#define FLASH_CR_OPTWRE 9

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
#define STM32G4_FLASH_PDKEYR    (STM32G4_FLASH_REGS_ADDR + 0x04)
#define STM32G4_FLASH_PCROP1SR  (STM32G4_FLASH_REGS_ADDR + 0x24)
#define STM32G4_FLASH_PCROP1ER  (STM32G4_FLASH_REGS_ADDR + 0x28)
#define STM32G4_FLASH_WRP1AR    (STM32G4_FLASH_REGS_ADDR + 0x2C)
#define STM32G4_FLASH_WRP1BR    (STM32G4_FLASH_REGS_ADDR + 0x30)
#define STM32G4_FLASH_PCROP2SR  (STM32G4_FLASH_REGS_ADDR + 0x44)
#define STM32G4_FLASH_PCROP2ER  (STM32G4_FLASH_REGS_ADDR + 0x48)
#define STM32G4_FLASH_WRP2AR    (STM32G4_FLASH_REGS_ADDR + 0x4C)
#define STM32G4_FLASH_WRP2BR    (STM32G4_FLASH_REGS_ADDR + 0x50)
#define STM32G4_FLASH_SEC1R     (STM32G4_FLASH_REGS_ADDR + 0x70)
#define STM32G4_FLASH_SEC2R     (STM32G4_FLASH_REGS_ADDR + 0x74)

// G0/G4 FLASH control register
#define STM32Gx_FLASH_CR_PG          (0)      /* Program */
#define STM32Gx_FLASH_CR_PER         (1)      /* Page erase */
#define STM32Gx_FLASH_CR_MER1        (2)      /* Mass erase */
#define STM32Gx_FLASH_CR_PNB         (3)      /* Page number */
#define STM32G0_FLASH_CR_PNG_LEN     (5)      /* STM32G0: 5 page number bits */
#define STM32G4_FLASH_CR_PNG_LEN     (7)      /* STM32G4: 7 page number bits */
#define STM32Gx_FLASH_CR_MER2       (15)      /* Mass erase (2nd bank)*/
#define STM32Gx_FLASH_CR_STRT       (16)      /* Start */
#define STM32Gx_FLASH_CR_OPTSTRT    (17)      /* Start of modification of option bytes */
#define STM32Gx_FLASH_CR_FSTPG      (18)      /* Fast programming */
#define STM32Gx_FLASH_CR_EOPIE      (24)      /* End of operation interrupt enable */
#define STM32Gx_FLASH_CR_ERRIE      (25)      /* Error interrupt enable */
#define STM32Gx_FLASH_CR_OBL_LAUNCH (27)      /* Forces the option byte loading */
#define STM32Gx_FLASH_CR_OPTLOCK    (30)      /* Options Lock */
#define STM32Gx_FLASH_CR_LOCK       (31)      /* FLASH_CR Lock */

// G0/G4 FLASH status register
#define STM32Gx_FLASH_SR_ERROR_MASK (0x3fa)
#define STM32Gx_FLASH_SR_BSY        (16)      /* FLASH_SR Busy */
#define STM32Gx_FLASH_SR_EOP        (0)       /* FLASH_EOP End of Operation */

// G4 FLASH option register
#define STM32G4_FLASH_OPTR_DBANK    (22)      /* FLASH_OPTR Dual Bank Mode */

// WB (RM0434)
#define STM32WB_FLASH_REGS_ADDR ((uint32_t)0x58004000)
#define STM32WB_FLASH_ACR       (STM32WB_FLASH_REGS_ADDR + 0x00)
#define STM32WB_FLASH_KEYR      (STM32WB_FLASH_REGS_ADDR + 0x08)
#define STM32WB_FLASH_OPT_KEYR  (STM32WB_FLASH_REGS_ADDR + 0x0C)
#define STM32WB_FLASH_SR        (STM32WB_FLASH_REGS_ADDR + 0x10)
#define STM32WB_FLASH_CR        (STM32WB_FLASH_REGS_ADDR + 0x14)
#define STM32WB_FLASH_ECCR      (STM32WB_FLASH_REGS_ADDR + 0x18)
#define STM32WB_FLASH_OPTR      (STM32WB_FLASH_REGS_ADDR + 0x20)
#define STM32WB_FLASH_PCROP1ASR (STM32WB_FLASH_REGS_ADDR + 0x24)
#define STM32WB_FLASH_PCROP1AER (STM32WB_FLASH_REGS_ADDR + 0x28)
#define STM32WB_FLASH_WRP1AR    (STM32WB_FLASH_REGS_ADDR + 0x2C)
#define STM32WB_FLASH_WRP1BR    (STM32WB_FLASH_REGS_ADDR + 0x30)
#define STM32WB_FLASH_PCROP1BSR (STM32WB_FLASH_REGS_ADDR + 0x34)
#define STM32WB_FLASH_PCROP1BER (STM32WB_FLASH_REGS_ADDR + 0x38)
#define STM32WB_FLASH_IPCCBR    (STM32WB_FLASH_REGS_ADDR + 0x3C)
#define STM32WB_FLASH_C2ACR     (STM32WB_FLASH_REGS_ADDR + 0x5C)
#define STM32WB_FLASH_C2SR      (STM32WB_FLASH_REGS_ADDR + 0x60)
#define STM32WB_FLASH_C2CR      (STM32WB_FLASH_REGS_ADDR + 0x64)
#define STM32WB_FLASH_SFR       (STM32WB_FLASH_REGS_ADDR + 0x80)
#define STM32WB_FLASH_SRRVR     (STM32WB_FLASH_REGS_ADDR + 0x84)

// WB Flash control register.
#define STM32WB_FLASH_CR_STRT       (16) /* FLASH_CR Start */
#define STM32WB_FLASH_CR_OPTLOCK    (30) /* FLASH_CR Option Lock */
#define STM32WB_FLASH_CR_LOCK       (31) /* FLASH_CR Lock */
// WB Flash status register.
#define STM32WB_FLASH_SR_BSY        (16) /* FLASH_SR Busy */

// 32L4 register base is at FLASH_REGS_ADDR (0x40022000)
#define STM32L4_FLASH_KEYR      (FLASH_REGS_ADDR + 0x08)
#define STM32L4_FLASH_OPTKEYR   (FLASH_REGS_ADDR + 0x0C)
#define STM32L4_FLASH_SR        (FLASH_REGS_ADDR + 0x10)
#define STM32L4_FLASH_CR        (FLASH_REGS_ADDR + 0x14)
#define STM32L4_FLASH_OPTR      (FLASH_REGS_ADDR + 0x20)

#define STM32L4_FLASH_SR_BSY        16
#define STM32L4_FLASH_SR_ERRMASK 0x3f8 /* SR [9:3] */

#define STM32L4_FLASH_CR_LOCK       31 /* Lock control register */
#define STM32L4_FLASH_CR_OPTLOCK    30 /* Lock option bytes */
#define STM32L4_FLASH_CR_PG          0 /* Program */
#define STM32L4_FLASH_CR_PER         1 /* Page erase */
#define STM32L4_FLASH_CR_MER1        2 /* Bank 1 erase */
#define STM32L4_FLASH_CR_MER2       15 /* Bank 2 erase */
#define STM32L4_FLASH_CR_STRT       16 /* Start command */
#define STM32L4_FLASH_CR_OPTSTRT    17 /* Start writing option bytes */
#define STM32L4_FLASH_CR_BKER       11 /* Bank select for page erase */
#define STM32L4_FLASH_CR_PNB         3 /* Page number (8 bits) */
#define STM32L4_FLASH_CR_OBL_LAUNCH 27 /* Option bytes reload */
// Bits requesting flash operations (useful when we want to clear them)
#define STM32L4_FLASH_CR_OPBITS \
    (uint32_t)((1lu << STM32L4_FLASH_CR_PG) | \
               (1lu << STM32L4_FLASH_CR_PER) | \
               (1lu << STM32L4_FLASH_CR_MER1) | \
               (1lu << STM32L4_FLASH_CR_MER1))
// Page is fully specified by BKER and PNB
#define STM32L4_FLASH_CR_PAGEMASK (uint32_t)(0x1fflu << STM32L4_FLASH_CR_PNB)

#define STM32L4_FLASH_OPTR_DUALBANK     21

// STM32L0x flash register base and offsets RM0090 - DM00031020.pdf
#define STM32L0_FLASH_REGS_ADDR ((uint32_t)0x40022000)
#define STM32L1_FLASH_REGS_ADDR ((uint32_t)0x40023c00)

#define STM32L0_FLASH_PELOCK (0)
#define STM32L0_FLASH_OPTLOCK (2)
#define STM32L0_FLASH_OBL_LAUNCH (18)

#define STM32L0_FLASH_SR_ERROR_MASK 0x00003F00

#define FLASH_ACR_OFF     ((uint32_t) 0x00)
#define FLASH_PECR_OFF    ((uint32_t) 0x04)
#define FLASH_PDKEYR_OFF  ((uint32_t) 0x08)
#define FLASH_PEKEYR_OFF  ((uint32_t) 0x0c)
#define FLASH_PRGKEYR_OFF ((uint32_t) 0x10)
#define FLASH_OPTKEYR_OFF ((uint32_t) 0x14)
#define FLASH_SR_OFF      ((uint32_t) 0x18)
#define FLASH_OBR_OFF     ((uint32_t) 0x1c)
#define FLASH_WRPR_OFF    ((uint32_t) 0x20)

//STM32F7
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
#define FLASH_F7_SR_OP_ERR 1 /* Operation error */
#define FLASH_F7_SR_EOP 0 /* End of operation */
#define FLASH_F7_OPTCR1_BOOT_ADD0 0
#define FLASH_F7_OPTCR1_BOOT_ADD1 16

#define FLASH_F7_SR_ERROR_MASK ((1 << FLASH_F7_SR_ERS_ERR) | (1 << FLASH_F7_SR_PGP_ERR) | (1 << FLASH_F7_SR_PGA_ERR) | (1 << FLASH_F7_SR_WRP_ERR) | (1 << FLASH_F7_SR_OP_ERR))

//STM32F4
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

#define L1_WRITE_BLOCK_SIZE 0x80
#define L0_WRITE_BLOCK_SIZE 0x40

void write_uint32(unsigned char* buf, uint32_t ui) {
    if (!is_bigendian()) { // le -> le (don't swap)
        buf[0] = ((unsigned char*)&ui)[0];
        buf[1] = ((unsigned char*)&ui)[1];
        buf[2] = ((unsigned char*)&ui)[2];
        buf[3] = ((unsigned char*)&ui)[3];
    } else {
        buf[0] = ((unsigned char*)&ui)[3];
        buf[1] = ((unsigned char*)&ui)[2];
        buf[2] = ((unsigned char*)&ui)[1];
        buf[3] = ((unsigned char*)&ui)[0];
    }
}

void write_uint16(unsigned char* buf, uint16_t ui) {
    if (!is_bigendian()) { // le -> le (don't swap)
        buf[0] = ((unsigned char*)&ui)[0];
        buf[1] = ((unsigned char*)&ui)[1];
    } else {
        buf[0] = ((unsigned char*)&ui)[1];
        buf[1] = ((unsigned char*)&ui)[0];
    }
}

uint32_t read_uint32(const unsigned char *c, const int pt) {
    uint32_t ui;
    char *p = (char *)&ui;

    if (!is_bigendian()) { // le -> le (don't swap)
        p[0] = c[pt + 0];
        p[1] = c[pt + 1];
        p[2] = c[pt + 2];
        p[3] = c[pt + 3];
    } else {
        p[0] = c[pt + 3];
        p[1] = c[pt + 2];
        p[2] = c[pt + 1];
        p[3] = c[pt + 0];
    }

    return(ui);
}

static uint32_t get_stm32l0_flash_base(stlink_t *sl)
{
    switch (sl->chip_id) {
    case STLINK_CHIPID_STM32_L1_CAT2:
    case STLINK_CHIPID_STM32_L1_MEDIUM:
    case STLINK_CHIPID_STM32_L1_MEDIUM_PLUS:
    case STLINK_CHIPID_STM32_L1_HIGH:
        return(STM32L1_FLASH_REGS_ADDR);
    default:
        return(STM32L0_FLASH_REGS_ADDR);
    }
}

static uint32_t __attribute__((unused)) read_flash_rdp(stlink_t *sl) {
    uint32_t rdp;
    stlink_read_debug32(sl, FLASH_WRPR, &rdp);
    return(rdp & 0xff);
}

static inline uint32_t read_flash_cr(stlink_t *sl) {
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
    } else {
        reg = FLASH_CR;
    }

    stlink_read_debug32(sl, reg, &res);

#if DEBUG_FLASH
    fprintf(stdout, "CR:0x%x\n", res);
#endif
    return(res);
}

static inline uint32_t read_flash_cr2(stlink_t *sl) {
    uint32_t res;
    stlink_read_debug32(sl, FLASH_CR2, &res);
#if DEBUG_FLASH
    fprintf(stdout, "CR2:0x%x\n", res);
#endif
    return(res);
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
    } else {
        ELOG("unsupported flash method, abort\n");
        return(-1);
    }

    stlink_read_debug32(sl, cr_reg, &n);
    return(n & (1u << cr_lock_shift));
}

static void unlock_flash(stlink_t *sl) {
    uint32_t key_reg;
    uint32_t flash_key1 = FLASH_KEY1;
    uint32_t flash_key2 = FLASH_KEY2;
    /* The unlock sequence consists of 2 write cycles where 2 key values are written
     * to the FLASH_KEYR register.
     * An invalid sequence results in a definitive lock of the FPEC block until next reset.
     */

    if ((sl->flash_type == STLINK_FLASH_TYPE_F0) ||
        (sl->flash_type == STLINK_FLASH_TYPE_F1_XL)) {
        key_reg = FLASH_KEYR;
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
    } else {
        ELOG("unsupported flash method, abort\n");
        return;
    }

    stlink_write_debug32(sl, key_reg, flash_key1);
    stlink_write_debug32(sl, key_reg, flash_key2);

    if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
        stlink_write_debug32(sl, FLASH_KEYR2, flash_key1);
        stlink_write_debug32(sl, FLASH_KEYR2, flash_key2);
    }
}

/* unlock flash if already locked */
static int unlock_flash_if(stlink_t *sl) {
    if (is_flash_locked(sl)) {
        unlock_flash(sl);

        if (is_flash_locked(sl)) {
            WLOG("Failed to unlock flash!\n");
            return(-1);
        }
    }

    DLOG("Successfully unlocked flash\n");
    return(0);
}

static void lock_flash(stlink_t *sl) {
    uint32_t cr_lock_shift, cr_reg, n;

    if (sl->flash_type == STLINK_FLASH_TYPE_F0 ||
        sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
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
    } else {
        ELOG("unsupported flash method, abort\n");
        return;
    }

    stlink_read_debug32(sl, cr_reg, &n);
    n |= (1u << cr_lock_shift);
    stlink_write_debug32(sl, cr_reg, n);

    if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
        n = read_flash_cr2(sl) | (1u << cr_lock_shift);
        stlink_write_debug32(sl, FLASH_CR2, n);
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
    default:
        ELOG("unsupported flash method, abort\n");
        return -1;
    }

    stlink_read_debug32(sl, optcr_reg, &n);

    if (active_bit_level == 0) {
        return(!(n & (1u << optlock_shift)));
    }

    return(n & (1u << optlock_shift));

}

static int lock_flash_option(stlink_t *sl) {
    uint32_t optlock_shift, optcr_reg, n;
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
    return(0);
}

static int unlock_flash_option(stlink_t *sl) {
    uint32_t optkey_reg;
    uint32_t optkey1 = FLASH_OPTKEY1;
    uint32_t optkey2 = FLASH_OPTKEY2;

    switch (sl->flash_type) {
    case STLINK_FLASH_TYPE_F0:
    case STLINK_FLASH_TYPE_F1_XL:
        optkey_reg = FLASH_OPTKEYR;
        optkey1 =  FLASH_F0_OPTKEY1;
        optkey2 =  FLASH_F0_OPTKEY2;
        break;
    case STLINK_FLASH_TYPE_F4:
        optkey_reg = FLASH_F4_OPT_KEYR;
        break;
    case STLINK_FLASH_TYPE_F7:
        optkey_reg = FLASH_F7_OPT_KEYR;
        break;
    case STLINK_FLASH_TYPE_L0:
        optkey_reg = get_stm32l0_flash_base(sl) + FLASH_OPTKEYR_OFF;
        optkey1 =  FLASH_L0_OPTKEY1;
        optkey2 =  FLASH_L0_OPTKEY2;
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
    default:
        ELOG("unsupported flash method, abort\n");
        return(-1);
    }

    stlink_write_debug32(sl, optkey_reg, optkey1);
    stlink_write_debug32(sl, optkey_reg, optkey2);
    return(0);
}

static int unlock_flash_option_if(stlink_t *sl) {
    if (is_flash_option_locked(sl)) {
        if (unlock_flash_option(sl)) {
            ELOG("Could not unlock flash option!\n");
            return(-1);
        }

        if (is_flash_option_locked(sl)) {
            ELOG("Failed to unlock flash option!\n");
            return(-1);
        }
    }

    DLOG("Successfully unlocked flash option\n");
    return(0);
}

static void set_flash_cr_pg(stlink_t *sl) {
    uint32_t cr_reg, x;

    x = read_flash_cr(sl);

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
    } else {
        cr_reg = FLASH_CR;
        x = (1 << FLASH_CR_PG);
    }

    stlink_write_debug32(sl, cr_reg, x);
}

static void clear_flash_cr_pg(stlink_t *sl) {
    uint32_t cr_reg, n;

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
    } else {
        cr_reg = FLASH_CR;
    }

    n = read_flash_cr(sl) & ~(1 << FLASH_CR_PG);
    stlink_write_debug32(sl, cr_reg, n);
}

static void set_flash_cr_per(stlink_t *sl) {
    uint32_t cr_reg, val;

    if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
        sl->flash_type == STLINK_FLASH_TYPE_G4) {
        cr_reg = STM32Gx_FLASH_CR;
    } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
        cr_reg = STM32WB_FLASH_CR;
    } else {
        cr_reg = FLASH_CR;
    }

    stlink_read_debug32(sl, cr_reg, &val);
    val |= (1 << FLASH_CR_PER);
    stlink_write_debug32(sl, cr_reg, val);
}

static void set_flash_cr2_per(stlink_t *sl) {
    const uint32_t n = (1 << FLASH_CR_PER);
    stlink_write_debug32(sl, FLASH_CR2, n);
}

static void clear_flash_cr_per(stlink_t *sl) {
    uint32_t cr_reg;

    if (sl->flash_type == STLINK_FLASH_TYPE_G0 ||
        sl->flash_type == STLINK_FLASH_TYPE_G4) {
        cr_reg = STM32Gx_FLASH_CR;
    } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
        cr_reg = STM32WB_FLASH_CR;
    } else {
        cr_reg = FLASH_CR;
    }

    const uint32_t n = read_flash_cr(sl) & ~(1 << FLASH_CR_PER);
    stlink_write_debug32(sl, cr_reg, n);
}

static void set_flash_cr_mer(stlink_t *sl, bool v) {
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
        cr_mer = (1 <<  STM32Gx_FLASH_CR_MER1);

        if (sl->has_dual_bank) {
            cr_mer |= (1 << STM32Gx_FLASH_CR_MER2);
        }

        cr_pg  = (1 << FLASH_CR_PG);
    } else if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
        cr_reg = STM32WB_FLASH_CR;
        cr_mer = (1 << FLASH_CR_MER);
        cr_pg  = (1 << FLASH_CR_PG);
    } else {
        cr_reg = FLASH_CR;
        cr_mer = (1 << FLASH_CR_MER);
        cr_pg  = (1 << FLASH_CR_PG);
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

static void set_flash_cr2_mer(stlink_t *sl, bool v) {
    const uint32_t cr_pg  = (1 << FLASH_CR_PER);
    const uint32_t cr_mer = (1 << FLASH_CR_MER);
    uint32_t val;

    stlink_read_debug32(sl, FLASH_CR2, &val);
    val &= ~cr_pg;

    if (v) {
        val |= cr_mer;
    } else {
        val &= ~cr_mer;
    }

    stlink_write_debug32(sl, FLASH_CR2, val);
}

static void __attribute__((unused)) clear_flash_cr_mer(stlink_t *sl) {
    uint32_t val, cr_reg, cr_mer;

    if (sl->flash_type == STLINK_FLASH_TYPE_F4) {
        cr_reg = FLASH_F4_CR;
        cr_mer = 1 << FLASH_CR_MER;
    } else if (sl->flash_type == STLINK_FLASH_TYPE_F7) {
        cr_reg = FLASH_F7_CR;
        cr_mer = 1 << FLASH_CR_MER;
    } else if (sl->flash_type == STLINK_FLASH_TYPE_L4) {
        cr_reg = STM32L4_FLASH_CR;
        cr_mer = (1 << STM32L4_FLASH_CR_MER1) | (1 << STM32L4_FLASH_CR_MER2);
    } else {
        cr_reg = FLASH_CR;
        cr_mer = (1 << FLASH_CR_MER);
    }

    stlink_read_debug32(sl, cr_reg, &val);
    val &= ~cr_mer;
    stlink_write_debug32(sl, cr_reg, val);
}

static void set_flash_cr_strt(stlink_t *sl) {
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
    } else {
        cr_reg = FLASH_CR;
        cr_strt = (1 << FLASH_CR_STRT);
    }

    stlink_read_debug32(sl, cr_reg, &val);
    val |= cr_strt;
    stlink_write_debug32(sl, cr_reg, val);
}

static void set_flash_cr2_strt(stlink_t *sl) {
    uint32_t val;

    stlink_read_debug32(sl, FLASH_CR2, &val);
    val |= 1 << FLASH_CR_STRT;
    stlink_write_debug32(sl, FLASH_CR2, val);
}

static inline uint32_t read_flash_sr(stlink_t *sl) {
    uint32_t res, sr_reg;

    if ((sl->flash_type == STLINK_FLASH_TYPE_F0) ||
        (sl->flash_type == STLINK_FLASH_TYPE_F1_XL)) {
        sr_reg = FLASH_SR;
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
    } else {
        ELOG("unsupported flash method, abort");
        return(-1);
    }

    stlink_read_debug32(sl, sr_reg, &res);
    return(res);
}

static inline uint32_t read_flash_sr2(stlink_t *sl) {
    uint32_t res;
    stlink_read_debug32(sl, FLASH_SR2, &res);
    return(res);
}

static inline unsigned int is_flash_busy(stlink_t *sl) {
    uint32_t sr_busy_shift;
    unsigned int res;

    if ((sl->flash_type == STLINK_FLASH_TYPE_F0) ||
        (sl->flash_type == STLINK_FLASH_TYPE_F0) ||
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
    } else {
        ELOG("unsupported flash method, abort");
        return(-1);
    }

    res = read_flash_sr(sl) & (1 << sr_busy_shift);

    if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
        res |= read_flash_sr2(sl) & (1 << sr_busy_shift);
    }

    return(res);
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

static int check_flash_error(stlink_t *sl)
{
    uint32_t res = 0;

    switch (sl->flash_type) {
    case STLINK_FLASH_TYPE_F0:
        res = read_flash_sr(sl) & FLASH_SR_ERROR_MASK;
        break;
    case STLINK_FLASH_TYPE_F7:
        res = read_flash_sr(sl) & FLASH_F7_SR_ERROR_MASK;
        break;
    case STLINK_FLASH_TYPE_G0:
    case STLINK_FLASH_TYPE_G4:
        res = read_flash_sr(sl) & STM32Gx_FLASH_SR_ERROR_MASK;
        break;
    case STLINK_FLASH_TYPE_L0:
        res = read_flash_sr(sl) & STM32L0_FLASH_SR_ERROR_MASK;
    default:
        break;
    }

    if (res) {
        ELOG("Flash programming error : %#010x\n", res);
        return(-1);
    }

    return(0);
}

static inline unsigned int is_flash_eop(stlink_t *sl) {
    return(read_flash_sr(sl) & (1 << FLASH_SR_EOP));
}

static void __attribute__((unused)) clear_flash_sr_eop(stlink_t *sl) {
    const uint32_t n = read_flash_sr(sl) & ~(1 << FLASH_SR_EOP);
    stlink_write_debug32(sl, FLASH_SR, n);
}

static void __attribute__((unused)) wait_flash_eop(stlink_t *sl) {
    // TODO: add some delays here
    while (is_flash_eop(sl) == 0)
        ;
}

static inline void write_flash_ar(stlink_t *sl, uint32_t n) {
    stlink_write_debug32(sl, FLASH_AR, n);
}

static inline void write_flash_ar2(stlink_t *sl, uint32_t n) {
    stlink_write_debug32(sl, FLASH_AR2, n);
}

static inline void write_flash_cr_psiz(stlink_t *sl, uint32_t n) {
    uint32_t x = read_flash_cr(sl);
    x &= ~(0x03 << 8);
    x |= (n << 8);
#if DEBUG_FLASH
    fprintf(stdout, "PSIZ:0x%x 0x%x\n", x, n);
#endif
    stlink_write_debug32(sl, FLASH_F4_CR, x);
}


static inline void write_flash_cr_snb(stlink_t *sl, uint32_t n) {
    uint32_t x = read_flash_cr(sl);
    x &= ~FLASH_F4_CR_SNB_MASK;
    x |= (n << FLASH_F4_CR_SNB);
    x |= (1 << FLASH_F4_CR_SER);
#if DEBUG_FLASH
    fprintf(stdout, "SNB:0x%x 0x%x\n", x, n);
#endif
    stlink_write_debug32(sl, FLASH_F4_CR, x);
}

static inline void write_flash_cr_bker_pnb(stlink_t *sl, uint32_t n) {
    stlink_write_debug32(sl, STM32L4_FLASH_SR, 0xFFFFFFFF & ~(1 << STM32L4_FLASH_SR_BSY));
    uint32_t x = read_flash_cr(sl);
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
    int ret;

    DLOG("*** stlink_exit_debug_mode ***\n");
    ret = stlink_write_debug32(sl, STLINK_REG_DHCSR, STLINK_REG_DHCSR_DBGKEY);

    if (ret == -1) {
        return(ret);
    }

    return(sl->backend->exit_debug_mode(sl));
}

int stlink_enter_swd_mode(stlink_t *sl) {
    DLOG("*** stlink_enter_swd_mode ***\n");
    return(sl->backend->enter_swd_mode(sl));
}

// Force the core into the debug mode -> halted state.
int stlink_force_debug(stlink_t *sl) {
    DLOG("*** stlink_force_debug_mode ***\n");
    return(sl->backend->force_debug(sl));
}

int stlink_exit_dfu_mode(stlink_t *sl) {
    DLOG("*** stlink_exit_dfu_mode ***\n");
    return(sl->backend->exit_dfu_mode(sl));
}

int stlink_core_id(stlink_t *sl) {
    int ret;

    DLOG("*** stlink_core_id ***\n");
    ret = sl->backend->core_id(sl);

    if (ret == -1) {
        ELOG("Failed to read core_id\n");
        return(ret);
    }

    if (sl->verbose > 2) {
        stlink_print_data(sl);
    }

    DLOG("core_id = 0x%08x\n", sl->core_id);
    return(ret);
}

// stlink_chip_id() is called by stlink_load_device_params()
// do not call this procedure directly.
int stlink_chip_id(stlink_t *sl, uint32_t *chip_id) {
    int ret;

    ret = stlink_read_debug32(sl, 0xE0042000, chip_id);

    if (ret == -1) {
        return(ret);
    }

    if (*chip_id == 0) {
        // Try Corex M0 DBGMCU_IDCODE register address
        ret = stlink_read_debug32(sl, 0x40015800, chip_id);
    }

    return(ret);
}

/**
 * Cortex m3 tech ref manual, CPUID register description
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
        return(-1);
    }

    cpuid->implementer_id = (raw >> 24) & 0x7f;
    cpuid->variant = (raw >> 20) & 0xf;
    cpuid->part = (raw >> 4) & 0xfff;
    cpuid->revision = raw & 0xf;
    return(0);
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
    uint32_t chip_id;
    uint32_t flash_size;

    stlink_chip_id(sl, &chip_id);
    sl->chip_id = chip_id & 0xfff;

    // Fix chip_id for F4 rev A errata , Read CPU ID, as CoreID is the same for F2/F4
    if (sl->chip_id == 0x411) {
        uint32_t cpuid;
        stlink_read_debug32(sl, 0xE000ED00, &cpuid);

        if ((cpuid  & 0xfff0) == 0xc240) {
            sl->chip_id = 0x413;
        }
    }

    params = stlink_chipid_get_params(sl->chip_id);

    if (params == NULL) {
        WLOG("unknown chip id! %#x\n", chip_id);
        return(-1);
    }

    if (params->flash_type == STLINK_FLASH_TYPE_UNKNOWN) {
        WLOG("Invalid flash type, please check device declaration\n");
        sl->flash_size = 0;
        return(0);
    }

    // These are fixed...
    sl->flash_base = STM32_FLASH_BASE;
    sl->sram_base = STM32_SRAM_BASE;
    stlink_read_debug32(sl, (params->flash_size_reg) & ~3, &flash_size);

    if (params->flash_size_reg & 2) {
        flash_size = flash_size >> 16;
    }

    flash_size = flash_size & 0xffff;

    if ((sl->chip_id == STLINK_CHIPID_STM32_L1_MEDIUM ||
         sl->chip_id == STLINK_CHIPID_STM32_F1_VL_MEDIUM_LOW ||
         sl->chip_id == STLINK_CHIPID_STM32_L1_MEDIUM_PLUS) && (flash_size == 0)) {
        sl->flash_size = 128 * 1024;
    } else if (sl->chip_id == STLINK_CHIPID_STM32_L1_CAT2) {
        sl->flash_size = (flash_size & 0xff) * 1024;
    } else if ((sl->chip_id & 0xFFF) == STLINK_CHIPID_STM32_L1_HIGH) {
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
    sl->has_dual_bank = params->has_dual_bank;
    sl->flash_pgsz = params->flash_pagesize;
    sl->sram_size = params->sram_size;
    sl->sys_base = params->bootrom_base;
    sl->sys_size = params->bootrom_size;
    sl->option_base = params->option_base;
    sl->option_size = params->option_size;

    // medium and low devices have the same chipid. ram size depends on flash size.
    // STM32F100xx datasheet Doc ID 16455 Table 2
    if (sl->chip_id == STLINK_CHIPID_STM32_F1_VL_MEDIUM_LOW && sl->flash_size < 64 * 1024) {
        sl->sram_size = 0x1000;
    }

    if (sl->chip_id == STLINK_CHIPID_STM32_G4_CAT3) {
        uint32_t flash_optr;
        stlink_read_debug32(sl, STM32Gx_FLASH_OPTR, &flash_optr);

        if (!(flash_optr & (1 << STM32G4_FLASH_OPTR_DBANK))) { sl->flash_pgsz <<= 1; }
    }

#if 0
    // Old code -- REW
    ILOG("Device connected is: %s, id %#x\n", params->description, chip_id);
    // TODO: make note of variable page size here.....
    ILOG("SRAM size: %#x bytes (%d KiB), Flash: %#x bytes (%d KiB) in pages of %u bytes\n",
         sl->sram_size, sl->sram_size / 1024, sl->flash_size, sl->flash_size / 1024,
         (unsigned int)sl->flash_pgsz);
#else
    ILOG("%s: %zd KiB SRAM, %zd KiB flash in at least %zd %s pages.\n",
         params->description, sl->sram_size / 1024, sl->flash_size / 1024,
         (sl->flash_pgsz < 1024) ? sl->flash_pgsz  :  sl->flash_pgsz / 1024,
         (sl->flash_pgsz < 1024) ?  "byte"         :  "KiB");
#endif
    return(0);
}

int stlink_reset(stlink_t *sl) {
    DLOG("*** stlink_reset ***\n");
    return(sl->backend->reset(sl));
}

int stlink_jtag_reset(stlink_t *sl, int value) {
    DLOG("*** stlink_jtag_reset ***\n");
    return(sl->backend->jtag_reset(sl, value));
}

int stlink_run(stlink_t *sl) {
    DLOG("*** stlink_run ***\n");
    return(sl->backend->run(sl));
}

int stlink_set_swdclk(stlink_t *sl, uint16_t divisor) {
    DLOG("*** set_swdclk ***\n");
    return(sl->backend->set_swdclk(sl, divisor));
}

int stlink_status(stlink_t *sl) {
    int ret;

    DLOG("*** stlink_status ***\n");
    ret = sl->backend->status(sl);
    stlink_core_stat(sl);
    return(ret);
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
            slv->jtag_api = slv->jtag_v > 11 ? STLINK_JTAG_API_V2 : STLINK_JTAG_API_V1;
        } else {
            slv->jtag_api = STLINK_JTAG_API_V2;

            // preferred API to get last R/W status from J15
            if (sl->version.jtag_v >= 15) {
                sl->version.flags |= STLINK_F_HAS_GETLASTRWSTATUS2;
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
    }

    return;
}

int stlink_version(stlink_t *sl) {
    DLOG("*** looking up stlink version\n");

    if (sl->backend->version(sl)) {
        return(-1);
    }

    _parse_version(sl, &sl->version);

    DLOG("st vid         = 0x%04x (expect 0x%04x)\n", sl->version.st_vid, STLINK_USB_VID_ST);
    DLOG("stlink pid     = 0x%04x\n", sl->version.stlink_pid);
    DLOG("stlink version = 0x%x\n", sl->version.stlink_v);
    DLOG("jtag version   = 0x%x\n", sl->version.jtag_v);
    DLOG("swim version   = 0x%x\n", sl->version.swim_v);

    if (sl->version.jtag_v == 0) {
        DLOG("    notice: the firmware doesn't support a jtag/swd interface\n");
    }

    if (sl->version.swim_v == 0) {
        DLOG("    notice: the firmware doesn't support a swim interface\n");
    }

    return(0);
}

int stlink_target_voltage(stlink_t *sl) {
    int voltage = -1;
    DLOG("*** reading target voltage\n");

    if (sl->backend->target_voltage != NULL) {
        voltage = sl->backend->target_voltage(sl);

        if (voltage != -1) {
            DLOG("target voltage = %ldmV\n", voltage);
        } else {
            DLOG("error reading target voltage\n");
        }
    } else {
        DLOG("reading voltage not supported by backend\n");
    }

    return(voltage);
}

int stlink_read_debug32(stlink_t *sl, uint32_t addr, uint32_t *data) {
    int ret;

    ret = sl->backend->read_debug32(sl, addr, data);
    if (!ret)
        DLOG("*** stlink_read_debug32 %#010x at %#010x\n", *data, addr);

    return(ret);
}

int stlink_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data) {
    DLOG("*** stlink_write_debug32 %#010x to %#010x\n", data, addr);
    return sl->backend->write_debug32(sl, addr, data);
}

int stlink_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    DLOG("*** stlink_write_mem32 %u bytes to %#x\n", len, addr);

    if (len % 4 != 0) {
        fprintf(stderr, "Error: Data length doesn't have a 32 bit alignment: +%d byte.\n",
                len % 4);
        abort();
    }

    return(sl->backend->write_mem32(sl, addr, len));
}

int stlink_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    DLOG("*** stlink_read_mem32 ***\n");

    if (len % 4 != 0) { // !!! never ever: fw gives just wrong values
        fprintf(stderr, "Error: Data length doesn't have a 32 bit alignment: +%d byte.\n",
                len % 4);
        abort();
    }

    return(sl->backend->read_mem32(sl, addr, len));
}

int stlink_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len) {
    DLOG("*** stlink_write_mem8 ***\n");

    if (len > 0x40) { // !!! never ever: Writing more then 0x40 bytes gives unexpected behaviour
        fprintf(stderr, "Error: Data length > 64: +%d byte.\n",
                len);
        abort();
    }

    return(sl->backend->write_mem8(sl, addr, len));
}

int stlink_read_all_regs(stlink_t *sl, struct stlink_reg *regp) {
    DLOG("*** stlink_read_all_regs ***\n");
    return(sl->backend->read_all_regs(sl, regp));
}

int stlink_read_all_unsupported_regs(stlink_t *sl, struct stlink_reg *regp) {
    DLOG("*** stlink_read_all_unsupported_regs ***\n");
    return(sl->backend->read_all_unsupported_regs(sl, regp));
}

int stlink_write_reg(stlink_t *sl, uint32_t reg, int idx) {
    DLOG("*** stlink_write_reg\n");
    return(sl->backend->write_reg(sl, reg, idx));
}

int stlink_read_reg(stlink_t *sl, int r_idx, struct stlink_reg *regp) {
    DLOG("*** stlink_read_reg\n");
    DLOG(" (%d) ***\n", r_idx);

    if (r_idx > 20 || r_idx < 0) {
        fprintf(stderr, "Error: register index must be in [0..20]\n");
        return(-1);
    }

    return(sl->backend->read_reg(sl, r_idx, regp));
}

int stlink_read_unsupported_reg(stlink_t *sl, int r_idx, struct stlink_reg *regp) {
    int r_convert;

    DLOG("*** stlink_read_unsupported_reg\n");
    DLOG(" (%d) ***\n", r_idx);

    /* Convert to values used by STLINK_REG_DCRSR */
    if (r_idx >= 0x1C && r_idx <= 0x1F) { // primask, basepri, faultmask, or control
        r_convert = 0x14;
    } else if (r_idx == 0x40) { // FPSCR
        r_convert = 0x21;
    } else if (r_idx >= 0x20 && r_idx < 0x40) {
        r_convert = 0x40 + (r_idx - 0x20);
    } else {
        fprintf(stderr, "Error: register address must be in [0x1C..0x40]\n");
        return(-1);
    }

    return(sl->backend->read_unsupported_reg(sl, r_convert, regp));
}

int stlink_write_unsupported_reg(stlink_t *sl, uint32_t val, int r_idx, struct stlink_reg *regp) {
    int r_convert;

    DLOG("*** stlink_write_unsupported_reg\n");
    DLOG(" (%d) ***\n", r_idx);

    /* Convert to values used by STLINK_REG_DCRSR */
    if (r_idx >= 0x1C && r_idx <= 0x1F) { /* primask, basepri, faultmask, or control */
        r_convert = r_idx;  // the backend function handles this
    } else if (r_idx == 0x40) { // FPSCR
        r_convert = 0x21;
    } else if (r_idx >= 0x20 && r_idx < 0x40) {
        r_convert = 0x40 + (r_idx - 0x20);
    } else {
        fprintf(stderr, "Error: register address must be in [0x1C..0x40]\n");
        return(-1);
    }

    return(sl->backend->write_unsupported_reg(sl, val, r_convert, regp));
}

bool stlink_is_core_halted(stlink_t *sl) {
    stlink_status(sl);
    return(sl->core_stat == TARGET_HALTED);
}

int stlink_step(stlink_t *sl) {
    DLOG("*** stlink_step ***\n");
    return(sl->backend->step(sl));
}

int stlink_current_mode(stlink_t *sl) {
    int mode = sl->backend->current_mode(sl);

    switch (mode) {
    case STLINK_DEV_DFU_MODE:
        DLOG("stlink current mode: dfu\n");
        return(mode);
    case STLINK_DEV_DEBUG_MODE:
        DLOG("stlink current mode: debug (jtag or swd)\n");
        return(mode);
    case STLINK_DEV_MASS_MODE:
        DLOG("stlink current mode: mass\n");
        return(mode);
    }

    DLOG("stlink mode: unknown!\n");
    return(STLINK_DEV_UNKNOWN_MODE);
}


// End of delegates....  Common code below here...

// Endianness
// http://www.ibm.com/developerworks/aix/library/au-endianc/index.html
// const int i = 1;
// #define is_bigendian() ( (*(char*)&i) == 0 )

unsigned int is_bigendian(void) {
    static volatile const unsigned int i = 1;
    return(*(volatile const char*)&i == 0);
}

uint16_t read_uint16(const unsigned char *c, const int pt) {
    uint32_t ui;
    char *p = (char *)&ui;

    if (!is_bigendian()) { // le -> le (don't swap)
        p[0] = c[pt + 0];
        p[1] = c[pt + 1];
    } else {
        p[0] = c[pt + 1];
        p[1] = c[pt + 0];
    }

    return(ui);
}

// same as above with entrypoint.

void stlink_run_at(stlink_t *sl, stm32_addr_t addr) {
    stlink_write_reg(sl, addr, 15); /* pc register */
    stlink_run(sl);

    while (stlink_is_core_halted(sl)) { usleep(3000000); }
}


// this function is called by stlink_status()
// do not call stlink_core_stat() directly, always use stlink_status()
void stlink_core_stat(stlink_t *sl) {
    switch (sl->core_stat ) {
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

void stlink_print_data(stlink_t * sl) {
    if (sl->q_len <= 0 || sl->verbose < UDEBUG) { return; }

    if (sl->verbose > 2) { DLOG("data_len = %d 0x%x\n", sl->q_len, sl->q_len); }

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
        //DLOG(" %02x", (unsigned int) sl->q_buf[i]);
        fprintf(stderr, " %02x", (unsigned int) sl->q_buf[i]);
    }
    //DLOG("\n\n");
    fprintf(stderr, "\n");
}

/* Memory mapped file */
typedef struct mapped_file {
    uint8_t* base;
    size_t len;
} mapped_file_t;

#define MAPPED_FILE_INITIALIZER { NULL, 0 }

static int map_file(mapped_file_t* mf, const char* path) {
    int error = -1;
    struct stat st;

    const int fd = open(path, O_RDONLY | O_BINARY);

    if (fd == -1) {
        fprintf(stderr, "open(%s) == -1\n", path);
        return(-1);
    }

    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "fstat(%s) == -1\n", path);
        goto on_error;
    }

    if (sizeof(st.st_size) != sizeof(size_t)) {
        // on 32 bit systems, check if there is an overflow
        if (st.st_size > (off_t)INT32_MAX) {
            fprintf(stderr, "mmap() size_t overflow for file %s\n", path);
            goto on_error;
        }
    }

    mf->base = (uint8_t*)mmap(NULL, (size_t)(st.st_size), PROT_READ, MAP_SHARED, fd, 0);

    if (mf->base == MAP_FAILED) {
        fprintf(stderr, "mmap() == MAP_FAILED for file %s\n", path);
        goto on_error;
    }

    mf->len = st.st_size;
    error = 0; // success

on_error:
    close(fd);
    return(error);
}

static void unmap_file(mapped_file_t * mf) {
    munmap((void*)mf->base, mf->len);
    mf->base = (unsigned char*)MAP_FAILED;
    mf->len = 0;
}

/* Limit the block size to compare to 0x1800 as anything larger will stall the STLINK2
 * Maybe STLINK V1 needs smaller value!
 */
static int check_file(stlink_t* sl, mapped_file_t* mf, stm32_addr_t addr) {
    size_t off;
    size_t n_cmp = sl->flash_pgsz;

    if ( n_cmp > 0x1800) { n_cmp = 0x1800; }

    for (off = 0; off < mf->len; off += n_cmp) {
        size_t aligned_size;

        size_t cmp_size = n_cmp; // adjust last page size

        if ((off + n_cmp) > mf->len) { cmp_size = mf->len - off; }

        aligned_size = cmp_size;

        if (aligned_size & (4 - 1)) { aligned_size = (cmp_size + 4) & ~(4 - 1); }

        stlink_read_mem32(sl, addr + (uint32_t)off, aligned_size);

        if (memcmp(sl->q_buf, mf->base + off, cmp_size)) { return(-1); }
    }

    return(0);
}

static void md5_calculate(mapped_file_t *mf) {
    // calculate md5 checksum of given binary file
    Md5Context md5Context;
    MD5_HASH md5Hash;
    Md5Initialise(&md5Context);
    Md5Update(&md5Context, mf->base, (uint32_t)mf->len);
    Md5Finalise(&md5Context, &md5Hash);
    printf("md5 checksum: ");

    for (int i = 0; i < (int)sizeof(md5Hash); i++) { printf("%x", md5Hash.bytes[i]); }

    printf(", ");
}

static void stlink_checksum(mapped_file_t *mp) {
    /* checksum that backward compatible with official ST tools */
    uint32_t sum = 0;
    uint8_t *mp_byte = (uint8_t *)mp->base;

    for (size_t i = 0; i < mp->len; ++i) { sum += mp_byte[i]; }

    printf("stlink checksum: 0x%08x\n", sum);
}

static void stlink_fwrite_finalize(stlink_t *sl, stm32_addr_t addr) {
    unsigned int val;
    // set stack
    stlink_read_debug32(sl, addr, &val);
    stlink_write_reg(sl, val, 13);
    // set PC to the reset routine
    stlink_read_debug32(sl, addr + 4, &val);
    stlink_write_reg(sl, val, 15);
    stlink_run(sl);
}

int stlink_mwrite_sram(stlink_t * sl, uint8_t* data, uint32_t length, stm32_addr_t addr) {
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

    if (len & 3) { len -= len & 3; }

    // do the copy by 1kB blocks
    for (off = 0; off < len; off += 1024) {
        size_t size = 1024;

        if ((off + size) > len) { size = len - off; }

        memcpy(sl->q_buf, data + off, size);

        if (size & 3) { size += 2; } // round size if needed

        stlink_write_mem32(sl, addr + (uint32_t)off, size);
    }

    if (length > len) {
        memcpy(sl->q_buf, data + len, length - len);
        stlink_write_mem8(sl, addr + (uint32_t)len, length - len);
    }


    error = 0; // success
    stlink_fwrite_finalize(sl, addr);

on_error:
    return(error);
}

int stlink_fwrite_sram(stlink_t * sl, const char* path, stm32_addr_t addr) {
    // write the file in sram at addr

    int error = -1;
    size_t off;
    size_t len;
    mapped_file_t mf = MAPPED_FILE_INITIALIZER;

    if (map_file(&mf, path) == -1) {
        fprintf(stderr, "map_file() == -1\n");
        return(-1);
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

    if (len & 3) { len -= len & 3; }

    // do the copy by 1kB blocks
    for (off = 0; off < len; off += 1024) {
        size_t size = 1024;

        if ((off + size) > len) { size = len - off; }

        memcpy(sl->q_buf, mf.base + off, size);

        if (size & 3) { size += 2; } // round size if needed

        stlink_write_mem32(sl, addr + (uint32_t)off, size);
    }

    if (mf.len > len) {
        memcpy(sl->q_buf, mf.base + len, mf.len - len);
        stlink_write_mem8(sl, addr + (uint32_t)len, mf.len - len);
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
    return(error);
}

typedef bool (*save_block_fn)(void* arg, uint8_t* block, ssize_t len);

static int stlink_read(
    stlink_t* sl, stm32_addr_t addr, size_t size, save_block_fn fn, void* fn_arg) {

    int error = -1;

    if (size < 1) { size = sl->flash_size; }

    if (size > sl->flash_size) {
        size = sl->flash_size;
    }

    size_t cmp_size = (sl->flash_pgsz > 0x1800) ? 0x1800 : sl->flash_pgsz;

    for (size_t off = 0; off < size; off += cmp_size) {
        size_t aligned_size;

        // adjust last page size
        if ((off + cmp_size) > size) { cmp_size = size - off; }

        aligned_size = cmp_size;

        if (aligned_size & (4 - 1)) { aligned_size = (cmp_size + 4) & ~(4 - 1); }

        stlink_read_mem32(sl, addr + (uint32_t)off, aligned_size);

        if (!fn(fn_arg, sl->q_buf, aligned_size)) { goto on_error; }
    }

    error = 0; // success

on_error:
    return(error);
}

struct stlink_fread_worker_arg {
    int fd;
};

static bool stlink_fread_worker(void* arg, uint8_t* block, ssize_t len) {
    struct stlink_fread_worker_arg* the_arg = (struct stlink_fread_worker_arg*)arg;

    if (write(the_arg->fd, block, len) != len) {
        fprintf(stderr, "write() != aligned_size\n");
        return(false);
    } else {
        return(true);
    }
}

struct stlink_fread_ihex_worker_arg {
    FILE* file;
    uint32_t addr;
    uint32_t lba;
    uint8_t buf[16];
    uint8_t buf_pos;
};

static bool stlink_fread_ihex_newsegment(struct stlink_fread_ihex_worker_arg* the_arg) {
    uint32_t addr = the_arg->addr;
    uint8_t sum = 2 + 4 + (uint8_t)((addr & 0xFF000000) >> 24) + (uint8_t)((addr & 0x00FF0000) >> 16);

    if (17 != fprintf(the_arg->file, ":02000004%04X%02X\r\n", (addr & 0xFFFF0000) >> 16,
            (uint8_t)(0x100 - sum))) {
        return(false);
    }

    the_arg->lba = (addr & 0xFFFF0000);
    return(true);
}

static bool stlink_fread_ihex_writeline(struct stlink_fread_ihex_worker_arg* the_arg) {
    uint8_t count = the_arg->buf_pos;

    if (count == 0) { return(true); }

    uint32_t addr = the_arg->addr;

    if (the_arg->lba != (addr & 0xFFFF0000)) { // segment changed
        if (!stlink_fread_ihex_newsegment(the_arg)) {
            return(false);
        }
    }

    uint8_t sum = count + (uint8_t)((addr & 0x0000FF00) >> 8) + (uint8_t)(addr & 0x000000FF);

    if (9 != fprintf(the_arg->file, ":%02X%04X00", count, (addr & 0x0000FFFF))) {
        return(false);
    }

    for (uint8_t i = 0; i < count; ++i) {
        uint8_t b = the_arg->buf[i];
        sum += b;

        if (2 != fprintf(the_arg->file, "%02X", b)) {
            return(false);
        }
    }

    if (4 != fprintf(the_arg->file, "%02X\r\n", (uint8_t)(0x100 - sum))) {
        return(false);
    }

    the_arg->addr += count;
    the_arg->buf_pos = 0;

    return(true);
}

static bool stlink_fread_ihex_init(
        struct stlink_fread_ihex_worker_arg* the_arg, int fd, stm32_addr_t addr) {
    the_arg->file    = fdopen(fd, "w");
    the_arg->addr    = addr;
    the_arg->lba     = 0;
    the_arg->buf_pos = 0;

    return (the_arg->file != NULL);
}

static bool stlink_fread_ihex_worker(void* arg, uint8_t* block, ssize_t len) {
    struct stlink_fread_ihex_worker_arg* the_arg = (struct stlink_fread_ihex_worker_arg*)arg;

    for (ssize_t i = 0; i < len; ++i) {
        if (the_arg->buf_pos == sizeof(the_arg->buf)) { // line is full
            if (!stlink_fread_ihex_writeline(the_arg)) { return(false); }
        }

        the_arg->buf[the_arg->buf_pos++] = block[i];
    }

    return(true);
}

static bool stlink_fread_ihex_finalize(struct stlink_fread_ihex_worker_arg* the_arg) {
    if (!stlink_fread_ihex_writeline(the_arg)) { return(false); }

    // FIXME: do we need the Start Linear Address?

    if (13 != fprintf(the_arg->file, ":00000001FF\r\n")) { // EoF
        return(false);
    }

    return (0 == fclose(the_arg->file));
}

int stlink_fread(stlink_t* sl, const char* path, bool is_ihex, stm32_addr_t addr, size_t size) {
    // read size bytes from addr to file
    ILOG("read from address %#010x size %d\n", addr, size);
    
    int error;
    int fd = open(path, O_RDWR | O_TRUNC | O_CREAT | O_BINARY, 00700);

    if (fd == -1) {
        fprintf(stderr, "open(%s) == -1\n", path);
        return(-1);
    }

    if (is_ihex) {
        struct stlink_fread_ihex_worker_arg arg;

        if (stlink_fread_ihex_init(&arg, fd, addr)) {
            error = stlink_read(sl, addr, size, &stlink_fread_ihex_worker, &arg);

            if (!stlink_fread_ihex_finalize(&arg)) { error = -1; }
        } else {
            error = -1;
        }
    } else {
        struct stlink_fread_worker_arg arg = { fd };
        error = stlink_read(sl, addr, size, &stlink_fread_worker, &arg);
    }

    close(fd);
    return(error);
}

int write_buffer_to_sram(stlink_t *sl, flash_loader_t* fl, const uint8_t* buf, size_t size) {
    // write the buffer right after the loader
    size_t chunk = size & ~0x3;
    size_t rem   = size & 0x3;

    if (chunk) {
        memcpy(sl->q_buf, buf, chunk);
        stlink_write_mem32(sl, fl->buf_addr, chunk);
    }

    if (rem) {
        memcpy(sl->q_buf, buf + chunk, rem);
        stlink_write_mem8(sl, (fl->buf_addr) + (uint32_t)chunk, rem);
    }

    return(0);
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
        return(offset + 1);
    } else if (flashaddr < 0xc000) {
        return(offset + 2);
    } else if (flashaddr < 0x10000) {
        return(offset + 3);
    } else if (flashaddr < 0x20000) {
        return(offset + 4);
    } else {
        return(offset + (flashaddr / 0x20000) + 4);
    }

}

uint32_t calculate_F7_sectornum(uint32_t flashaddr) {
    flashaddr &= ~STM32_FLASH_BASE; // Page now holding the actual flash address

    if (flashaddr < 0x20000) {
        return(flashaddr / 0x8000);
    } else if (flashaddr < 0x40000) {
        return(4);
    } else {
        return((flashaddr / 0x40000) + 4);
    }

}

// returns BKER:PNB for the given page address
uint32_t calculate_L4_page(stlink_t *sl, uint32_t flashaddr) {
    uint32_t bker = 0;
    uint32_t flashopt;
    stlink_read_debug32(sl, STM32L4_FLASH_OPTR, &flashopt);
    flashaddr -= STM32_FLASH_BASE;

    if (sl->chip_id == STLINK_CHIPID_STM32_L4 ||
        sl->chip_id == STLINK_CHIPID_STM32_L496X ||
        sl->chip_id == STLINK_CHIPID_STM32_L4RX) {
        // this chip use dual banked flash
        if (flashopt & (uint32_t)(1lu << STM32L4_FLASH_OPTR_DUALBANK)) {
            uint32_t banksize = (uint32_t)sl->flash_size / 2;

            if (flashaddr >= banksize) {
                flashaddr -= banksize;
                bker = 0x100;
            }
        }
    }

    // For 1MB chips without the dual-bank option set, the page address will overflow
    // into the BKER bit, which gives us the correct bank:page value.
    return(bker | flashaddr / (uint32_t)sl->flash_pgsz);
}

uint32_t stlink_calculate_pagesize(stlink_t *sl, uint32_t flashaddr) {
    if ((sl->chip_id == STLINK_CHIPID_STM32_F2) ||
        (sl->chip_id == STLINK_CHIPID_STM32_F4) ||
        (sl->chip_id == STLINK_CHIPID_STM32_F4_DE) ||
        (sl->chip_id == STLINK_CHIPID_STM32_F4_LP) ||
        (sl->chip_id == STLINK_CHIPID_STM32_F4_HD) ||
        (sl->chip_id == STLINK_CHIPID_STM32_F411RE) ||
        (sl->chip_id == STLINK_CHIPID_STM32_F446) ||
        (sl->chip_id == STLINK_CHIPID_STM32_F4_DSI) ||
        (sl->chip_id == STLINK_CHIPID_STM32_F72XXX) ||
        (sl->chip_id == STLINK_CHIPID_STM32_F412)) {
        uint32_t sector = calculate_F4_sectornum(flashaddr);

        if (sector >= 12) { sector -= 12; }

        if (sector < 4) {
            sl->flash_pgsz = 0x4000;
        } else if (sector < 5) {
            sl->flash_pgsz = 0x10000;
        } else {
            sl->flash_pgsz = 0x20000;
        }
    } else if (sl->chip_id == STLINK_CHIPID_STM32_F7 ||
               sl->chip_id == STLINK_CHIPID_STM32_F7XXXX) {
        uint32_t sector = calculate_F7_sectornum(flashaddr);

        if (sector < 4) {
            sl->flash_pgsz = 0x8000;
        } else if (sector < 5) {
            sl->flash_pgsz = 0x20000;
        } else {
            sl->flash_pgsz = 0x40000;
        }
    }

    return((uint32_t)sl->flash_pgsz);
}

/**
 * Erase a page of flash, assumes sl is fully populated with things like chip/core ids
 * @param sl stlink context
 * @param flashaddr an address in the flash page to erase
 * @return 0 on success -ve on failure
 */
int stlink_erase_flash_page(stlink_t *sl, stm32_addr_t flashaddr) {
    if (sl->flash_type == STLINK_FLASH_TYPE_F4 ||
        sl->flash_type == STLINK_FLASH_TYPE_F7 ||
        sl->flash_type == STLINK_FLASH_TYPE_L4) {
        // wait for ongoing op to finish
        wait_flash_busy(sl);

        // unlock if locked
        unlock_flash_if(sl);

        // select the page to erase
        if ((sl->chip_id == STLINK_CHIPID_STM32_L4) ||
            (sl->chip_id == STLINK_CHIPID_STM32_L43X) ||
            (sl->chip_id == STLINK_CHIPID_STM32_L46X) ||
            (sl->chip_id == STLINK_CHIPID_STM32_L496X) ||
            (sl->chip_id == STLINK_CHIPID_STM32_L4RX)) {
            // calculate the actual bank+page from the address
            uint32_t page = calculate_L4_page(sl, flashaddr);

            fprintf(stderr, "EraseFlash - Page:0x%x Size:0x%x ",
                    page, stlink_calculate_pagesize(sl, flashaddr));

            write_flash_cr_bker_pnb(sl, page);
        } else if (sl->chip_id == STLINK_CHIPID_STM32_F7 ||
                   sl->chip_id == STLINK_CHIPID_STM32_F7XXXX) {
            // calculate the actual page from the address
            uint32_t sector = calculate_F7_sectornum(flashaddr);

            fprintf(stderr, "EraseFlash - Sector:0x%x Size:0x%x ",
                    sector, stlink_calculate_pagesize(sl, flashaddr));
            write_flash_cr_snb(sl, sector);
        } else {
            // calculate the actual page from the address
            uint32_t sector = calculate_F4_sectornum(flashaddr);

            fprintf(stderr, "EraseFlash - Sector:0x%x Size:0x%x ",
                    sector, stlink_calculate_pagesize(sl, flashaddr));

            // the SNB values for flash sectors in the second bank do not directly
            // follow the values for the first bank on 2mb devices...
            if (sector >= 12) { sector += 4; }

            write_flash_cr_snb(sl, sector);
        }

        set_flash_cr_strt(sl); // start erase operation
        wait_flash_busy(sl);   // wait for completion
        lock_flash(sl);        // TODO: fails to program if this is in
#if DEBUG_FLASH
        fprintf(stdout, "Erase Final CR:0x%x\n", read_flash_cr(sl));
#endif
    } else if (sl->flash_type == STLINK_FLASH_TYPE_L0) {

        uint32_t val;
        uint32_t flash_regs_base;

        if (sl->chip_id == STLINK_CHIPID_STM32_L0 ||
            sl->chip_id == STLINK_CHIPID_STM32_L0_CAT5 ||
            sl->chip_id == STLINK_CHIPID_STM32_L0_CAT2 ||
            sl->chip_id == STLINK_CHIPID_STM32_L011) {
            flash_regs_base = STM32L0_FLASH_REGS_ADDR;
        } else {
            flash_regs_base = STM32L_FLASH_REGS_ADDR;
        }

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
                return(-1);
            }

            // unlock program memory
            stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, FLASH_L0_PRGKEY1);
            stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, FLASH_L0_PRGKEY2);

            // check pecr.prglock is cleared
            stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);

            if (val & (1 << 1)) {
                WLOG("pecr.prglock not clear (%#x)\n", val);
                return(-1);
            }
        }

        // set pecr.{erase,prog}
        val |= (1 << 9) | (1 << 3);
        stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
#if 0
        /* Wait for sr.busy to be cleared
         * MP: Test shows that busy bit is not set here. Perhaps, PM0062 is wrong
         * and we do not need to wait here for clearing the busy bit.
         */
        do {
            stlink_read_debug32(sl, STM32L_FLASH_SR, &val)
        } while ((val & (1 << 0)) != 0);
#endif

        // write 0 to the first word of the page to be erased
        stlink_write_debug32(sl, flashaddr, 0);

        /* MP: It is better to wait for clearing the busy bit after issuing page
         * erase command, even though PM0062 recommends to wait before it.
         * Test shows that a few iterations is performed in the following loop
         * before busy bit is cleared.
         */
        do
            stlink_read_debug32(sl, flash_regs_base + FLASH_SR_OFF, &val);
        while ((val & (1 << 0)) != 0);

        // reset lock bits
        stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
        val |= (1 << 0) | (1 << 1) | (1 << 2);
        stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
    } else if (sl->flash_type == STLINK_FLASH_TYPE_WB ||
               sl->flash_type == STLINK_FLASH_TYPE_G0 ||
               sl->flash_type == STLINK_FLASH_TYPE_G4) {
        uint32_t val;
        wait_flash_busy(sl);  // wait for any ongoing Flash operation to finish
        unlock_flash_if(sl);
        set_flash_cr_per(sl); // set the 'enable Flash erase' bit

        // set the page to erase
        if (sl->flash_type == STLINK_FLASH_TYPE_WB) {
            uint32_t flash_page = ((flashaddr - STM32_FLASH_BASE) / (uint32_t)(sl->flash_pgsz));
            stlink_read_debug32(sl, STM32WB_FLASH_CR, &val);

            // sec 3.10.5 - PNB[7:0] is offset by 3.
            val &= ~(0xFF << 3); // Clear previously set page number (if any)
            val |= ((flash_page & 0xFF) << 3);

            stlink_write_debug32(sl, STM32WB_FLASH_CR, val);
        } else if (sl->flash_type == STLINK_FLASH_TYPE_G0) {
            uint32_t flash_page = ((flashaddr - STM32_FLASH_BASE) / (uint32_t)(sl->flash_pgsz));
            stlink_read_debug32(sl, STM32Gx_FLASH_CR, &val);
            // sec 3.7.5 - PNB[5:0] is offset by 3. PER is 0x2.
            val &= ~(0x3F << 3);
            val |= ((flash_page & 0x3F) << 3) | (1 << FLASH_CR_PER);
            stlink_write_debug32(sl, STM32Gx_FLASH_CR, val);
        } else if (sl->flash_type == STLINK_FLASH_TYPE_G4) {
            uint32_t flash_page = ((flashaddr - STM32_FLASH_BASE) / (uint32_t)(sl->flash_pgsz));
            stlink_read_debug32(sl, STM32Gx_FLASH_CR, &val);
            // sec 3.7.5 - PNB[6:0] is offset by 3. PER is 0x2.
            val &= ~(0x7F << 3);
            val |= ((flash_page & 0x7F) << 3) | (1 << FLASH_CR_PER);
            stlink_write_debug32(sl, STM32Gx_FLASH_CR, val);
        }

        set_flash_cr_strt(sl);  // set the 'start operation' bit
        wait_flash_busy(sl);    // wait for the 'busy' bit to clear
        clear_flash_cr_per(sl); // clear the 'enable page erase' bit
        lock_flash(sl);
    } else if ((sl->flash_type == STLINK_FLASH_TYPE_F0) ||
               ((sl->flash_type == STLINK_FLASH_TYPE_F1_XL) && (flashaddr < FLASH_BANK2_START_ADDR))) {
        wait_flash_busy(sl);
        unlock_flash_if(sl);
        clear_flash_cr_pg(sl);         // clear the pg bit
        set_flash_cr_per(sl);          // set the page erase bit
        write_flash_ar(sl, flashaddr); // select the page to erase
        set_flash_cr_strt(sl);         // start erase operation, reset by hw with busy bit
        wait_flash_busy(sl);
        lock_flash(sl);
    } else if ((sl->flash_type == STLINK_FLASH_TYPE_F1_XL) && (flashaddr >= FLASH_BANK2_START_ADDR)) {
        wait_flash_busy(sl);
        unlock_flash_if(sl);
        set_flash_cr2_per(sl);          // set the page erase bit
        write_flash_ar2(sl, flashaddr); // select the page to erase
        set_flash_cr2_strt(sl);         // start erase operation, reset by hw with busy bit
        wait_flash_busy(sl);
        lock_flash(sl);
    } else {
        WLOG("unknown coreid %x, page erase failed\n", sl->core_id);
        return(-1);
    }

    // TODO: verify the erased page
    return(0);
}

int stlink_erase_flash_mass(stlink_t *sl) {
    // TODO: User MER bit to mass-erase G0, G4, WB series.
    if (sl->flash_type == STLINK_FLASH_TYPE_L0 ||
        sl->flash_type == STLINK_FLASH_TYPE_WB) {
        // erase each page
        int i = 0, num_pages = (int)(sl->flash_size / sl->flash_pgsz);

        for (i = 0; i < num_pages; i++) {
            // addr must be an addr inside the page
            stm32_addr_t addr = (stm32_addr_t)sl->flash_base + i * (stm32_addr_t)sl->flash_pgsz;

            if (stlink_erase_flash_page(sl, addr) == -1) {
                WLOG("Failed to erase_flash_page(%#zx) == -1\n", addr);
                return(-1);
            }

            fprintf(stdout, "-> Flash page at %5d/%5d erased\n", i, num_pages);
            fflush(stdout);
        }

        fprintf(stdout, "\n");
    } else {
        wait_flash_busy(sl);
        unlock_flash_if(sl);
        set_flash_cr_mer(sl, 1);      // set the mass erase bit
        set_flash_cr_strt(sl);        // start erase operation, reset by hw with busy bit

        if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
            set_flash_cr2_mer(sl, 1); // set the mass erase bit in bank 2
            set_flash_cr2_strt(sl);   // start erase operation in bank 2
        }

        wait_flash_busy_progress(sl);
        check_flash_error(sl);
        lock_flash(sl);

        set_flash_cr_mer(sl, 0);      // reset the mass erase bit

        if (sl->flash_type == STLINK_FLASH_TYPE_F1_XL) {
            set_flash_cr2_mer(sl, 0); // reset the mass erase bit in bank 2
        }

        // TODO: verify the erased memory
    }

    return(0);
}

int stlink_fcheck_flash(stlink_t *sl, const char* path, stm32_addr_t addr) {
    // check the contents of path are at addr

    int res;
    mapped_file_t mf = MAPPED_FILE_INITIALIZER;

    if (map_file(&mf, path) == -1) { return(-1); }

    res = check_file(sl, &mf, addr);
    unmap_file(&mf);
    return(res);
}

/**
 * Verify addr..addr+len is binary identical to base...base+len
 * @param sl stlink context
 * @param address stm device address
 * @param data host side buffer to check against
 * @param length how much
 * @return 0 for success, -ve for failure
 */
int stlink_verify_write_flash(stlink_t *sl, stm32_addr_t address, uint8_t *data, unsigned length) {
    size_t off;
    size_t cmp_size = (sl->flash_pgsz > 0x1800) ? 0x1800 : sl->flash_pgsz;
    ILOG("Starting verification of write complete\n");

    for (off = 0; off < length; off += cmp_size) {
        size_t aligned_size;

        // adjust last page size
        if ((off + cmp_size) > length) { cmp_size = length - off; }

        aligned_size = cmp_size;

        if (aligned_size & (4 - 1)) { aligned_size = (cmp_size + 4) & ~(4 - 1); }

        stlink_read_mem32(sl, address + (uint32_t)off, aligned_size);

        if (memcmp(sl->q_buf, data + off, cmp_size)) {
            ELOG("Verification of flash failed at offset: %u\n", (unsigned int)off);
            return(-1);
        }
    }

    ILOG("Flash written and verified! jolly good!\n");
    return(0);

}

int stm32l1_write_half_pages(
        stlink_t *sl, stm32_addr_t addr, uint8_t* base, uint32_t len, uint32_t pagesize) {
    unsigned int count;
    unsigned int num_half_pages = len / pagesize;
    uint32_t val;
    uint32_t flash_regs_base;
    flash_loader_t fl;

    if (sl->chip_id == STLINK_CHIPID_STM32_L0 ||
        sl->chip_id == STLINK_CHIPID_STM32_L0_CAT5 ||
        sl->chip_id == STLINK_CHIPID_STM32_L0_CAT2 ||
        sl->chip_id == STLINK_CHIPID_STM32_L011) {
        flash_regs_base = STM32L0_FLASH_REGS_ADDR;
    } else {
        flash_regs_base = STM32L_FLASH_REGS_ADDR;
    }

    ILOG("Starting Half page flash write for STM32L core id\n");

    /* Flash loader initialisation */
    if (stlink_flash_loader_init(sl, &fl) == -1) {
        WLOG("stlink_flash_loader_init() == -1\n");
        return(-1);
    }

    // unlock already done
    stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
    val |= (1 << FLASH_L1_FPRG);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);

    val |= (1 << FLASH_L1_PROG);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);

    do {
        stlink_read_debug32(sl, flash_regs_base + FLASH_SR_OFF, &val);
    } while ((val & (1 << 0)) != 0);

    for (count = 0; count  < num_half_pages; count++) {
        if (stlink_flash_loader_run(
                sl, &fl, addr + count * pagesize, base + count * pagesize, pagesize) == -1) {
            WLOG("l1_stlink_flash_loader_run(%#zx) failed! == -1\n", addr + count * pagesize);
            stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
            val &= ~((1 << FLASH_L1_FPRG) | (1 << FLASH_L1_PROG));
            stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
            return(-1);
        }

        // wait for sr.busy to be cleared
        if (sl->verbose >= 1) {
            // show progress; writing procedure is slow and previous errors are misleading
            fprintf(stdout, "\r%3u/%u halfpages written", count + 1, num_half_pages);
            fflush(stdout);
        }

        do {
            stlink_read_debug32(sl, flash_regs_base + FLASH_SR_OFF, &val);
        } while ((val & (1 << 0)) != 0);
    }

    stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
    val &= ~(1 << FLASH_L1_PROG);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
    stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
    val &= ~(1 << FLASH_L1_FPRG);
    stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
    return(0);
}

int stlink_write_flash(
        stlink_t *sl, stm32_addr_t addr, uint8_t* base, uint32_t len, uint8_t eraseonly) {
    size_t off;
    flash_loader_t fl;
    ILOG("Attempting to write %d (%#x) bytes to stm32 address: %u (%#x)\n", len, len, addr, addr);
    // check addr range is inside the flash
    stlink_calculate_pagesize(sl, addr);

    if (addr < sl->flash_base) {
        ELOG("addr too low %#x < %#x\n", addr, sl->flash_base);
        return(-1);
    } else if ((addr + len) < addr) {
        ELOG("addr overruns\n");
        return(-1);
    } else if ((addr + len) > (sl->flash_base + sl->flash_size)) {
        ELOG("addr too high\n");
        return(-1);
    } else if (addr & 1) {
        ELOG("unaligned addr 0x%x\n", addr);
        return(-1);
    } else if (len & 1) {
        WLOG("unaligned len 0x%x -- padding with zero\n", len);
        len += 1;
    } else if (addr & (sl->flash_pgsz - 1)) {
        ELOG("addr not a multiple of current pagesize (%zd bytes), not supported, "
             "check page start address and compare with flash module organisation "
             "in related ST reference manual of your device.\n", sl->flash_pgsz);
        return(-1);
    }

    // make sure we've loaded the context with the chip details
    stlink_core_id(sl);

    // Erase each page
    int page_count = 0;

    for (off = 0; off < len; off += stlink_calculate_pagesize(sl, addr + (uint32_t)off)) {
        // addr must be an addr inside the page
        if (stlink_erase_flash_page(sl, addr + (uint32_t)off) == -1) {
            ELOG("Failed to erase_flash_page(%#zx) == -1\n", addr + off);
            return(-1);
        }

        ILOG("Flash page at addr: 0x%08lx erased\n", (unsigned long)(addr + off));
        page_count++;
    }

    ILOG("Finished erasing %d pages of %d (%#x) bytes\n",
         page_count, sl->flash_pgsz, sl->flash_pgsz);

    if (eraseonly) { return(0); }

    if ((sl->flash_type == STLINK_FLASH_TYPE_F4) ||
        (sl->flash_type == STLINK_FLASH_TYPE_F7) ||
        (sl->flash_type == STLINK_FLASH_TYPE_L4)) {
        // TODO: check write operation

        ILOG("Starting Flash write for F2/F4/F7/L4\n");
        // Flash loader initialisation
        if (stlink_flash_loader_init(sl, &fl) == -1) {
            ELOG("stlink_flash_loader_init() == -1\n");
            return(-1);
        }

        unlock_flash_if(sl); // first unlock the cr

        // TODO: Check that Voltage range is 2.7 - 3.6 V
        if ((sl->chip_id != STLINK_CHIPID_STM32_L4) &&
            (sl->chip_id != STLINK_CHIPID_STM32_L43X) &&
            (sl->chip_id != STLINK_CHIPID_STM32_L46X) &&
            (sl->chip_id != STLINK_CHIPID_STM32_L496X) &&
            (sl->chip_id != STLINK_CHIPID_STM32_L4RX)) {

            if ( sl->version.stlink_v == 1) {
                printf("STLINK V1 cannot read voltage, defaulting to 32-bit "
                       "writes on F4 devices\n");
                write_flash_cr_psiz(sl, 2);
            } else {
                // set parallelism to 32 bit
                int voltage = stlink_target_voltage(sl);

                if (voltage == -1) {
                    printf("Failed to read Target voltage\n");
                    return(voltage);
                } else if (voltage > 2700) {
                    printf("enabling 32-bit flash writes\n");
                    write_flash_cr_psiz(sl, 2);
                } else {
                    printf("Target voltage (%d mV) too low for 32-bit flash, "
                           "using 8-bit flash writes\n", voltage);
                    write_flash_cr_psiz(sl, 0);
                }
            }
        } else {
            // L4 does not have a byte-write mode
            int voltage = stlink_target_voltage(sl);

            if (voltage == -1) {
                printf("Failed to read Target voltage\n");
                return(voltage);
            } else if (voltage < 1710) {
                printf("Target voltage (%d mV) too low for flash writes!\n", voltage);
                return(-1);
            }
        }

        // set programming mode
        set_flash_cr_pg(sl);

        size_t buf_size = (sl->sram_size > 0x8000) ? 0x8000 : 0x4000;

        for (off = 0; off < len;) {
            size_t size = len - off > buf_size ? buf_size : len - off;
            printf("size: %u\n", (unsigned int)size);

            if (stlink_flash_loader_run(sl, &fl, addr + (uint32_t)off, base + off, size) == -1) {
                ELOG("stlink_flash_loader_run(%#zx) failed! == -1\n", addr + off);
                return(-1);
            }

            off += size;
        }

        clear_flash_cr_pg(sl);
        lock_flash(sl);

    // STM32F4END
    } else if (sl->flash_type == STLINK_FLASH_TYPE_WB ||
               sl->flash_type == STLINK_FLASH_TYPE_G0 ||
               sl->flash_type == STLINK_FLASH_TYPE_G4) {
        fprintf(stdout, "Writing\r\n");
        fflush(stdout);
        wait_flash_busy(sl);
        unlock_flash_if(sl); // unlock flash if necessary
        set_flash_cr_pg(sl); // set PG 'allow programming' bit
        // write all words.
        off = 0;
        fprintf(stdout, "Starting %3u page write\r\n", (unsigned int)(len / sl->flash_pgsz));
        fflush(stdout);

        for ( ; off < len; off += sizeof(uint32_t)) {
            uint32_t data;

            if (off > 254) { fprintf(stdout, "\r"); }

            if ((off % sl->flash_pgsz) > (sl->flash_pgsz - 5)) {
                fprintf(stdout, "\r%3u/%3u pages written",
                        (unsigned int)(off / sl->flash_pgsz),
                        (unsigned int)(len / sl->flash_pgsz));
                fflush(stdout);
            }

            write_uint32((unsigned char*)&data, *(uint32_t*)(base + off));
            stlink_write_debug32(sl, addr + (uint32_t)off, data);
            wait_flash_busy(sl); // wait for 'busy' bit in FLASH_SR to clear
        }

        // flash writes happen as 2 words at a time
        if ((off / sizeof(uint32_t)) % 2 != 0) {
            stlink_write_debug32(sl, addr + (uint32_t)off, 0); // write a single word of zeros
            wait_flash_busy(sl); // wait for 'busy' bit in FLASH_SR to clear
        }

        clear_flash_cr_pg(sl); // reset PG bit.
        lock_flash(sl);
    } else if (sl->flash_type == STLINK_FLASH_TYPE_L0) {
        // use fast word write
        // TODO: half page
        uint32_t val;
        uint32_t flash_regs_base;
        uint32_t pagesize;

        if (sl->chip_id == STLINK_CHIPID_STM32_L0 ||
            sl->chip_id == STLINK_CHIPID_STM32_L0_CAT5 ||
            sl->chip_id == STLINK_CHIPID_STM32_L0_CAT2 ||
            sl->chip_id == STLINK_CHIPID_STM32_L011) {
            flash_regs_base = STM32L0_FLASH_REGS_ADDR;
            pagesize = L0_WRITE_BLOCK_SIZE;
        } else {
            flash_regs_base = STM32L_FLASH_REGS_ADDR;
            pagesize = L1_WRITE_BLOCK_SIZE;
        }

        // TODO: check write operation

        // disable pecr protection
        stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF, FLASH_L0_PEKEY1);
        stlink_write_debug32(sl, flash_regs_base + FLASH_PEKEYR_OFF, FLASH_L0_PEKEY2);

        // check pecr.pelock is cleared
        stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);

        if (val & (1 << 0)) {
            fprintf(stderr, "pecr.pelock not clear\n");
            return(-1);
        }

        // unlock program memory
        stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, FLASH_L0_PRGKEY1);
        stlink_write_debug32(sl, flash_regs_base + FLASH_PRGKEYR_OFF, FLASH_L0_PRGKEY2);

        // check pecr.prglock is cleared
        stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);

        if (val & (1 << 1)) {
            fprintf(stderr, "pecr.prglock not clear\n");
            return(-1);
        }

        off = 0;

        if (len > pagesize) {
            if (stm32l1_write_half_pages(sl, addr, base, len, pagesize) == -1) {
                // this may happen on a blank device!
                WLOG("\nwrite_half_pages failed == -1\n");
            } else {
                off = (len / pagesize) * pagesize;
            }
        }

        // write remaining word in program memory
        for ( ; off < len; off += sizeof(uint32_t)) {
            uint32_t data;

            if (off > 254) { fprintf(stdout, "\r"); }

            if ((off % sl->flash_pgsz) > (sl->flash_pgsz - 5)) {
                fprintf(stdout, "\r%3u/%3u pages written",
                        (unsigned int)(off / sl->flash_pgsz),
                        (unsigned int)(len / sl->flash_pgsz));
                fflush(stdout);
            }

            write_uint32((unsigned char*)&data, *(uint32_t*)(base + off));
            stlink_write_debug32(sl, addr + (uint32_t)off, data);

            // wait for sr.busy to be cleared
            do {
                stlink_read_debug32(sl, flash_regs_base + FLASH_SR_OFF, &val);
            } while ((val & (1 << 0)) != 0);

            // TODO: check redo write operation
        }

        fprintf(stdout, "\n");
        // reset lock bits
        stlink_read_debug32(sl, flash_regs_base + FLASH_PECR_OFF, &val);
        val |= (1 << 0) | (1 << 1) | (1 << 2);
        stlink_write_debug32(sl, flash_regs_base + FLASH_PECR_OFF, val);
    } else if ((sl->flash_type == STLINK_FLASH_TYPE_F0) ||
               (sl->flash_type == STLINK_FLASH_TYPE_F1_XL)) {
        ILOG("Starting Flash write for VL/F0/F3/F1_XL core id\n");

        // flash loader initialisation
        if (stlink_flash_loader_init(sl, &fl) == -1) {
            ELOG("stlink_flash_loader_init() == -1\n");
            return(-1);
        }

        int write_block_count = 0;

        for (off = 0; off < len; off += sl->flash_pgsz) {
            // adjust last write size
            size_t size = sl->flash_pgsz;

            if ((off + sl->flash_pgsz) > len) { size = len - off; }

            // unlock and set programming mode
            unlock_flash_if(sl);

            if (sl->flash_type != STLINK_FLASH_TYPE_F1_XL) { set_flash_cr_pg(sl); }

            DLOG("Finished unlocking flash, running loader!\n");

            if (stlink_flash_loader_run(sl, &fl, addr + (uint32_t)off, base + off, size) == -1) {
                ELOG("stlink_flash_loader_run(%#zx) failed! == -1\n", addr + off);
                return(-1);
            }

            lock_flash(sl);

            if (sl->verbose >= 1) {
                // show progress; writing procedure is slow and previous errors are misleading
                fprintf(stdout, "\r%3u/%lu pages written", ++write_block_count,
                        (unsigned long)((len + sl->flash_pgsz - 1) / sl->flash_pgsz));
                fflush(stdout);
            }
        }

        fprintf(stdout, "\n");
    } else {
        ELOG("unknown coreid, not sure how to write: %x\n", sl->core_id);
        return(-1);
    }

    return(stlink_verify_write_flash(sl, addr, base, len));
}

// TODO: length not checked
static uint8_t stlink_parse_hex(const char* hex) {
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
            return(0); // error
        }
    }

    return((d[0] << 4) | (d[1]));
}

int stlink_parse_ihex(const char* path, uint8_t erased_pattern, uint8_t * * mem, size_t * size,
        uint32_t * begin) {
    int res = 0;
    *begin = UINT32_MAX;
    uint8_t* data = NULL;
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
                ELOG("Cannot allocate %d bytes\n", *size);
                res = -1;
                break;
            }

            memset(data, erased_pattern, *size);
        }

        FILE* file = fopen(path, "r");

        if (!file) {
            ELOG("Cannot open file\n");
            res = -1;
            break;
        }

        uint32_t lba = 0;
        char line[1 + 5 * 2 + 255 * 2 + 2];

        while (fgets(line, sizeof(line), file)) {
            if (line[0] == '\n' || line[0] == '\r') { continue; } // skip empty lines

            if (line[0] != ':') { // no marker - wrong file format
                ELOG("Wrong file format - no marker\n");
                res = -1;
                break;
            }

            size_t l = strlen(line);

            while (l > 0 && (line[l - 1] == '\n' || line[l - 1] == '\r')) { --l; } // trim EoL

            if ((l < 11) || (l == (sizeof(line) - 1))) { // line too short or long - wrong file format
                ELOG("Wrong file format - wrong line length\n");
                res = -1;
                break;
            }

            uint8_t chksum = 0; // check sum

            for (size_t i = 1; i < l; i += 2) { chksum += stlink_parse_hex(line + i); }

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

            uint16_t offset  = ((uint16_t)stlink_parse_hex(line + 3) << 8) |
                               ((uint16_t)stlink_parse_hex(line + 5));
            uint8_t rectype = stlink_parse_hex(line + 7);

            switch (rectype) {
            case 0: /* Data */
                if (scan == 0) {
                    uint32_t b = lba + offset;
                    uint32_t e = b + reclen - 1;

                    if (b < *begin) { *begin = b; }

                    if (e > end) { end = e; }
                } else {
                    for (uint8_t i = 0; i < reclen; ++i) {
                        uint8_t b = stlink_parse_hex(line + 9 + i * 2);
                        uint32_t addr = lba + offset + i;

                        if (addr >= *begin && addr <= end) { data[addr - *begin] = b; }
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

            if (res != 0) { break; }
        }

        fclose(file);
    }

    if (res == 0) {
        *mem = data;
    } else {
        free(data);
    }

    return(res);
}

uint8_t stlink_get_erased_pattern(stlink_t *sl) {
    if (sl->flash_type == STLINK_FLASH_TYPE_L0) {
        return(0x00);
    } else {
        return(0xff);
    }
}

int stlink_mwrite_flash(stlink_t *sl, uint8_t* data, uint32_t length, stm32_addr_t addr) {
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
            if (data[--idx] != erased_pattern) { break; }

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
                            (num_empty == length) ? (uint32_t)length : (uint32_t)length - num_empty,
                            num_empty == length);
    stlink_fwrite_finalize(sl, addr);
    return(err);
}

/**
 * Write the given binary file into flash at address "addr"
 * @param sl
 * @param path readable file path, should be binary image
 * @param addr where to start writing
 * @return 0 on success, -ve on failure.
 */
int stlink_fwrite_flash(stlink_t *sl, const char* path, stm32_addr_t addr) {
    /* Write the file in flash at addr */
    int err;
    unsigned int num_empty, idx;
    uint8_t erased_pattern = stlink_get_erased_pattern(sl);
    mapped_file_t mf = MAPPED_FILE_INITIALIZER;

    if (map_file(&mf, path) == -1) {
        ELOG("map_file() == -1\n");
        return(-1);
    }

    printf("file %s ", path);
    md5_calculate(&mf);
    stlink_checksum(&mf);

    if (sl->opt) {
        idx = (unsigned int)mf.len;

        for (num_empty = 0; num_empty != mf.len; ++num_empty) {
             if (mf.base[--idx] != erased_pattern) { break; }
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
    err = stlink_write_flash(sl, addr, mf.base,
                             (num_empty == mf.len) ? (uint32_t)mf.len : (uint32_t)mf.len - num_empty,
                             num_empty == mf.len);
    stlink_fwrite_finalize(sl, addr);
    unmap_file(&mf);
    return(err);
}

/**
 * Write option bytes
 * @param sl
 * @param addr of the memory mapped option bytes
 * @param base option bytes to write
 * @return 0 on success, -ve on failure.
 */
static int stlink_write_option_bytes_gx(
        stlink_t *sl, uint8_t* base, stm32_addr_t addr, uint32_t len) {
    /* Write options bytes */
    uint32_t val;
    int ret = 0;
    (void)len;
    uint32_t data;

    write_uint32((unsigned char*)&data, *(uint32_t*)(base));
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

    return(ret);
}

/**
 * Write option bytes
 * @param sl
 * @param addr of the memory mapped option bytes
 * @param base option bytes to write
 * @return 0 on success, -ve on failure.
 */
static int stlink_write_option_bytes_l0(
        stlink_t *sl, uint8_t* base, stm32_addr_t addr, uint32_t len) {
    uint32_t flash_base = get_stm32l0_flash_base(sl);
    uint32_t val;
    uint32_t data;
    int ret = 0;

    // Clear errors
    stlink_write_debug32(sl, flash_base + FLASH_SR_OFF, STM32L0_FLASH_REGS_ADDR);

    while (len != 0) {
        write_uint32((unsigned char*)&data, *(uint32_t*)(base)); // write options bytes

        WLOG("Writing option bytes %#10x to %#10x\n", data, addr);
        stlink_write_debug32(sl, addr, data);
        wait_flash_busy(sl);

        if ((ret = check_flash_error(sl))) { break; }

        len -= 4;
        addr += 4;
        base += 4;
    }

    // Reload options
    stlink_read_debug32(sl, flash_base + FLASH_PECR_OFF, &val);
    val |= (1 << STM32L0_FLASH_OBL_LAUNCH);
    stlink_write_debug32(sl, flash_base + FLASH_PECR_OFF, val);

    return(ret);
}

/**
 * Write option bytes
 * @param sl
 * @param addr of the memory mapped option bytes
 * @param base option bytes to write
 * @return 0 on success, -ve on failure.
 */
static int stlink_write_option_bytes_l4(
        stlink_t *sl, uint8_t* base, stm32_addr_t addr, uint32_t len) {

    uint32_t val;
    int ret = 0;
    (void)addr;
    (void)len;

    // write options bytes
    uint32_t data;
    write_uint32((unsigned char*)&data, *(uint32_t*)(base));
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

    return(ret);
}


/**
 * Write option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
static int stlink_write_option_bytes_f4(
        stlink_t *sl, uint8_t* base, stm32_addr_t addr, uint32_t len) {
    uint32_t option_byte;
    int ret = 0;
    (void)addr;
    (void)len;

    write_uint32((unsigned char*)&option_byte, *(uint32_t*)(base));

    // write option byte, ensuring we dont lock opt, and set strt bit
    stlink_write_debug32(sl, FLASH_F4_OPTCR,
                        (option_byte & ~(1 << FLASH_F4_OPTCR_LOCK)) | (1 << FLASH_F4_OPTCR_START));

    wait_flash_busy(sl);
    ret = check_flash_error(sl);

    // option bytes are reloaded at reset only, no obl. */
    return(ret);
}

/**
 * Write option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
static int stlink_write_option_bytes_f7(stlink_t *sl, uint8_t* base, stm32_addr_t addr, uint32_t len) {
    uint32_t option_byte;
    int ret = 0;

    //(void) addr;
    //(void) len;

    ILOG("Asked to write option byte %#10x to %#010x.\n", *(uint32_t*) (base), addr);
    write_uint32((unsigned char*) &option_byte, *(uint32_t*) (base));
    ILOG("Write %d option bytes %#010x to %#010x!\n", len, option_byte, addr);

    if ( addr == 0 ) {
        addr = FLASH_F7_OPTCR;
        ILOG("No address provided, using %#10x\n", addr);
    }
    
    if ( addr == FLASH_F7_OPTCR ) {
        /* write option byte, ensuring we dont lock opt, and set strt bit */
        stlink_write_debug32(sl, FLASH_F7_OPTCR, (option_byte & ~(1 << FLASH_F7_OPTCR_LOCK)) | (1 << FLASH_F7_OPTCR_START));
    } else if ( addr == FLASH_F7_OPTCR1 ) {
        // Read FLASH_F7_OPTCR
        uint32_t oldvalue;
        stlink_read_debug32(sl, FLASH_F7_OPTCR, &oldvalue);
        /* write option byte */
        stlink_write_debug32(sl, FLASH_F7_OPTCR1, option_byte);
        // Write FLASH_F7_OPTCR lock and start address
        stlink_write_debug32(sl, FLASH_F7_OPTCR, (oldvalue & ~(1 << FLASH_F7_OPTCR_LOCK)) | (1 << FLASH_F7_OPTCR_START));
    } else {
        WLOG("WIP: write %#010x to address %#010x\n", option_byte, addr);
        stlink_write_debug32(sl, addr, option_byte);
    }

    wait_flash_busy(sl);

    ret = check_flash_error(sl);
    if (!ret)
        ILOG("Wrote %d option bytes %#010x to %#010x!\n", len, *(uint32_t*) base, addr);
    
    /* option bytes are reloaded at reset only, no obl. */

    return ret;
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register_Gx(stlink_t *sl, uint32_t* option_byte) {
    return stlink_read_debug32(sl, STM32Gx_FLASH_OPTR, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_bytes_Gx(stlink_t *sl, uint32_t* option_byte) {
    return stlink_read_option_control_register_Gx(sl, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register_f2(stlink_t *sl, uint32_t* option_byte) {
    return stlink_read_debug32(sl, FLASH_F2_OPT_CR, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_bytes_f2(stlink_t *sl, uint32_t* option_byte) {
    return stlink_read_option_control_register_f2(sl, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register_f4(stlink_t *sl, uint32_t* option_byte) {
    return stlink_read_debug32(sl, FLASH_F4_OPTCR, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_bytes_f4(stlink_t *sl, uint32_t* option_byte) {
    return stlink_read_option_control_register_f4(sl, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register_f7(stlink_t *sl, uint32_t* option_byte) {
    DLOG("@@@@ Read option control register byte from %#10x\n", FLASH_F7_OPTCR);
    return stlink_read_debug32(sl, FLASH_F7_OPTCR, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register1_f7(stlink_t *sl, uint32_t* option_byte) {
    DLOG("@@@@ Read option control register 1 byte from %#10x\n", FLASH_F7_OPTCR1);
    return stlink_read_debug32(sl, FLASH_F7_OPTCR1, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_bytes_boot_add_f7(stlink_t *sl, uint32_t* option_byte) {
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
int stlink_read_option_bytes_f7(stlink_t *sl, uint32_t* option_byte) {
    int err = -1;
    for (uint8_t counter = 0; counter < (sl->option_size / 4 - 1); counter++) {
        err = stlink_read_debug32(sl, sl->option_base + counter * sizeof(uint32_t), option_byte);
        if (err == -1) {
            return err;
        } else {
            printf("%08x\n", *option_byte);
        }
    }

    return stlink_read_debug32(sl, sl->option_base + (uint32_t) (sl->option_size / 4 - 1) * sizeof(uint32_t), option_byte);
}

/**
 * Read first option bytes
 * @param sl
 * @param option_byte option value
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_bytes_generic(stlink_t *sl, uint32_t* option_byte) {
    DLOG("@@@@ Read option bytes boot address from %#10x\n", sl->option_base);
    return stlink_read_debug32(sl, sl->option_base, option_byte);
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
//int stlink_read_option_bytes_boot_add_generic(stlink_t *sl, uint32_t* option_byte) {
//    DLOG("@@@@ Read option bytes boot address from %#10x\n", sl->option_base);
//    return stlink_read_debug32(sl, sl->option_base, option_byte);
//}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
//int stlink_read_option_control_register_generic(stlink_t *sl, uint32_t* option_byte) {
//    DLOG("@@@@ Read option control register byte from %#10x\n", sl->option_base);
//    return stlink_read_debug32(sl, sl->option_base, option_byte);
//}

/**
 * Read option bytes
 * @param sl
 * @param option_byte value to read
 * @return 0 on success, -ve on failure.
 */
//int stlink_read_option_control_register1_generic(stlink_t *sl, uint32_t* option_byte) {
//    DLOG("@@@@ Read option control register 1 byte from %#10x\n", sl->option_base);
//    return stlink_read_debug32(sl, sl->option_base, option_byte);
//}

/**
 * Read option bytes
 * @param sl
 * @param option_byte option value
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_bytes32(stlink_t *sl, uint32_t* option_byte) {
    if (sl->option_base == 0) {
        ELOG("Option bytes read is currently not supported for connected chip\n");
        return(-1);
    }

    switch (sl->chip_id) {
    case STLINK_CHIPID_STM32_F2:
        return stlink_read_option_bytes_f2(sl, option_byte);
    case STLINK_CHIPID_STM32_F446:
        return stlink_read_option_bytes_f4(sl, option_byte);
    case STLINK_CHIPID_STM32_F7XXXX:
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
int stlink_read_option_bytes_boot_add32(stlink_t *sl, uint32_t* option_byte)
{
    if (sl->option_base == 0) {
        ELOG("Option bytes boot address read is currently not supported for connected chip\n");
        return -1;
    }

    switch (sl->chip_id) {
    case STLINK_CHIPID_STM32_F7XXXX:
        return stlink_read_option_bytes_boot_add_f7(sl, option_byte);
    default:
        return -1;
        //return stlink_read_option_bytes_boot_add_generic(sl, option_byte);
    }
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte option value
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register32(stlink_t *sl, uint32_t* option_byte)
{
    if (sl->option_base == 0) {
        ELOG("Option bytes read is currently not supported for connected chip\n");
        return -1;
    }

    switch (sl->chip_id) {
    case STLINK_CHIPID_STM32_F7XXXX:
        return stlink_read_option_control_register_f7(sl, option_byte);
    default:
        return -1;
        //return stlink_read_option_control_register_generic(sl, option_byte);
    }
}

/**
 * Read option bytes
 * @param sl
 * @param option_byte option value
 * @return 0 on success, -ve on failure.
 */
int stlink_read_option_control_register1_32(stlink_t *sl, uint32_t* option_byte)
{
    if (sl->option_base == 0) {
        ELOG("Option bytes read is currently not supported for connected chip\n");
        return -1;
    }

    switch (sl->chip_id) {
    case STLINK_CHIPID_STM32_F7XXXX:
        return stlink_read_option_control_register1_f7(sl, option_byte);
    default:
        return -1;
        //return stlink_read_option_control_register1_generic(sl, option_byte);
    }
}

/**
 * Write option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
int stlink_write_option_bytes32(stlink_t *sl, uint32_t option_byte)
{
    WLOG("About to write option byte %#10x to %#10x.\n", option_byte, sl->option_base);
    return stlink_write_option_bytes(sl, sl->option_base, (uint8_t *) &option_byte, 4);
}

/**
 * Write option bytes
 * @param sl
 * @param addr of the memory mapped option bytes
 * @param base option bytes to write
 * @return 0 on success, -ve on failure.
 */
int stlink_write_option_bytes(stlink_t *sl, stm32_addr_t addr, uint8_t* base, uint32_t len) {
    int ret = -1;

    if (sl->option_base == 0) {
        ELOG("Option bytes writing is currently not supported for connected chip\n");
        return(-1);
    }

    
    if ((addr < sl->option_base) || addr > sl->option_base + sl->option_size) {
        ELOG("Option bytes start address out of Option bytes range\n");
        return(-1);
    }

    if (addr + len > sl->option_base + sl->option_size) {
        ELOG("Option bytes data too long\n");
        return(-1);
    }

    wait_flash_busy(sl);

    if (unlock_flash_if(sl)) {
        ELOG("Flash unlock failed! System reset required to be able to unlock it again!\n");
        return(-1);
    }

    if (unlock_flash_option_if(sl)) {
        ELOG("Flash option unlock failed!\n");
        return(-1);
    }

    switch (sl->flash_type) {
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
    default:
        ELOG("Option bytes writing is currently not implemented for connected chip\n");
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
static int stlink_write_option_control_register_f7(stlink_t *sl, uint32_t option_control_register) {
    int ret = 0;

    ILOG("Asked to write option control register 1 %#10x to %#010x.\n", option_control_register, FLASH_F7_OPTCR);
    //write_uint32((unsigned char*) &option_byte, *(uint32_t*) (base));
    //ILOG("Write %d option bytes %#010x to %#010x!\n", len, option_byte, addr);
    
    /* write option byte, ensuring we dont lock opt, and set strt bit */
    stlink_write_debug32(sl, FLASH_F7_OPTCR, (option_control_register & ~(1 << FLASH_F7_OPTCR_LOCK)) | (1 << FLASH_F7_OPTCR_START));

    wait_flash_busy(sl);

    ret = check_flash_error(sl);
    if (!ret)
        ILOG("Wrote option bytes %#010x to %#010x!\n", option_control_register, FLASH_F7_OPTCR1);
    
    return ret;
}

/**
 * Write option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
static int stlink_write_option_control_register1_f7(stlink_t *sl, uint32_t option_control_register1) {
    int ret = 0;

    ILOG("Asked to write option control register 1 %#010x to %#010x.\n", option_control_register1, FLASH_F7_OPTCR1);
    //write_uint32((unsigned char*) &option_byte, *(uint32_t*) (base));
    //ILOG("Write %d option bytes %#010x to %#010x!\n", len, option_byte, addr);
    
    /* write option byte, ensuring we dont lock opt, and set strt bit */
    uint32_t current_control_register_value;
    stlink_read_debug32(sl, FLASH_F7_OPTCR, &current_control_register_value);
    
    /* write option byte */
    stlink_write_debug32(sl, FLASH_F7_OPTCR1, option_control_register1);
    stlink_write_debug32(sl, FLASH_F7_OPTCR, (current_control_register_value & ~(1 << FLASH_F7_OPTCR_LOCK)) | (1 << FLASH_F7_OPTCR_START));

    wait_flash_busy(sl);

    ret = check_flash_error(sl);
    if (!ret)
        ILOG("Wrote option bytes %#010x to %#010x!\n", option_control_register1, FLASH_F7_OPTCR1);
    
    return ret;
}

/**
 * Write option bytes
 * @param sl
 * @param option_byte value to write
 * @return 0 on success, -ve on failure.
 */
static int stlink_write_option_bytes_boot_add_f7(stlink_t *sl, uint32_t option_byte_boot_add) {
    ILOG("Asked to write option byte boot add %#010x.\n", option_byte_boot_add);
    return stlink_write_option_control_register1_f7(sl, option_byte_boot_add);
}

/**
 * Write option bytes
 * @param sl
 * @param option bytes boot address to write
 * @return 0 on success, -ve on failure.
 */
int stlink_write_option_bytes_boot_add32(stlink_t *sl, uint32_t option_bytes_boot_add)
{
    int ret = -1;

    wait_flash_busy(sl);

    if (unlock_flash_if(sl)) {
        ELOG("Flash unlock failed! System reset required to be able to unlock it again!\n");
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
        ELOG("Option bytes boot address writing is currently not implemented for connected chip\n");
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
int stlink_write_option_control_register32(stlink_t *sl, uint32_t option_control_register)
{
    int ret = -1;

    wait_flash_busy(sl);

    if (unlock_flash_if(sl)) {
        ELOG("Flash unlock failed! System reset required to be able to unlock it again!\n");
        return -1;
    }

    if (unlock_flash_option_if(sl)) {
        ELOG("Flash option unlock failed!\n");
        return -1;
    }

    switch (sl->flash_type) {
    case STLINK_FLASH_TYPE_F7:
        ret = stlink_write_option_control_register_f7(sl, option_control_register);
        break;
    default:
        ELOG("Option control register writing is currently not implemented for connected chip\n");
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
int stlink_write_option_control_register1_32(stlink_t *sl, uint32_t option_control_register1)
{
    int ret = -1;

    wait_flash_busy(sl);

    if (unlock_flash_if(sl)) {
        ELOG("Flash unlock failed! System reset required to be able to unlock it again!\n");
        return -1;
    }

    if (unlock_flash_option_if(sl)) {
        ELOG("Flash option unlock failed!\n");
        return -1;
    }

    switch (sl->flash_type) {
    case STLINK_FLASH_TYPE_F7:
        ret = stlink_write_option_control_register1_f7(sl, option_control_register1);
        break;
    default:
        ELOG("Option control register 1 writing is currently not implemented for connected chip\n");
        break;
    }

    if (ret)
        ELOG("Flash option write failed!\n");
    else
        ILOG("Wrote option control register 1 %#010x!\n", option_control_register1);

    lock_flash_option(sl);
    lock_flash(sl);

    return(ret);
}

/**
 * Write the given binary file with option bytes
 * @param sl
 * @param path readable file path, should be binary image
 * @param addr of the memory mapped option bytes
 * @return 0 on success, -ve on failure.
 */
int stlink_fwrite_option_bytes(stlink_t *sl, const char* path, stm32_addr_t addr) {
    /* Write the file in flash at addr */
    int err;
    mapped_file_t mf = MAPPED_FILE_INITIALIZER;

    if (map_file(&mf, path) == -1) {
        ELOG("map_file() == -1\n");
        return(-1);
    }

    printf("file %s ", path);
    md5_calculate(&mf);
    stlink_checksum(&mf);

    err = stlink_write_option_bytes(sl, addr, mf.base, (uint32_t)mf.len);
    stlink_fwrite_finalize(sl, addr);
    unmap_file(&mf);

    return(err);
}
