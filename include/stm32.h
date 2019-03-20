/*
 * File:   stm32.h
 *
 * STM32 specific defines
 */

#ifndef STM32_H
#define STM32_H

// cortex core ids
#define STM32VL_CORE_ID 0x1ba01477
#define STM32F7_CORE_ID 0x5ba02477

// Constant STM32 memory map figures
#define STM32_FLASH_BASE           ((uint32_t)0x08000000)
#define STM32_SRAM_BASE            ((uint32_t)0x20000000)
#define STM32_G0_OPTION_BYTES_BASE ((uint32_t)0x1FFF7800)

#endif /* STM32_H */
