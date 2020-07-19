/*
 * File: stm32.h
 *
 * STM32-specific defines
 */

#ifndef STM32_H
#define STM32_H

/* Cortex core ids */
#define STM32VL_CORE_ID 0x1ba01477
#define STM32F7_CORE_ID 0x5ba02477

/* Constant STM32 memory map figures */
#define STM32_FLASH_BASE           ((uint32_t)0x08000000)
#define STM32_SRAM_BASE            ((uint32_t)0x20000000)
#define STM32_G0_OPTION_BYTES_BASE ((uint32_t)0x1FFF7800)
#define STM32_G4_OPTION_BYTES_BASE ((uint32_t)0x1FFFF800)
#define STM32_L0_CATx_OPTION_BYTES_BASE ((uint32_t)0x1FF80000)
#define STM32_F2_OPTION_BYTES_BASE ((uint32_t)0x1FFFC000)
#define STM32_L4_OPTION_BYTES_BASE ((uint32_t)0x1FFF7800)
#define STM32_L1_OPTION_BYTES_BASE ((uint32_t)0x1FF80000)
#define STM32_F7_OPTION_BYTES_BASE ((uint32_t)0x1FFF0000)

#endif // STM32_H
