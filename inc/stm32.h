/*
 * File: stm32.h
 *
 * STM32-specific defines & identification parametres
 */

#ifndef STM32_H
#define STM32_H

#include <stdint.h>

/* STM32 Cortex-M core ids (CPUTAPID) */
enum stm32_core_id {
    STM32_CORE_ID_M0_SWD        = 0x0bb11477,   // (RM0091 Section 32.5.3) F0 SW-DP
    STM32_CORE_ID_M0P_SWD       = 0x0bc11477,   // (RM0444 Section 40.5.3) G0 SW-DP
                                                // (RM0377 Section 27.5.3) L0 SW-DP
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
                                                // (RM0473 Section 33.4.4) WB SW-DP
                                                // (RM0453 Section 38.4.1) WL SW-DP
    STM32_CORE_ID_M7F_JTAG      = 0x5ba00477,   // (RM0385 Section 40.6.3) F7 JTAG
    STM32_CORE_ID_M7F_M33_SWD   = 0x6ba02477,   // (RM0481 Section 58.3.3) H5 SW-DP
                                                // (RM0433 Section 60.4.1) H7 SW-DP
    STM32_CORE_ID_M7F_M33_JTAG  = 0x6ba00477,   // (RM0481 Section 58.3.1) H5 JTAG
                                                // (RM0433 Section 60.4.1) H7 JTAG
                                                // (RM0473 Section 33.4.1) WB JTAG
                                                // (RM0453 Section 38.3.8) WL JTAG
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
    STM32_FLASH_TYPE_L4        =  9,
    STM32_FLASH_TYPE_L5_U5_H5  = 10,
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
    STM32_CHIPID_L0_CAT3          = 0x417,
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
    STM32_CHIPID_L0_CAT1          = 0x457,
    STM32_CHIPID_F410             = 0x458,
    STM32_CHIPID_G0_CAT2          = 0x460, /* G07x/G08x */
    STM32_CHIPID_L496x_L4A6x      = 0x461,
    STM32_CHIPID_L45x_L46x        = 0x462,
    STM32_CHIPID_F413             = 0x463,
    STM32_CHIPID_L41x_L42x        = 0x464,
    STM32_CHIPID_G0_CAT1          = 0x466, /* G03x/G04x */
    STM32_CHIPID_G0_CAT3          = 0x467, /* G0Bx/G0Cx */
    STM32_CHIPID_G4_CAT2          = 0x468, /* RM0440, section 46.6.1 "MCU device ID code" */
    STM32_CHIPID_G4_CAT3          = 0x469,
    STM32_CHIPID_L4Rx             = 0x470, /* RM0432, p.2247, found on the STM32L4R9I-DISCO board */
    STM32_CHIPID_L4PX             = 0x471, /* RM0432, p.2247 */
    STM32_CHIPID_L5x2xx           = 0x472, /* RM0438, p.2157 */
    STM32_CHIPID_G4_CAT4          = 0x479,
    STM32_CHIPID_H7Ax             = 0x480, /* RM0455, p.2863 */
    STM32_CHIPID_U5x5             = 0x482, /* RM0456, p.2991 */
    STM32_CHIPID_H72x             = 0x483, /* RM0468, p.3199 */
    STM32_CHIPID_H5xx             = 0x484, /* RM0481, p.3085 */
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
#define STM32_SRAM_BASE              ((uint32_t)0x20000000)
#define STM32_FLASH_BASE             ((uint32_t)0x08000000)

#define STM32_F1_FLASH_BANK2_BASE    ((uint32_t)0x08080000)
#define STM32_H7_FLASH_BANK2_BASE    ((uint32_t)0x08100000)

#define STM32F0_DBGMCU_CR 0xE0042004
#define STM32F0_DBGMCU_CR_IWDG_STOP 8
#define STM32F0_DBGMCU_CR_WWDG_STOP 9

#define STM32F4_DBGMCU_APB1FZR1 0xE0042008
#define STM32F4_DBGMCU_APB1FZR1_WWDG_STOP 11
#define STM32F4_DBGMCU_APB1FZR1_IWDG_STOP 12

#define STM32L0_DBGMCU_APB1_FZ 0x40015808
#define STM32L0_DBGMCU_APB1_FZ_WWDG_STOP 11
#define STM32L0_DBGMCU_APB1_FZ_IWDG_STOP 12

#define STM32L1_DBGMCU_APB1_FZ 0xE0042008
#define STM32L1_DBGMCU_APB1_FZ_WWDG_STOP 11
#define STM32L1_DBGMCU_APB1_FZ_IWDG_STOP 12

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

#define STM32L1_RCC_AHBENR 0x4002381C
#define STM32L1_RCC_DMAEN 0x30000000 // DMA2EN | DMA1EN

#define STM32L5_RCC_AHB1ENR 0x40021048                  // RM0438, p. 91,377
#define STM32L5_RCC_DMAEN 0x00000003 // DMA2EN | DMA1EN // RM0438, p. 378

#define STM32H7_RCC_AHB1ENR 0x58024538
#define STM32H7_RCC_DMAEN 0x00000003 // DMA2EN | DMA1EN

#define STM32WB_RCC_AHB1ENR 0x58000048
#define STM32WB_RCC_DMAEN 0x00000003 // DMA2EN | DMA1EN

#define STM32L5_PWR_CR1 0x40007000                      // RM0438, p. 93,324
#define STM32L5_PWR_CR1_VOS 9

#endif // STM32_H
