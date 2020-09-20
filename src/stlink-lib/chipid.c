#include <stlink.h>
#include "chipid.h"

static const struct stlink_chipid_params devices[] = {
    {
        // RM0410 document was used to find these paramaters
        .chip_id = STLINK_CHIPID_STM32_F7XXXX,
        .description = "F76xxx",
        .flash_type = STLINK_FLASH_TYPE_F7,
        .flash_size_reg = 0x1ff0f442,               // section 45.2
        .flash_pagesize = 0x800,                    // No flash pages
        .sram_size = 0x80000,                       // "SRAM" byte size in hex from
        .bootrom_base = 0x00200000,                 // ! "System memory" starting address from
        .bootrom_size = 0xEDC0,                      // ! @todo "System memory" byte size in hex from
        .option_base = STM32_F7_OPTION_BYTES_BASE,  // Used for reading back the option bytes, writing uses FLASH_F7_OPTCR and FLASH_F7_OPTCR1
        .option_size = 0x20
    },
    {
        // RM0385 and DS10916 document was used to find these paramaters
        .chip_id = STLINK_CHIPID_STM32_F7,
        .description = "F7xx",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1ff0f442,          // section 41.2
        .flash_pagesize = 0x800,               // No flash pages
        .sram_size = 0x50000,                  // "SRAM" byte size in hex from DS Fig 18
        .bootrom_base = 0x00100000,            // "System memory" starting address from DS Fig 18
        .bootrom_size = 0xEDC0                 // "System memory" byte size in hex from DS Fig 18
    },
    {
        // RM0431 and DS document was used to find these paramaters
        .chip_id = STLINK_CHIPID_STM32_F72XXX,
        .description = "F72x/F73x",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1ff07a22,          // section 35.2
        .flash_pagesize = 0x800,               // No flash pages
        .sram_size = 0x40000,                  // "SRAM" byte size in hex from DS Fig 24
        .bootrom_base = 0x00100000,            // "System memory" starting address from DS Fig 24
        .bootrom_size = 0xEDC0                 // "System memory" byte size in hex from DS Fig 24
    },
    {   // table 2, PM0063
        .chip_id = STLINK_CHIPID_STM32_F1_MEDIUM,
        .description = "F1xx Medium-density",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x400,
        .sram_size = 0x5000,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800
    },
    {   // table 1, PM0059
        .chip_id = STLINK_CHIPID_STM32_F2,
        .description = "F2xx",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1fff7a22,          // as in RM0033 Rev 5
        .flash_pagesize = 0x20000,
        .sram_size = 0x20000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800,
        .option_base = 0x1FFFC000,
        .option_size = 4,
    },
    {   // PM0063
        .chip_id = STLINK_CHIPID_STM32_F1_LOW,
        .description = "F1 Low-density device",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x400,
        .sram_size = 0x2800,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800
    },
    {
        .chip_id = STLINK_CHIPID_STM32_F4,
        .description = "F4xx",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,          // As in rm0090 since Rev 2
        .flash_pagesize = 0x4000,
        .sram_size = 0x30000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800
    },
    {
        .chip_id = STLINK_CHIPID_STM32_F4_DSI,
        .description = "F46x/F47x",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,          // As in rm0090 since Rev 2
        .flash_pagesize = 0x4000,
        .sram_size = 0x40000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800
    },
    {
        .chip_id = STLINK_CHIPID_STM32_F4_HD,
        .description = "F42x/F43x",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,          // As in rm0090 since Rev 2
        .flash_pagesize = 0x4000,
        .sram_size = 0x40000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800
    },
    {
        .chip_id = STLINK_CHIPID_STM32_F4_LP,
        .description = "F4xx (low power)",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x10000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800
    },
    {
        .chip_id = STLINK_CHIPID_STM32_F411RE,
        .description = "stm32f411re",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x20000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800
    },
    {
        .chip_id = STLINK_CHIPID_STM32_F4_DE,
        .description = "F4xx (Dynamic Efficency)",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x18000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800
    },
    {
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
        // This ignores the EEPROM! (and uses the page erase size,
        // not the sector write protection...)
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
        .chip_id = STLINK_CHIPID_STM32_L1_CAT2,
        .description = "L1xx Cat.2",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff8004c,
        .flash_pagesize = 0x100,
        .sram_size = 0x8000,
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x1000
    },
    {
        .chip_id = STLINK_CHIPID_STM32_L1_MEDIUM_PLUS,
        .description = "L1xx Medium-Plus-density",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff800cc,
        .flash_pagesize = 0x100,
        .sram_size = 0x8000, // not completely clear if there are some with 48k
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x1000
    },
    {
        .chip_id = STLINK_CHIPID_STM32_L1_HIGH,
        .description = "L1xx High-density",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff800cc,
        .flash_pagesize = 0x100,
        .sram_size = 0xC000, // not completely clear if there are some with 32k
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x1000,
        .option_base = STM32_L1_OPTION_BYTES_BASE,
        .option_size = 8,
    },
    {
        .chip_id = STLINK_CHIPID_STM32_L152_RE,
        .description = "L152RE",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff800cc,
        .flash_pagesize = 0x100,
        .sram_size = 0x14000, // not completely clear if there are some with 32k
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x1000
    },
    {
        .chip_id = STLINK_CHIPID_STM32_F1_CONN,
        .description = "F1 Connectivity line",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x800,
        .sram_size = 0x10000,
        .bootrom_base = 0x1fffb000,
        .bootrom_size = 0x4800
    },
    {    // Low and Medium density VL have same chipid. RM0041 25.6.1
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
        // STM32F446x family. Support based on DM00135183.pdf (RM0390) document.
        .chip_id = STLINK_CHIPID_STM32_F446,
        .description = "F446",
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
        // STM32F410 MCUs. Support based on DM00180366.pdf (RM0401) document.
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
        // This is STK32F303VCT6 device from STM32 F3 Discovery board.
        // Support based on DM00043574.pdf (RM0316) document.
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
        // This is STK32F373VCT6 device from STM32 F373 eval board
        // Support based on 303 above (37x and 30x have same memory map)
        .chip_id = STLINK_CHIPID_STM32_F37x,
        .description = "F3xx",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,
        .flash_pagesize = 0x800,
        .sram_size = 0xa000,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800
    },
    {
        .chip_id = STLINK_CHIPID_STM32_F1_VL_HIGH,
        .description = "F1xx High-density value line",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x800,
        .sram_size = 0x8000,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800
    },
    {
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
        // Use this as an example for mapping future chips:
        // RM0091 document was used to find these paramaters
        .chip_id = STLINK_CHIPID_STM32_F0_CAN,
        .description = "F07x",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,          // "Flash size data register" (pg735)
        .flash_pagesize = 0x800,               // Page sizes listed in Table 4
        .sram_size = 0x4000,                   // "SRAM" byte size in hex from Table 2
        .bootrom_base = 0x1fffC800,            // "System memory" starting address from Table 2
        .bootrom_size = 0x3000                 // "System memory" byte size in hex from Table 2
    },
    {
        // Use this as an example for mapping future chips:
        // RM0091 document was used to find these paramaters
        .chip_id = STLINK_CHIPID_STM32_F0,
        .description = "F0xx",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,          // "Flash size data register" (pg735)
        .flash_pagesize = 0x400,               // Page sizes listed in Table 4
        .sram_size = 0x2000,                   // "SRAM" byte size in hex from Table 2
        .bootrom_base = 0x1fffec00,            // "System memory" starting address from Table 2
        .bootrom_size = 0xC00                  // "System memory" byte size in hex from Table 2
    },
    {
        // RM0402 document was used to find these parameters
        // Table 4.
        .chip_id = STLINK_CHIPID_STM32_F412,
        .description = "F412",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,          // "Flash size data register" (pg1135)
        .flash_pagesize = 0x4000,              // Table 5. Flash module organization ?
        .sram_size = 0x40000,                  // "SRAM" byte size in hex from Table 4
        .bootrom_base = 0x1FFF0000,            // "System memory" starting address from Table 4
        .bootrom_size = 0x7800                 // "System memory" byte size in hex from Table 4
    },
    {
        // RM0430 DocID029473 Rev 2 document was used to find these parameters
        // Figure 2, Table 4, Table 5, Section 35.2
        .chip_id = STLINK_CHIPID_STM32_F413,
        .description = "F413",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,          // "Flash size data register" Section 35.2
        .flash_pagesize = 0x4000,              // Table 5. Flash module organization (variable sector sizes, but 0x4000 is smallest)
        .sram_size = 0x50000,                  // "SRAM" byte size in hex from Figure 2 (Table 4 only says 0x40000)
        .bootrom_base = 0x1FFF0000,            // "System memory" starting address from Table 4
        .bootrom_size = 0x7800                 // "System memory" byte size in hex from Table 4
    },
    {
        .chip_id = STLINK_CHIPID_STM32_F09X,
        .description = "F09X",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,          // "Flash size data register" (pg735)
        .flash_pagesize = 0x800,               // Page sizes listed in Table 4 (pg 56)
        .sram_size = 0x8000,                   // "SRAM" byte size in hex from Table 2 (pg 50)
        .bootrom_base = 0x1fffd800,            // "System memory" starting address from Table 2
        .bootrom_size = 0x2000                 // "System memory" byte size in hex from Table 2
    },
    {
        // Use this as an example for mapping future chips:
        // RM0091 document was used to find these paramaters
        .chip_id = STLINK_CHIPID_STM32_F04,
        .description = "F04x",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,          // "Flash size data register" (pg735)
        .flash_pagesize = 0x400,               // Page sizes listed in Table 4
        .sram_size = 0x1800,                   // "SRAM" byte size in hex from Table 2
        .bootrom_base = 0x1fffec00,            // "System memory" starting address from Table 2
        .bootrom_size = 0xC00                  // "System memory" byte size in hex from Table 2
    },
    {
        // Use this as an example for mapping future chips:
        // RM0091 document was used to find these paramaters
        .chip_id = STLINK_CHIPID_STM32_F0_SMALL,
        .description = "F0xx small",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,         // "Flash size data register" (pg735)
        .flash_pagesize = 0x400,              // Page sizes listed in Table 4
        .sram_size = 0x1000,                  // "SRAM" byte size in hex from Table 2
        .bootrom_base = 0x1fffec00,           // "System memory" starting address from Table 2
        .bootrom_size = 0xC00                 // "System memory" byte size in hex from Table 2
    },
    {
        // STM32F30x
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
        // STM32L0x
        // RM0367,RM0377 documents was used to find these parameters
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
        // STM32L0x Category 5
        // RM0367,RM0377 documents was used to find these parameters
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
        // STM32L0x Category 2
        // RM0367,RM0377 documents was used to find these parameters
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
        // STM32F334, STM32F303x6/8, and STM32F328
        // From RM0364 and RM0316
        .chip_id = STLINK_CHIPID_STM32_F334,
        .description = "F334 medium density", // (RM0316 sec 33.6.1)
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,
        .flash_pagesize = 0x800,
        .sram_size = 0x3000,
        .bootrom_base = 0x1fffd800,
        .bootrom_size = 0x2000
    },
    {
        // This is STK32F303RET6 device from STM32 F3 Nucelo board.
        // Support based on DM00043574.pdf (RM0316) document rev 5.
        .chip_id = STLINK_CHIPID_STM32_F303_HIGH,
        .description = "F303 high density",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,          // 34.2.1 Flash size data register
        .flash_pagesize = 0x800,               // 4.2.1 Flash memory organization
        .sram_size = 0x10000,                  // 3.3 Embedded SRAM
        .bootrom_base = 0x1fffd800,            // 3.3.2 / Table 4 System Memory
        .bootrom_size = 0x2000
    },
    {
        // STM32L4x6
        // From RM0351.
        .chip_id = STLINK_CHIPID_STM32_L4,
        .description = "L4xx",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg = 0x1FFF75e0,          // "Flash size data register" (sec 45.2, page 1671)
        .flash_pagesize = 0x800,               // 2k (sec 3.2, page 78; also appears in sec 3.3.1
                                               // and tables 4-6 on pages 79-81)
        // SRAM1 is "up to" 96k in the standard Cortex-M memory map;
        // SRAM2 is 32k mapped at at 0x10000000 (sec 2.3, page 73 for
        // sizes; table 2, page 74 for SRAM2 location)
        .sram_size = 0x18000,
        .bootrom_base = 0x1fff0000,            // Tables 4-6, pages 80-81 (Bank 1 system memory)
        .bootrom_size = 0x7000,                // 28k (per bank), same source as base
        .option_base = STM32_L4_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STM32L4RX
        // From DM00310109.pdf
        .chip_id = STLINK_CHIPID_STM32_L4RX,
        .description = "L4Rx",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg = 0x1fff75e0,          // "Flash size data register" (sec 52.2, page 2049)
        .flash_pagesize = 0x1000,              // 4k, section 3.3, pg 97
        .sram_size = 0xa0000,                  // 192k (SRAM1) + 64k SRAM2 + 384k SRAM3 = 640k, or 0xA0000
        .bootrom_base = 0x1fff0000,            // 3.3.1, pg 99
        .bootrom_size = 0x7000                 // 28k (per bank), same source as base (pg 99)
    },
    {
        // STLINK_CHIPID_STM32_L41X
        // From RM0394 Rev 4 and DS12469 Rev 5
        .chip_id = STLINK_CHIPID_STM32_L41X,
        .description = "L41x",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg = 0x1fff75e0,          // "Flash size data register" (RM0394, sec 47.2, page 1586)
        .flash_pagesize = 0x800,               // 2k (DS12469, sec 3.4, page 17)
        // SRAM1 is 32k at 0x20000000
        // SRAM2 is 8k at 0x10000000 and 0x20008000 (DS12469, sec 3.5, page 18)
        .sram_size = 0xa000,                   // 40k (DS12469, sec 3.5, page 18)
        .bootrom_base = 0x1fff0000,            // System Memory (RM0394, sec 3.3.1, table 8)
        .bootrom_size = 0x7000                 // 28k, same source as base
    },
    {
        // STLINK_CHIPID_STM32_L43X
        // From RM0392.
        .chip_id = STLINK_CHIPID_STM32_L43X,
        .description = "L43x/L44x",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg = 0x1fff75e0,          // "Flash size data register" (sec 43.2, page 1410)
        .flash_pagesize = 0x800,               // 2k (sec 3.2, page 74; also appears in sec 3.3.1
                                               // and tables 7-8 on pages 75-76)
        // SRAM1 is "up to" 64k in the standard Cortex-M memory map;
        // SRAM2 is 16k mapped at 0x10000000 (sec 2.3, page 73 for
        // sizes; table 2, page 74 for SRAM2 location)
        .sram_size = 0xc000,
        .bootrom_base = 0x1fff0000,            // Tables 4-6, pages 80-81 (Bank 1 system memory)
        .bootrom_size = 0x7000,                // 28k (per bank), same source as base
        .option_base = STM32_L4_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STLINK_CHIPID_STM32_L496X
        // Support based on en.DM00083560.pdf (RM0351) document rev 5.
        .chip_id = STLINK_CHIPID_STM32_L496X,
        .description = "L496x/L4A6x",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg = 0x1fff75e0,          // "Flash size data register" (sec 49.2, page 1809)
        .flash_pagesize = 0x800,               // Page erase (2 Kbyte) (sec 3.2, page 93)
        // SRAM1 is 256k at 0x20000000
        // SRAM2 is 64k at 0x20040000 (sec 2.2.1, fig 2, page 74)
        .sram_size = 0x40000,                  // Embedded SRAM (sec 2.4, page 84)
        .bootrom_base = 0x1fff0000,            // System Memory (Bank 1) (sec 3.3.1)
        .bootrom_size = 0x7000,                // 28k (per bank), same source as base
        .option_base = STM32_L4_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STLINK_CHIPID_STM32_L46X
        // From RM0394 (updated version of RM0392?).
        .chip_id = STLINK_CHIPID_STM32_L46X,
        .description = "L45x/46x",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg = 0x1fff75e0,          // "Flash size data register" (sec 45.2, page 1463)
        .flash_pagesize = 0x800,               // 2k (sec 3.2, page 73; also appears in sec 3.3.1
                                               // and tables 7 on pages 73-74)
        // SRAM1 is 128k at 0x20000000;
        // SRAM2 is 32k mapped at 0x10000000 (sec 2.4.2, table 3-4, page 68, also fig 2 on page 63)
        .sram_size = 0x20000,
        .bootrom_base = 0x1fff0000,            // Tables 6, pages 71-72 (Bank 1 system memory, also fig 2 on page 63)
        .bootrom_size = 0x7000                 // 28k (per bank), same source as base
    },
    {
        // STM32L011
        .chip_id = STLINK_CHIPID_STM32_L011,
        .description = "L011",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff8007c,
        .flash_pagesize = 0x80,
        .sram_size = 0x2000,
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x2000
    },
    {
        // STM32G030/031/041 (from RM0454 & RM0444)
        .chip_id = STLINK_CHIPID_STM32_G0_CAT1,
        .description = "G030/G031/G041",
        .flash_type = STLINK_FLASH_TYPE_G0,
        .flash_size_reg = 0x1FFF75E0,          // Section 38.2
        .flash_pagesize = 0x800,               // 2k (sec 3.2)
        .sram_size = 0x2000,                   // 8k (sec 2.3)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x2000,                // 8k (sec 2.2.2 table 3)
        .option_base = STM32_G0_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STM32G071/081 (from RM0444)
        .chip_id = STLINK_CHIPID_STM32_G0_CAT2,
        .description = "G070/G071/G081",
        .flash_type = STLINK_FLASH_TYPE_G0,
        .flash_size_reg = 0x1FFF75E0,          // Section 38.2
        .flash_pagesize = 0x800,               // 2k (sec 3.2)
        .sram_size = 0x9000,                   // 36k (sec 2.3)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7000,                // 28k (sec 2.2.2 table 2)
        .option_base = STM32_G0_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STM32G431/441 (from RM0440)
        .chip_id = STLINK_CHIPID_STM32_G4_CAT2,
        .description = "G4 Category-2",
        .flash_type = STLINK_FLASH_TYPE_G4,
        .flash_size_reg = 0x1FFF75E0,          // Section 47.2
        .flash_pagesize = 0x800,               // 2k (sec 3.3.1)
        // SRAM1 is 16k at 0x20000000
        // SRAM2 is 6k at 0x20014000
        // SRAM3/CCM is 10k at 0x10000000, aliased at 0x20018000
        .sram_size = 0x8000,                   // 32k (sec 2.4)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7000,                // 28k (table 2)
        .option_base = STM32_G4_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STM32G471/473/474/483/484 (from RM0440)
        .chip_id = STLINK_CHIPID_STM32_G4_CAT3,
        .description = "G4 Category-3",
        .flash_type = STLINK_FLASH_TYPE_G4,
        .has_dual_bank = true,
        .flash_size_reg = 0x1FFF75E0,          // Section 47.2
        .flash_pagesize = 0x800,               // 2k (sec 3.3.1)
        // SRAM1 is 80k at 0x20000000
        // SRAM2 is 16k at 0x20014000
        // SRAM3/CCM is 32k at 0x10000000, aliased at 0x20018000
        .sram_size = 0x18000,                  // 128k (sec 2.4)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7000,                // 28k (table 2)
        .option_base = STM32_G4_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STM32WB55 (from RM0434)
        .chip_id = STLINK_CHIPID_STM32_WB55,
        .description = "WB55",
        .flash_type = STLINK_FLASH_TYPE_WB,
        .flash_size_reg = 0x1FFF75E0,
        .flash_pagesize = 0x1000,             // 4k
        .sram_size = 0x40000,
        .bootrom_base = 0x1fff0000, // see the memory map
        .bootrom_size = 0x7000
    },
    {
        // unknown
        .chip_id = STLINK_CHIPID_UNKNOWN,
        .description = "unknown device",
        .flash_type = STLINK_FLASH_TYPE_UNKNOWN,
        .flash_size_reg = 0x0,
        .flash_pagesize = 0x0,
        .sram_size = 0x0,
        .bootrom_base = 0x0,
        .bootrom_size = 0x0
    },
};

const struct stlink_chipid_params *stlink_chipid_get_params(uint32_t chipid) {
    const struct stlink_chipid_params *params = NULL;

    for (size_t n = 0; n < STLINK_ARRAY_SIZE(devices); n++)
        if (devices[n].chip_id == chipid) {
            params = &devices[n];
            break;
        }

    return(params);
}
