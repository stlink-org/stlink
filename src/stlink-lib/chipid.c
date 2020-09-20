#include <stlink.h>
#include "chipid.h"

static const struct stlink_chipid_params devices[] = {
    {
        // unknown device
        .chip_id = STLINK_CHIPID_UNKNOWN,
        .description = "unknown device",
        .flash_type = STLINK_FLASH_TYPE_UNKNOWN,
        .flash_size_reg = 0x0,
        .flash_pagesize = 0x0,
        .sram_size = 0x0,
        .bootrom_base = 0x0,
        .bootrom_size = 0x0
    },

    /* == STM32F0 == */

    {
        // RM0091
        .chip_id = STLINK_CHIPID_STM32_F0,
        .description = "F0xx",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,          // "Flash size data register"
        .flash_pagesize = 0x400,               // Page sizes listed in table 4
        .sram_size = 0x2000,                   // "SRAM" byte size in hex from table 2
        .bootrom_base = 0x1fffec00,            // "System memory" starting address from table 2
        .bootrom_size = 0xC00                  // "System memory" byte size in hex from table 2
    },
    {
        // RM0091
        .chip_id = STLINK_CHIPID_STM32_F0_SMALL,
        .description = "F0xx small",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,         // "Flash size data register"
        .flash_pagesize = 0x400,              // Page sizes listed in table 4
        .sram_size = 0x1000,                  // "SRAM" byte size in hex from table 2
        .bootrom_base = 0x1fffec00,           // "System memory" starting address from table 2
        .bootrom_size = 0xC00                 // "System memory" byte size in hex from table 2
    },
    {
        // RM0091
        .chip_id = STLINK_CHIPID_STM32_F04,
        .description = "F04x",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,          // "Flash size data register"
        .flash_pagesize = 0x400,               // Page sizes listed in table 4
        .sram_size = 0x1800,                   // "SRAM" byte size in hex from table 2
        .bootrom_base = 0x1fffec00,            // "System memory" starting address from table 2
        .bootrom_size = 0xC00                  // "System memory" byte size in hex from table 2
    },
    {
        // RM0091
        .chip_id = STLINK_CHIPID_STM32_F0_CAN,
        .description = "F07x",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,          // "Flash size data register"
        .flash_pagesize = 0x800,               // Page sizes listed in table 4
        .sram_size = 0x4000,                   // "SRAM" byte size in hex from table 2
        .bootrom_base = 0x1fffC800,            // "System memory" starting address from table 2
        .bootrom_size = 0x3000                 // "System memory" byte size in hex from table 2
    },
    {   // RM0091
        .chip_id = STLINK_CHIPID_STM32_F09X,
        .description = "F09X",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,          // "Flash size data register"
        .flash_pagesize = 0x800,               // Page sizes listed in table 4
        .sram_size = 0x8000,                   // "SRAM" byte size in hex from table 2
        .bootrom_base = 0x1fffd800,            // "System memory" starting address from table 2
        .bootrom_size = 0x2000                 // "System memory" byte size in hex from table 2
    },

    /* == STM32F1 == */

    {   
        // PM0063
        .chip_id = STLINK_CHIPID_STM32_F1_LOW,
        .description = "F1 Low-density",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x400,
        .sram_size = 0x2800,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800
    },
    {   
        // PM0063, Table 2
        .chip_id = STLINK_CHIPID_STM32_F1_MEDIUM,
        .description = "F1xx Medium-density",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x400,
        .sram_size = 0x5000,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800
    },
    {   
        // TODO: Add missing reference here!
        .chip_id = STLINK_CHIPID_STM32_F1_HIGH,
        .description = "F1xx High-density",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x800,
        .sram_size = 0x10000,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800
    },
    {   
        // RM0041 25.6.1 - Low and Medium density VL have same chipid.
        .chip_id = STLINK_CHIPID_STM32_F1_VL_MEDIUM_LOW,
        .description = "F1xx Value Line",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x400,
        .sram_size = 0x2000, // 0x1000 for low density devices
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800
    },
    {   
        // TODO: Add missing reference here!
        .chip_id = STLINK_CHIPID_STM32_F1_VL_HIGH,
        .description = "F1xx High-density Value Line",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x800,
        .sram_size = 0x8000,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800
    },
    {   
        // TODO: Add missing reference here!
        .chip_id = STLINK_CHIPID_STM32_F1_XL,
        .description = "F1xx XL-density",
        .flash_type = STLINK_FLASH_TYPE_F1_XL,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x800,
        .sram_size = 0x18000,
        .bootrom_base = 0x1fffe000,
        .bootrom_size = 0x1800
    },
    {   
        // TODO: Add missing reference here!
        .chip_id = STLINK_CHIPID_STM32_F1_CONN,
        .description = "F1 Connectivity Line",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x800,
        .sram_size = 0x10000,
        .bootrom_base = 0x1fffb000,
        .bootrom_size = 0x4800
    },

    /* == STM32F2 == */

    {   
        // PM0059, Table 1
        .chip_id = STLINK_CHIPID_STM32_F2,
        .description = "F2xx",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1fff7a22, // as in RM0033 Rev. 5
        .flash_pagesize = 0x20000,
        .sram_size = 0x20000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800,
        .option_base = 0x1FFFC000,
        .option_size = 4,
    },

    /* == STM32F3 == */

    {
        // STM32F30x
        // TODO: Add missing reference here!
        .chip_id = STLINK_CHIPID_STM32_F3_SMALL,
        .description = "F3xx small",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,
        .flash_pagesize = 0x800,
        .sram_size = 0xa000,
        .bootrom_base = 0x1fffd800,
        .bootrom_size = 0x2000
    },
    {
        // STM32F303VCT6 device from the STM32F3 Discovery board
        // RM0316
        .chip_id = STLINK_CHIPID_STM32_F3,
        .description = "F3xx",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,
        .flash_pagesize = 0x800,
        .sram_size = 0xa000,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800
    },
    {
        // STM32F373VCT6 device from the STM32F373 evaluation board
        // Support based on F303 above (F30x and F37x have the same memory map).
        .chip_id = STLINK_CHIPID_STM32_F37x,
        .description = "F37x",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,
        .flash_pagesize = 0x800,
        .sram_size = 0xa000,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800
    },
    {
        // STM32F303RET6 device from the STM32F3 Nucleo board
        // RM0316 Rev. 5
        .chip_id = STLINK_CHIPID_STM32_F303_HIGH,
        .description = "F303 high-density",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,          // Flash size data register (34.2.1)
        .flash_pagesize = 0x800,               // Flash memory organisation (4.2.1)
        .sram_size = 0x10000,                  // Embedded SRAM (3.3)
        .bootrom_base = 0x1fffd800,            // System Memory (3.3.2 Table 4)
        .bootrom_size = 0x2000
    },
    {
        // STM32F334, STM32F303x6/8 and STM32F328
        // RM0316 & RM0364
        .chip_id = STLINK_CHIPID_STM32_F334,
        .description = "F334 medium-density", // RM0316 (Section 33.6.1)
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,
        .flash_pagesize = 0x800,
        .sram_size = 0x3000,
        .bootrom_base = 0x1fffd800,
        .bootrom_size = 0x2000
    },

    /* == STM32F4 == */

    {   
        // TODO: Add missing reference here!
        .chip_id = STLINK_CHIPID_STM32_F4_LP,
        .description = "F4xx (Low Power)",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x10000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800
    },
    {   
        // RM0090 Rev. 2
        .chip_id = STLINK_CHIPID_STM32_F4,
        .description = "F4xx",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x30000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800
    },
    {   
        // TODO: Add missing reference here!
        .chip_id = STLINK_CHIPID_STM32_F4_DE,
        .description = "F4xx (Dynamic Efficiency)",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x18000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800
    },
    {
        // STM32F410
        // RM0401
        .chip_id = STLINK_CHIPID_STM32_F410,
        .description = "F410",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1fff7a22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x8000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800
    },
    {   
        // STM32F411RE
        // TODO: Add missing reference here!
        .chip_id = STLINK_CHIPID_STM32_F411RE,
        .description = "F411RE",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x20000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800
    },
    {
        // STM32F412
        // RM0402
        .chip_id = STLINK_CHIPID_STM32_F412,
        .description = "F412",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,          // "Flash size data register"
        .flash_pagesize = 0x4000,              // Flash module organisation from table 5
        .sram_size = 0x40000,                  // "SRAM" byte size in hex from table 4
        .bootrom_base = 0x1FFF0000,            // "System memory" starting address from table 4
        .bootrom_size = 0x7800                 // "System memory" byte size in hex from table 4
    },
    {
        // RM0430 Rev. 2
        .chip_id = STLINK_CHIPID_STM32_F413,
        .description = "F413",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,          // "Flash size data register" (Section 35.2)
        .flash_pagesize = 0x4000,              // Flash module organisation from table 5 (variable sector sizes, but 0x4000 is smallest)
        .sram_size = 0x50000,                  // "SRAM" byte size in hex from figure 2 (Table 4 only says 0x40000)
        .bootrom_base = 0x1FFF0000,            // "System memory" starting address from table 4
        .bootrom_size = 0x7800                 // "System memory" byte size in hex from table 4
    },
    {   
        // RM0090 Rev. 2
        .chip_id = STLINK_CHIPID_STM32_F4_HD,
        .description = "F42x/F43x",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x40000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800
    },
    {
        // STM32F446x
        // RM0390
        .chip_id = STLINK_CHIPID_STM32_F446,
        .description = "F446x",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1fff7a22,
        .flash_pagesize = 0x20000,
        .sram_size = 0x20000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800,
        .option_base = 0x1FFFC000,
        .option_size = 4,
    },
    {   
        // RM0090 Rev. 2
        .chip_id = STLINK_CHIPID_STM32_F4_DSI,
        .description = "F46x/F47x",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x40000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800
    },

    /* == STM32F7 == */

    {
        // RM0385 & DS10916
        .chip_id = STLINK_CHIPID_STM32_F7,
        .description = "F7xx",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1ff0f442,               // Section 41.2
        .flash_pagesize = 0x800,                    // No flash pages
        .sram_size = 0x50000,                       // "SRAM" byte size in hex from figure 18
        .bootrom_base = 0x00100000,                 // "System memory" starting address from figure 18
        .bootrom_size = 0xEDC0                      // "System memory" byte size in hex from figure 18
    },
    {
        // RM0431
        .chip_id = STLINK_CHIPID_STM32_F72XXX,
        .description = "F72x/F73x",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1ff07a22,               // Section 35.2
        .flash_pagesize = 0x800,                    // No flash pages
        .sram_size = 0x40000,                       // "SRAM" byte size in hex from figure 24
        .bootrom_base = 0x00100000,                 // "System memory" starting address from figure 24
        .bootrom_size = 0xEDC0                      // "System memory" byte size in hex from figure 24
    },
    {
        // RM0410
        .chip_id = STLINK_CHIPID_STM32_F7XXXX,
        .description = "F76xxx",
        .flash_type = STLINK_FLASH_TYPE_F7,
        .flash_size_reg = 0x1ff0f442,               // Section 45.2
        .flash_pagesize = 0x800,                    // No flash pages
        .sram_size = 0x80000,                       // "SRAM" byte size in hex
        .bootrom_base = 0x00200000,                 // ! "System memory" starting address
        .bootrom_size = 0xEDC0,                     // ! @todo "System memory" byte size in hex
        .option_base = STM32_F7_OPTION_BYTES_BASE,  // Used for reading back the option bytes, writing uses FLASH_F7_OPTCR and FLASH_F7_OPTCR1
        .option_size = 0x20
    },


    /* == STM32G0 == */

    {
        // STM32G030/031/041
        // RM0454 & RM0444
        .chip_id = STLINK_CHIPID_STM32_G0_CAT1,
        .description = "G030/G031/G041",
        .flash_type = STLINK_FLASH_TYPE_G0,
        .flash_size_reg = 0x1FFF75E0,          // Section 38.2
        .flash_pagesize = 0x800,               // 2k (Section 3.2)
        .sram_size = 0x2000,                   // 8k (Section 2.3)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x2000,                // 8k (Section 2.2.2 Table 3)
        .option_base = STM32_G0_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STM32G071/081
        // RM0444
        .chip_id = STLINK_CHIPID_STM32_G0_CAT2,
        .description = "G070/G071/G081",
        .flash_type = STLINK_FLASH_TYPE_G0,
        .flash_size_reg = 0x1FFF75E0,          // Section 38.2
        .flash_pagesize = 0x800,               // 2k (Section 3.2)
        .sram_size = 0x9000,                   // 36k (Section 2.3)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7000,                // 28k (Section 2.2.2 Table 2)
        .option_base = STM32_G0_OPTION_BYTES_BASE,
        .option_size = 4,
    },

    /* == STM32G4 == */

    {
        // STM32G431/441
        // RM0440
        .chip_id = STLINK_CHIPID_STM32_G4_CAT2,
        .description = "G4 Category-2",
        .flash_type = STLINK_FLASH_TYPE_G4,
        .flash_size_reg = 0x1FFF75E0,              // Section 47.2
        .flash_pagesize = 0x800,                   // 2k (Section 3.3.1)
        // SRAM1 is 16k at 0x20000000
        // SRAM2 is 6k at 0x20014000
        // SRAM3/CCM is 10k at 0x10000000, aliased at 0x20018000
        .sram_size = 0x8000,                       // 32k (Section 2.4)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7000,                    // 28k (Table 2)
        .option_base = STM32_G4_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STM32G471/473/474/483/484
        // RM0440
        .chip_id = STLINK_CHIPID_STM32_G4_CAT3,
        .description = "G4 Category-3",
        .flash_type = STLINK_FLASH_TYPE_G4,
        .has_dual_bank = true,
        .flash_size_reg = 0x1FFF75E0,              // Section 47.2
        .flash_pagesize = 0x800,                   // 2k (Section 3.3.1)
        // SRAM1 is 80k at 0x20000000
        // SRAM2 is 16k at 0x20014000
        // SRAM3/CCM is 32k at 0x10000000, aliased at 0x20018000
        .sram_size = 0x18000,                      // 128k (Section 2.4)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7000,                    // 28k (Table 2)
        .option_base = STM32_G4_OPTION_BYTES_BASE,
        .option_size = 4,
    },

    /* == STM32L0 == */

    {
        // STM32L0x Category 2
        // RM0367 & RM0377
        .chip_id = STLINK_CHIPID_STM32_L0_CAT2,
        .description = "L0xx Category 2",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff8007c,
        .flash_pagesize = 0x80,
        .sram_size = 0x2000,
        .bootrom_base = 0x1ff0000,
        .bootrom_size = 0x1000,
        .option_base = STM32_L0_CATx_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STM32L0x Category 5
        // RM0367 & RM0377
        .chip_id = STLINK_CHIPID_STM32_L0_CAT5,
        .description = "L0xx Category 5",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff8007c,
        .flash_pagesize = 0x80,
        .sram_size = 0x5000,
        .bootrom_base = 0x1ff0000,
        .bootrom_size = 0x2000,
        .option_base = STM32_L0_CATx_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STM32L0x3
        // RM0367 & RM0377
        .chip_id = STLINK_CHIPID_STM32_L0,
        .description = "L0x3",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff8007c,
        .flash_pagesize = 0x80,
        .sram_size = 0x2000,
        .bootrom_base = 0x1ff0000,
        .bootrom_size = 0x1000,
        .option_base = STM32_L0_CATx_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STM32L011
        // TODO: Add missing reference here!
        .chip_id = STLINK_CHIPID_STM32_L011,
        .description = "L011",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff8007c,
        .flash_pagesize = 0x80,
        .sram_size = 0x2000,
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x2000
    },

    /* == STM32L1 == */

    {
        // TODO: Add missing reference here!
        // This ignores the EEPROM! (and uses the page erase size, not the sector write protection...)
        .chip_id = STLINK_CHIPID_STM32_L1_MEDIUM,
        .description = "L1xx Medium-density",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff8004c,
        .flash_pagesize = 0x100,
        .sram_size = 0x4000,
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x1000
    },
    {
        // TODO: Add missing reference here!
        .chip_id = STLINK_CHIPID_STM32_L1_MEDIUM_PLUS,
        .description = "L1xx Medium-Plus-density",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff800cc,
        .flash_pagesize = 0x100,
        .sram_size = 0x8000, // unclear if there are some device with 48k
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x1000
    },
    {
        // TODO: Add missing reference here!
        .chip_id = STLINK_CHIPID_STM32_L1_HIGH,
        .description = "L1xx High-density",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff800cc,
        .flash_pagesize = 0x100,
        .sram_size = 0xC000, // unclear if there are some devices with 32k
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x1000,
        .option_base = STM32_L1_OPTION_BYTES_BASE,
        .option_size = 8,
    },
    {
        // TODO: Add missing reference here!
        .chip_id = STLINK_CHIPID_STM32_L1_CAT2,
        .description = "L1xx Category 2",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff8004c,
        .flash_pagesize = 0x100,
        .sram_size = 0x8000,
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x1000
    },
    {
        // STM32L152RE
        // TODO: Add missing reference here!
        .chip_id = STLINK_CHIPID_STM32_L152_RE,
        .description = "L152RE",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff800cc,
        .flash_pagesize = 0x100,
        .sram_size = 0x14000, // unclear if there are some devices with 32k
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x1000
    },

    /* == STM32L4 == */

    {
        // STM32L4xx
        // RM0351
        .chip_id = STLINK_CHIPID_STM32_L4,
        .description = "L4xx",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg = 0x1FFF75e0,              // "Flash size data register" (Section 45.2)
        .flash_pagesize = 0x800,                   // 2k (Section 3.2, also appears in section 3.3.1 and tables 4-6)
        // SRAM1 is "up to" 96k in the standard Cortex-M memory map;
        // SRAM2 is 32k mapped at at 0x10000000 (Section 2.3 for sizes; table 2 for SRAM2 location)
        .sram_size = 0x18000,
        .bootrom_base = 0x1fff0000,                // Tables 4-6 (Bank 1 system memory)
        .bootrom_size = 0x7000,                    // 28k (per bank), same source as base
        .option_base = STM32_L4_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // RM0394 Rev. 4 & DS12469 Rev. 5
        .chip_id = STLINK_CHIPID_STM32_L41X,
        .description = "L41x",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg = 0x1fff75e0,              // "Flash size data register" (RM0394, Section 47.2)
        .flash_pagesize = 0x800,                   // 2k (DS12469, Section 3.4)
        // SRAM1 is 32k at 0x20000000
        // SRAM2 is 8k at 0x10000000 and 0x20008000 (DS12469, Section 3.5)
        .sram_size = 0xa000,                       // 40k (DS12469, Section 3.5)
        .bootrom_base = 0x1fff0000,                // System Memory (RM0394, Section 3.3.1, table 8)
        .bootrom_size = 0x7000                     // 28k, same source as base
    },
    {
        // RM0392
        .chip_id = STLINK_CHIPID_STM32_L43X,
        .description = "L43x/L44x",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg = 0x1fff75e0,              // "Flash size data register" (Section 43.2)
        .flash_pagesize = 0x800,                   // 2k (Section 3.2, also appears in section 3.3.1 and tables 7-8)
        // SRAM1 is "up to" 64k in the standard Cortex-M memory map;
        // SRAM2 is 16k mapped at 0x10000000 (Section 2.3 for sizes; table 2 for SRAM2 location)
        .sram_size = 0xc000,
        .bootrom_base = 0x1fff0000,                // Tables 4-6 (Bank 1 system memory)
        .bootrom_size = 0x7000,                    // 28k (per bank), same source as base
        .option_base = STM32_L4_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // RM0394
        .chip_id = STLINK_CHIPID_STM32_L46X,
        .description = "L45x/46x",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg = 0x1fff75e0,             // "Flash size data register" (Section 45.2)
        .flash_pagesize = 0x800,                  // 2k (Section 3.2, also appears in section 3.3.1 and table 7)
        // SRAM1 is 128k at 0x20000000;
        // SRAM2 is 32k mapped at 0x10000000 (Section 2.4.2, table 3-4 and figure 2)
        .sram_size = 0x20000,
        .bootrom_base = 0x1fff0000,               // Tables 6 and figure 2 (Bank 1 system memory)
        .bootrom_size = 0x7000                    // 28k (per bank), same source as base
    },
    {
        // RM0351 Rev. 5
        .chip_id = STLINK_CHIPID_STM32_L496X,
        .description = "L496x/L4A6x",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg = 0x1fff75e0,            // "Flash size data register" (Section 49.2)
        .flash_pagesize = 0x800,                 // Page erase (2 Kbyte) (Section 3.2)
        // SRAM1 is 256k at 0x20000000
        // SRAM2 is 64k at 0x20040000 (Section 2.2.1, Figure 2)
        .sram_size = 0x40000,                  // Embedded SRAM (Section 2.4)
        .bootrom_base = 0x1fff0000,            // System Memory (Bank 1) (Section 3.3.1)
        .bootrom_size = 0x7000,                // 28k (per bank), same source as base
        .option_base = STM32_L4_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STM32L4Rx
        // RM0432
        .chip_id = STLINK_CHIPID_STM32_L4RX,
        .description = "L4Rx",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg = 0x1fff75e0,          // "Flash size data register" (Section 52.2)
        .flash_pagesize = 0x1000,              // 4k (Section 3.3)
        .sram_size = 0xa0000,                  // 192k (SRAM1) + 64k SRAM2 + 384k SRAM3 = 640k, or 0xA0000
        .bootrom_base = 0x1fff0000,            // Section 3.3.1
        .bootrom_size = 0x7000                 // 28k (per bank), same source as base
    },

    /* == STM32L5 == */
  
    {
        // STM32L5x2
        // RM0438
        .chip_id = STLINK_CHIPID_STM32_L5x2,
        .description = "L5x2",
        .flash_type = STLINK_FLASH_TYPE_L5,
        .has_dual_bank = true,
        .flash_size_reg = 0x0BFA05E0,
        .flash_pagesize = 0x800, // 2 banks of 128 x 2K pages
        .sram_size = 0x40000,
        .bootrom_base = 0x0BF90000, // see memory map
        .bootrom_size = 0x8000,
        .option_base = STM32_L5_OPTION_BYTES_BASE,
        .option_size = 4,
    },
  
    /* == STM32H7 == */

    {
        // RM0433
        .chip_id = STLINK_CHIPID_STM32_H74XXX,
        .description = "H743xx",
        .flash_type = STLINK_FLASH_TYPE_H7,
        .flash_size_reg = 0x1FF1E880,          // Section 60.2
        .flash_pagesize = 0x20000,             // No flash pages
        .sram_size = 0x100000,                 // "SRAM" byte size in hex
        .bootrom_base = 0x1FFF0000,            //! "System memory" starting address
        .bootrom_size = 0x1E800                //! @todo "System memory" byte size in hex
    },

    /* == STM32WB == */

    {
        // STM32WB55
        // RM0434
        .chip_id = STLINK_CHIPID_STM32_WB55,
        .description = "WB55",
        .flash_type = STLINK_FLASH_TYPE_WB,
        .flash_size_reg = 0x1FFF75E0,
        .flash_pagesize = 0x1000,             // 4k
        .sram_size = 0x40000,
        .bootrom_base = 0x1fff0000,           // see the memory map
        .bootrom_size = 0x7000
    },
};

const struct stlink_chipid_params *stlink_chipid_get_params(uint32_t chipid) {
    const struct stlink_chipid_params *params = NULL;

    for (size_t n = 0; n < STLINK_ARRAY_SIZE(devices); n++) {
        if (devices[n].chip_id == chipid) {
            params = &devices[n];
            break;
        }
    }

    return(params);
}
