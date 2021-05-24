#include <stlink.h>
#include "chipid.h"

static const struct stlink_chipid_params devices[] = {
    {
        // STM32F76x/F77x
        // RM0410
        .chip_id = STLINK_CHIPID_STM32_F7XXXX,
        .description = "F76x/F77x",
        .flash_type = STLINK_FLASH_TYPE_F7,
        .flash_size_reg = 0x1ff0f442, // section 45.2
        .flash_pagesize = 0x800,      // No flash pages
        .sram_size = 0x80000,         // "SRAM" byte size in hex from
        .bootrom_base = 0x00200000,   // "System memory" starting address from
        .bootrom_size = 0xEDC0,
        .option_base =
            STM32_F7_OPTION_BYTES_BASE, // Used for reading back the option
                                        // bytes, writing uses FLASH_F7_OPTCR
                                        // and FLASH_F7_OPTCR1
        .option_size = 0x20,
        .flags = CHIP_F_HAS_DUAL_BANK | CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F74x/F75x
        // RM0385, DS10916
        .chip_id = STLINK_CHIPID_STM32_F7,
        .description = "F74x/F75x",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1ff0f442, // section 41.2
        .flash_pagesize = 0x800,      // No flash pages
        .sram_size = 0x50000,         // "SRAM" byte size in hex from DS Fig 18
        .bootrom_base =
            0x00100000, // "System memory" starting address from DS Fig 18
        .bootrom_size =
            0xEDC0, // "System memory" byte size in hex from DS Fig 18
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F72x/F73x
        // RM0431
        .chip_id = STLINK_CHIPID_STM32_F72XXX,
        .description = "F72x/F73x",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1ff07a22, // section 35.2
        .flash_pagesize = 0x800,      // No flash pages
        .sram_size = 0x40000,         // "SRAM" byte size in hex from DS Fig 24
        .bootrom_base =
            0x00100000, // "System memory" starting address from DS Fig 24
        .bootrom_size =
            0xEDC0, // "System memory" byte size in hex from DS Fig 24
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F1xx medium-density devices
        // RM0008
        .chip_id = STLINK_CHIPID_STM32_F1_MEDIUM,
        .description = "F1xx MD",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x400,
        .sram_size = 0x5000,
        .bootrom_base = 0x1ffff000, // 2.3.3 "Embedded Flash memory"
        .bootrom_size = 0x800,
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F205xx, STM32F207xx, STM32F215xx, STM32F217xx
        // RM0033 (rev 5)
        .chip_id = STLINK_CHIPID_STM32_F2,
        .description = "F2xx",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1fff7a22,
        .flash_pagesize = 0x20000,
        .sram_size = 0x20000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800,
        .option_base = 0x1FFFC000,
        .option_size = 4,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F1xx low-density devices
        // RM0008
        .chip_id = STLINK_CHIPID_STM32_F1_LOW,
        .description = "F1xx LD",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x400,
        .sram_size = 0x2800,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800,
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F4x5/F4x7
        // RM0090 (rev 2)
        .chip_id = STLINK_CHIPID_STM32_F4,
        .description = "F4x5/F4x7",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x30000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800,
        .option_base = STM32_F4_OPTION_BYTES_BASE,
        .option_size = 4,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F46x/F47x
        // RM0090 (rev 2)
        .chip_id = STLINK_CHIPID_STM32_F4_DSI,
        .description = "F46x/F47x",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x40000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F42x/F43x
        // RM0090 (rev 2)
        .chip_id = STLINK_CHIPID_STM32_F4_HD,
        .description = "F42x/F43x",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x40000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        .chip_id = STLINK_CHIPID_STM32_F4_LP,
        .description = "F401xB/C",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x10000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        .chip_id = STLINK_CHIPID_STM32_F411XX,
        .description = "F411xC/E",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x20000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        .chip_id = STLINK_CHIPID_STM32_F4_DE,
        .description = "F401xD/E",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22,
        .flash_pagesize = 0x4000,
        .sram_size = 0x18000,
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7800,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F1xx high-density devices
        // RM0008
        .chip_id = STLINK_CHIPID_STM32_F1_HIGH,
        .description = "F1xx HD",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x800,
        .sram_size = 0x10000,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800,
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32L100/L15x/L16x Cat.1
        // RM0038
        .chip_id = STLINK_CHIPID_STM32_L1_MEDIUM,
        .description = "L1xx Cat.1",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff8004c,
        .flash_pagesize = 0x100,
        .sram_size = 0x4000, // up to 16k
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x1000,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32L100/L15x/L16x Cat.2
        // RM0038
        .chip_id = STLINK_CHIPID_STM32_L1_CAT2,
        .description = "L1xx Cat.2",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff8004c,
        .flash_pagesize = 0x100,
        .sram_size = 0x8000, // up to 32k
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x1000,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32L100/L15x/L16x Cat.3
        // RM0038
        .chip_id = STLINK_CHIPID_STM32_L1_MEDIUM_PLUS,
        .description = "L1xx Cat.3",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff800cc,
        .flash_pagesize = 0x100,
        .sram_size = 0x8000, // up to 32k
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x1000,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32L100/L15x/L16x Cat.4
        // RM0038
        .chip_id = STLINK_CHIPID_STM32_L1_HIGH,
        .description = "L1xx Cat.4",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff800cc,
        .flash_pagesize = 0x100,
        .sram_size = 0xC000, // up to 48k
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x1000,
        .option_base = STM32_L1_OPTION_BYTES_BASE,
        .option_size = 8,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32L100/L15x/L16x Cat.5
        // RM0038
        .chip_id = STLINK_CHIPID_STM32_L152_RE,
        .description = "L1xx Cat.5",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff800cc,
        .flash_pagesize = 0x100,
        .sram_size = 0x14000, // up to 80k
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x1000,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F1xx connectivity devices
        // RM0008
        .chip_id = STLINK_CHIPID_STM32_F1_CONN,
        .description = "F1xx CL",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x800,
        .sram_size = 0x10000,
        .bootrom_base = 0x1fffb000,
        .bootrom_size = 0x4800,
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F1xx low- and medium-density value line devices
        // RM0041
        .chip_id = STLINK_CHIPID_STM32_F1_VL_MEDIUM_LOW,
        .description = "F1xx VL",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x400,
        .sram_size = 0x2000, // 0x1000 for low density devices
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800,
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F446x family
        // RM0390
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
        .flags = CHIP_F_HAS_SWO_TRACING,
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
        .bootrom_size = 0x7800,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F303xB/C, STM32F358, STM32F302xBxC 
        // RM0316, RM0365
        .chip_id = STLINK_CHIPID_STM32_F3,
        .description = "F302/F303/F358",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,
        .flash_pagesize = 0x800,
        .sram_size = 0xa000,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800,
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F373Cx/Rx/Vx, STM32F378Cx/Rx/Vx
        // RM0313
        .chip_id = STLINK_CHIPID_STM32_F37x,
        .description = "F37x",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,
        .flash_pagesize = 0x800,
        .sram_size = 0xa000,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800,
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F1xx high-density  value line devices
        // RM0041
        .chip_id = STLINK_CHIPID_STM32_F1_VL_HIGH,
        .description = "F1xx HD VL",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x800,
        .sram_size = 0x8000,
        .bootrom_base = 0x1ffff000,
        .bootrom_size = 0x800,
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F1xx XL-density devices
        // RM0008
        .chip_id = STLINK_CHIPID_STM32_F1_XL,
        .description = "F1xx XL",
        .flash_type = STLINK_FLASH_TYPE_F1_XL,
        .flash_size_reg = 0x1ffff7e0,
        .flash_pagesize = 0x800,
        .sram_size = 0x18000,
        .bootrom_base = 0x1fffe000,
        .bootrom_size = 0x1800,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F07x
        // RM0091
        .chip_id = STLINK_CHIPID_STM32_F0_CAN,
        .description = "F07x",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc, // "Flash size data register" (pg735)
        .flash_pagesize = 0x800,      // Page sizes listed in Table 4
        .sram_size = 0x4000,          // "SRAM" byte size in hex from Table 2
        .bootrom_base =
            0x1fffC800,         // "System memory" starting address from Table 2
        .bootrom_size = 0x3000, // "System memory" byte size in hex from Table 2
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
    },
    {
        // STM32F05x
        // RM0091
        .chip_id = STLINK_CHIPID_STM32_F0,
        .description = "F05x",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc, // "Flash size data register" (pg735)
        .flash_pagesize = 0x400,      // Page sizes listed in Table 4
        .sram_size = 0x2000,          // "SRAM" byte size in hex from Table 2
        .bootrom_base =
            0x1fffec00,        // "System memory" starting address from Table 2
        .bootrom_size = 0xC00, // "System memory" byte size in hex from Table 2
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
    },
    {
        // STM32F412
        // RM0402
        .chip_id = STLINK_CHIPID_STM32_F412,
        .description = "F412",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22, // "Flash size data register" (pg1135)
        .flash_pagesize = 0x4000,     // Table 5. Flash module organization ?
        .sram_size = 0x40000,         // "SRAM" byte size in hex from Table 4
        .bootrom_base =
            0x1FFF0000,         // "System memory" starting address from Table 4
        .bootrom_size = 0x7800, // "System memory" byte size in hex from Table 4
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F413/F423
        // RM0430 (rev 2)
        .chip_id = STLINK_CHIPID_STM32_F413,
        .description = "F413/F423",
        .flash_type = STLINK_FLASH_TYPE_F4,
        .flash_size_reg = 0x1FFF7A22, // "Flash size data register" Section 35.2
        .flash_pagesize =
            0x4000, // Table 5. Flash module organization (variable sector
                    // sizes, but 0x4000 is smallest)
        .sram_size = 0x50000, // "SRAM" byte size in hex from Figure 2 (Table 4
                              // only says 0x40000)
        .bootrom_base =
            0x1FFF0000,         // "System memory" starting address from Table 4
        .bootrom_size = 0x7800, // "System memory" byte size in hex from Table 4
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F09x
        // RM0091
        .chip_id = STLINK_CHIPID_STM32_F09X,
        .description = "F09x",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc, // "Flash size data register" (pg735)
        .flash_pagesize = 0x800,      // Page sizes listed in Table 4 (pg 56)
        .sram_size = 0x8000, // "SRAM" byte size in hex from Table 2 (pg 50)
        .bootrom_base =
            0x1fffd800,         // "System memory" starting address from Table 2
        .bootrom_size = 0x2000, // "System memory" byte size in hex from Table 2
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
    },
    {
        // STM32F04x
        // RM0091
        .chip_id = STLINK_CHIPID_STM32_F04,
        .description = "F04x",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc, // "Flash size data register" (pg735)
        .flash_pagesize = 0x400,      // Page sizes listed in Table 4
        .sram_size = 0x1800,          // "SRAM" byte size in hex from Table 2
        .bootrom_base =
            0x1fffec00,        // "System memory" starting address from Table 2
        .bootrom_size = 0xC00, // "System memory" byte size in hex from Table 2
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
    },
    {
        // STM32F03x
        // RM0091
        .chip_id = STLINK_CHIPID_STM32_F0_SMALL,
        .description = "F03x",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc, // "Flash size data register" (pg735)
        .flash_pagesize = 0x400,      // Page sizes listed in Table 4
        .sram_size = 0x1000,          // "SRAM" byte size in hex from Table 2
        .bootrom_base =
            0x1fffec00,        // "System memory" starting address from Table 2
        .bootrom_size = 0xC00, // "System memory" byte size in hex from Table 2
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
    },
    {
        // STM32F301x6/8, STM32F318x8, STM32F302x6x8
        // RM0366, RM0365
        .chip_id = STLINK_CHIPID_STM32_F3_SMALL,
        .description = "F301/F302/F318",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,
        .flash_pagesize = 0x800,
        .sram_size = 0xa000,
        .bootrom_base = 0x1fffd800,
        .bootrom_size = 0x2000,
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32L0xx Category 3
        // RM0367, RM0377, RM0451
        .chip_id = STLINK_CHIPID_STM32_L0,
        .description = "L0xx Cat.3",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff8007c,
        .flash_pagesize = 0x80,
        .sram_size = 0x2000,
        .bootrom_base = 0x1ff0000,
        .bootrom_size = 0x1000,
        .option_base = STM32_L0_OPTION_BYTES_BASE,
        .option_size = 20,
    },
    {
        // STM32L0x Category 5
        // RM0367, RM0377, RM0451
        .chip_id = STLINK_CHIPID_STM32_L0_CAT5,
        .description = "L0xx Cat.5",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff8007c,
        .flash_pagesize = 0x80,
        .sram_size = 0x5000,
        .bootrom_base = 0x1ff0000,
        .bootrom_size = 0x2000,
        .option_base = STM32_L0_OPTION_BYTES_BASE,
        .option_size = 20,
        .flags = CHIP_F_HAS_DUAL_BANK,
    },
    {
        // STM32L0x Category 2
        // RM0367, RM0377
        .chip_id = STLINK_CHIPID_STM32_L0_CAT2,
        .description = "L0xx Cat.2",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff8007c,
        .flash_pagesize = 0x80,
        .sram_size = 0x2000,
        .bootrom_base = 0x1ff0000,
        .bootrom_size = 0x1000,
        .option_base = STM32_L0_OPTION_BYTES_BASE,
        .option_size = 20,
    },
    {
        // STM32F334, STM32F303x6/8, STM32F328
        // RM0364, RM0316
        .chip_id = STLINK_CHIPID_STM32_F334,
        .description = "F303/F328/F334", // (RM0316 sec 33.6.1)
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc,
        .flash_pagesize = 0x800,
        .sram_size = 0x3000,
        .bootrom_base = 0x1fffd800,
        .bootrom_size = 0x2000,
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32F303xD/E, STM32F398xE, STM32F302xD/E
        // RM0316 (rev 5), RM0365
        .chip_id = STLINK_CHIPID_STM32_F303_HIGH,
        .description = "F302/F303/F398",
        .flash_type = STLINK_FLASH_TYPE_F0,
        .flash_size_reg = 0x1ffff7cc, // 34.2.1 Flash size data register
        .flash_pagesize = 0x800,      // 4.2.1 Flash memory organization
        .sram_size = 0x10000,         // 3.3 Embedded SRAM
        .bootrom_base = 0x1fffd800,   // 3.3.2 / Table 4 System Memory
        .bootrom_size = 0x2000,
        .option_base = STM32_F0_OPTION_BYTES_BASE,
        .option_size = 16,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32L47x/L48x
        // RM0351
        .chip_id = STLINK_CHIPID_STM32_L4,
        .description = "L47x/L48x",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg =
            0x1FFF75e0, // "Flash size data register" (sec 45.2, page 1671)
        .flash_pagesize =
            0x800, // 2k (sec 3.2, page 78; also appears in sec 3.3.1
                   // and tables 4-6 on pages 79-81)
                   // SRAM1 is "up to" 96k in the standard Cortex-M memory map;
                   // SRAM2 is 32k mapped at at 0x10000000 (sec 2.3, page 73 for
                   // sizes; table 2, page 74 for SRAM2 location)
        .sram_size = 0x18000,
        .bootrom_base =
            0x1fff0000, // Tables 4-6, pages 80-81 (Bank 1 system memory)
        .bootrom_size = 0x7000, // 28k (per bank), same source as base
        .option_base = STM32_L4_OPTION_BYTES_BASE,
        .option_size = 4,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32L4RX
        // RM0432
        .chip_id = STLINK_CHIPID_STM32_L4RX,
        .description = "L4Rx/L4Sx",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg =
            0x1fff75e0, // "Flash size data register" (sec 58.2, page 2274)
        .flash_pagesize = 0x1000, // 4k, section 3.3, pg 117-120
        //TODO: flash_pagesize can be 8k depend on the dual-bank mode and flash size
        .sram_size =
            0xa0000, // 192k (SRAM1) + 64k SRAM2 + 384k SRAM3 = 640k, or 0xA0000
        .bootrom_base = 0x1fff0000, // 3.3.1, pg 117
        .bootrom_size = 0x7000, // 28k (per bank), same source as base (pg 117)
        .flags = CHIP_F_HAS_DUAL_BANK | CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32L4PX
        // RM0432
        .chip_id = STLINK_CHIPID_STM32_L4PX,
        .description = "L4Px",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg =
            0x1fff75e0, // "Flash size data register" (sec 58.2, page 2274)
        .flash_pagesize = 0x1000, // 4k, section 3.3, pg 117-120
        //TODO: flash_pagesize can be 8k depend on the dual-bank mode and flash size
        .sram_size =
            0xa0000, // 192k (SRAM1) + 64k SRAM2 + 384k SRAM3 = 640k, or 0xA0000
        .bootrom_base = 0x1fff0000, // 3.3.1, pg 117
        .bootrom_size = 0x7000, // 28k (per bank), same source as base (pg 117)
        .flags = CHIP_F_HAS_DUAL_BANK | CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STLINK_CHIPID_STM32_L41X
        // RM0394 (rev 4), DS12469 (rev 5)
        .chip_id = STLINK_CHIPID_STM32_L41X,
        .description = "L41x/L42x",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg = 0x1fff75e0, // "Flash size data register" (RM0394,
                                      // sec 47.2, page 1586)
        .flash_pagesize = 0x800,      // 2k (DS12469, sec 3.4, page 17)
                                      // SRAM1 is 32k at 0x20000000
                                 // SRAM2 is 8k at 0x10000000 and 0x20008000
                                 // (DS12469, sec 3.5, page 18)
        .sram_size = 0xa000, // 40k (DS12469, sec 3.5, page 18)
        .bootrom_base =
            0x1fff0000,         // System Memory (RM0394, sec 3.3.1, table 8)
        .bootrom_size = 0x7000, // 28k, same source as base
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STLINK_CHIPID_STM32_L43X
        // RM0392
        .chip_id = STLINK_CHIPID_STM32_L43X,
        .description = "L43x/L44x",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg =
            0x1fff75e0, // "Flash size data register" (sec 43.2, page 1410)
        .flash_pagesize =
            0x800, // 2k (sec 3.2, page 74; also appears in sec 3.3.1
                   // and tables 7-8 on pages 75-76)
                   // SRAM1 is "up to" 64k in the standard Cortex-M memory map;
                   // SRAM2 is 16k mapped at 0x10000000 (sec 2.3, page 73 for
                   // sizes; table 2, page 74 for SRAM2 location)
        .sram_size = 0xc000,
        .bootrom_base =
            0x1fff0000, // Tables 4-6, pages 80-81 (Bank 1 system memory)
        .bootrom_size = 0x7000, // 28k (per bank), same source as base
        .option_base = STM32_L4_OPTION_BYTES_BASE,
        .option_size = 4,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STLINK_CHIPID_STM32_L496X
        // RM0351 (rev 5)
        .chip_id = STLINK_CHIPID_STM32_L496X,
        .description = "L49x/L4Ax",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg =
            0x1fff75e0, // "Flash size data register" (sec 49.2, page 1809)
        .flash_pagesize =
            0x800, // Page erase (2 Kbyte) (sec 3.2, page 93)
                   // SRAM1 is 256k at 0x20000000
                   // SRAM2 is 64k at 0x20040000 (sec 2.2.1, fig 2, page 74)
        .sram_size = 0x40000,       // Embedded SRAM (sec 2.4, page 84)
        .bootrom_base = 0x1fff0000, // System Memory (Bank 1) (sec 3.3.1)
        .bootrom_size = 0x7000,     // 28k (per bank), same source as base
        .option_base = STM32_L4_OPTION_BYTES_BASE,
        .option_size = 4,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STLINK_CHIPID_STM32_L46X
        // RM0394 (updated version of RM0392?)
        .chip_id = STLINK_CHIPID_STM32_L46X,
        .description = "L45x/46x",
        .flash_type = STLINK_FLASH_TYPE_L4,
        .flash_size_reg =
            0x1fff75e0, // "Flash size data register" (sec 45.2, page 1463)
        .flash_pagesize =
            0x800, // 2k (sec 3.2, page 73; also appears in sec 3.3.1
                   // and tables 7 on pages 73-74)
                   // SRAM1 is 128k at 0x20000000;
                   // SRAM2 is 32k mapped at 0x10000000 (sec 2.4.2, table 3-4,
                   // page 68, also fig 2 on page 63)
        .sram_size = 0x20000,
        .bootrom_base = 0x1fff0000, // Tables 6, pages 71-72 (Bank 1 system
                                    // memory, also fig 2 on page 63)
        .bootrom_size = 0x7000,     // 28k (per bank), same source as base
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32L0xx Category 1
        // RM0451, RM0377
        .chip_id = STLINK_CHIPID_STM32_L011,
        .description = "L01x/L02x",
        .flash_type = STLINK_FLASH_TYPE_L0,
        .flash_size_reg = 0x1ff8007c,
        .flash_pagesize = 0x80,
        .sram_size = 0x2000,
        .bootrom_base = 0x1ff00000,
        .bootrom_size = 0x2000,
    },
    {
        // STM32G030/031/041
        // RM0454, RM0444
        .chip_id = STLINK_CHIPID_STM32_G0_CAT1,
        .description = "G03x/G04x",
        .flash_type = STLINK_FLASH_TYPE_G0,
        .flash_size_reg = 0x1FFF75E0, // Section 38.2
        .flash_pagesize = 0x800,      // 2k (sec 3.2)
        .sram_size = 0x2000,          // 8k (sec 2.3)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x2000,       // 8k (sec 2.2.2 table 3)
        .option_base = STM32_G0_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STM32G071/081
        // RM0444
        .chip_id = STLINK_CHIPID_STM32_G0_CAT2,
        .description = "G07x/G08x",
        .flash_type = STLINK_FLASH_TYPE_G0,
        .flash_size_reg = 0x1FFF75E0, // Section 38.2
        .flash_pagesize = 0x800,      // 2k (sec 3.2)
        .sram_size = 0x9000,          // 36k (sec 2.3)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7000,       // 28k (sec 2.2.2 table 2)
        .option_base = STM32_G0_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STM32G0B1/G0C1
        // RM0444
        .chip_id = STLINK_CHIPID_STM32_G0_CAT3,
        .description = "G0Bx/G0Cx",
        .flash_type = STLINK_FLASH_TYPE_G0,
        .flash_size_reg = 0x1FFF75E0, // Section 38.2
        .flash_pagesize = 0x800,      // 2k (sec 3.2)
        .sram_size = 0x9000,          // 36k (sec 2.3)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7000,       // 28k (sec 2.2.2 table 2)
        .option_base = STM32_G0_OPTION_BYTES_BASE,
        .option_size = 4,
        .flags = CHIP_F_HAS_DUAL_BANK,
    },
    {
        // STM32G051/G061
        // RM0444
        .chip_id = STLINK_CHIPID_STM32_G0_CAT4,
        .description = "G05x/G06x",
        .flash_type = STLINK_FLASH_TYPE_G0,
        .flash_size_reg = 0x1FFF75E0, // Section 38.2
        .flash_pagesize = 0x800,      // 2k (sec 3.2)
        .sram_size = 0x9000,          // 36k (sec 2.3)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7000,       // 28k (sec 2.2.2 table 2)
        .option_base = STM32_G0_OPTION_BYTES_BASE,
        .option_size = 4,
    },
    {
        // STM32G431/441
        // RM0440
        .chip_id = STLINK_CHIPID_STM32_G4_CAT2,
        .description = "G43x/G44x",
        .flash_type = STLINK_FLASH_TYPE_G4,
        .flash_size_reg = 0x1FFF75E0, // Section 47.2
        .flash_pagesize =
            0x800, // 2k (sec 3.3.1)
                   // SRAM1 is 16k at 0x20000000
                   // SRAM2 is 6k at 0x20014000
                   // SRAM3/CCM is 10k at 0x10000000, aliased at 0x20018000
        .sram_size = 0x8000, // 32k (sec 2.4)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7000, // 28k (table 2)
        .option_base = STM32_G4_OPTION_BYTES_BASE,
        .option_size = 4,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32G471/473/474/483/484
        // RM0440
        .chip_id = STLINK_CHIPID_STM32_G4_CAT3,
        .description = "G47x/G48x",
        .flash_type = STLINK_FLASH_TYPE_G4,
        .flash_size_reg = 0x1FFF75E0, // Section 47.2
        .flash_pagesize =
            0x800, // 2k (sec 3.3.1)
                   // SRAM1 is 80k at 0x20000000
                   // SRAM2 is 16k at 0x20014000
                   // SRAM3/CCM is 32k at 0x10000000, aliased at 0x20018000
        .sram_size = 0x20000, // 128k (sec 2.4)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7000, // 28k (table 2)
        .option_base = STM32_G4_OPTION_BYTES_BASE,
        .option_size = 4,
        .flags = CHIP_F_HAS_DUAL_BANK | CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32G491/G4A1
        // RM0440
        .chip_id = STLINK_CHIPID_STM32_G4_CAT4,
        .description = "G49x/G4Ax",
        .flash_type = STLINK_FLASH_TYPE_G4,
        .flash_size_reg = 0x1FFF75E0, // Section 47.2
        .flash_pagesize =
            0x800, // 2k (sec 3.3.1)
                   // SRAM1 is 80k at 0x20000000
                   // SRAM2 is 16k at 0x20014000
                   // SRAM3/CCM is 32k at 0x10000000, aliased at 0x20018000
        .sram_size = 0x1C000, // 112k (sec 2.4)
        .bootrom_base = 0x1fff0000,
        .bootrom_size = 0x7000, // 28k (table 2)
        .option_base = STM32_G4_OPTION_BYTES_BASE,
        .option_size = 4,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32WB55xx, STM32WB35xx, STM32WB50CG/30CE
        // RM0434, RM0471
        .chip_id = STLINK_CHIPID_STM32_WB55,
        .description = "WB5x/3x",
        .flash_type = STLINK_FLASH_TYPE_WB,
        .flash_size_reg = 0x1FFF75E0,
        .flash_pagesize = 0x1000, // 4k
        .sram_size = 0x40000,
        .bootrom_base = 0x1fff0000, // see the memory map
        .bootrom_size = 0x7000,
        .flags = CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32H742/743/753 (from RM0433)
        .chip_id = STLINK_CHIPID_STM32_H74XXX,
        .description = "H74x/H75x",
        .flash_type = STLINK_FLASH_TYPE_H7,
        .flash_size_reg = 0x1ff1e880, // "Flash size register" (pg3272)
        .flash_pagesize = 0x20000,    // 128k sector (pg147)
        .sram_size = 0x20000,         // 128k "DTCM" from Table 7
        .bootrom_base =
            0x1ff00000, // "System memory" starting address from Table 7
        .bootrom_size =
            0x20000, // "System memory" byte size in hex from Table 7
        .option_base = STM32_H7_OPTION_BYTES_BASE,
        .option_size = 44, // FLASH_OPTSR_CUR to FLASH_BOOT_PRGR from Table 28
        .flags = CHIP_F_HAS_DUAL_BANK | CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32H7A3/7B3
        // RM0455
        .chip_id = STLINK_CHIPID_STM32_H7AX,
        .description = "H7Ax/H7Bx",
        .flash_type = STLINK_FLASH_TYPE_H7,
        .flash_size_reg = 0x08FFF80C, // "Flash size register" (p.2949)
        .flash_pagesize = 0x2000,     // 8k sector (p.146)
        .sram_size = 0x20000,         // 128k "DTCM" (Figure 1)
        .bootrom_base =
            0x1FF00000, // "System memory" starting address (Table 12-14)
        .bootrom_size = 0x20000, // "System memory" byte size in hex splitted to
                                 // two banks (Table 12-14)
        .option_base = STM32_H7_OPTION_BYTES_BASE,
        .option_size = 44,
        .flags = CHIP_F_HAS_DUAL_BANK | CHIP_F_HAS_SWO_TRACING,
    },
    {
        // STM32H72x/H73x
        // RM0468
        .chip_id = STLINK_CHIPID_STM32_H72X,
        .description = "H72x/H73x",
        .flash_type = STLINK_FLASH_TYPE_H7,
        .flash_size_reg = 0x1FF1E880, // "Flash size register" (p.3286)
        .flash_pagesize = 0x20000,    // 128k sector (p.152)
        .sram_size = 0x20000,         // 128k "DTCM" (Figure 1)
        .bootrom_base =
            0x1FF00000,          // "System memory" starting address (Table 6)
        .bootrom_size = 0x20000, // "System memory" byte size in hex (Table 6)
        .option_base = STM32_H7_OPTION_BYTES_BASE,
        .option_size = 44,
        .flags = CHIP_F_HAS_SWO_TRACING,
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
        .bootrom_size = 0x0,
    },
};

const struct stlink_chipid_params *stlink_chipid_get_params(uint32_t chipid) {
  const struct stlink_chipid_params *params = NULL;

  for (size_t n = 0; n < STLINK_ARRAY_SIZE(devices); n++)
    if (devices[n].chip_id == chipid) {
      params = &devices[n];
      break;
    }

  return (params);
}
