#ifndef STLINK_CHIPID_H_
#define STLINK_CHIPID_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Chip IDs are explained in the appropriate programming manual for the
 * DBGMCU_IDCODE register (0xE0042000)
 * stm32 chipids, only lower 12 bits...
 */
enum stlink_stm32_chipids {
    STLINK_CHIPID_UNKNOWN                = 0x000,

    STLINK_CHIPID_STM32_F1_MD            = 0x410, /* medium density */
    STLINK_CHIPID_STM32_F2               = 0x411,
    STLINK_CHIPID_STM32_F1_LD            = 0x412, /* low density */
    STLINK_CHIPID_STM32_F4               = 0x413,
    STLINK_CHIPID_STM32_F1_HD            = 0x414, /* high density */
    STLINK_CHIPID_STM32_L4               = 0x415,
    STLINK_CHIPID_STM32_L1_MD            = 0x416, /* medium density */
    STLINK_CHIPID_STM32_L0               = 0x417,
    STLINK_CHIPID_STM32_F1_CONN          = 0x418, /* connectivity line */
    STLINK_CHIPID_STM32_F4_HD            = 0x419, /* high density */
    STLINK_CHIPID_STM32_F1_VL_MD_LD      = 0x420, /* value line medium & low density */
    STLINK_CHIPID_STM32_F446             = 0x421,
    STLINK_CHIPID_STM32_F3               = 0x422,
    STLINK_CHIPID_STM32_F4_LP            = 0x423,
    STLINK_CHIPID_STM32_L0_CAT2          = 0x425,
    STLINK_CHIPID_STM32_L1_MD_PLUS       = 0x427, /* medium density plus */
    STLINK_CHIPID_STM32_F1_VL_HD         = 0x428, /* value line high density */
    STLINK_CHIPID_STM32_L1_CAT2          = 0x429,
    STLINK_CHIPID_STM32_F1_XLD           = 0x430, /* extra low density plus */
    STLINK_CHIPID_STM32_F411xx           = 0x431,
    STLINK_CHIPID_STM32_F37x             = 0x432,
    STLINK_CHIPID_STM32_F4_DE            = 0x433,
    STLINK_CHIPID_STM32_F4_DSI           = 0x434,
    STLINK_CHIPID_STM32_L43x_L44x        = 0x435,
    STLINK_CHIPID_STM32_L1_MD_PLUS_HD    = 0x436, /* medium density plus & high density */
    STLINK_CHIPID_STM32_L152_RE          = 0x437,
    STLINK_CHIPID_STM32_F334             = 0x438,
    STLINK_CHIPID_STM32_F3xx_SMALL       = 0x439,
    STLINK_CHIPID_STM32_F0               = 0x440,
    STLINK_CHIPID_STM32_F412             = 0x441,
    STLINK_CHIPID_STM32_F09x             = 0x442,
    STLINK_CHIPID_STM32_F0xx_SMALL       = 0x444,
    STLINK_CHIPID_STM32_F04              = 0x445,
    STLINK_CHIPID_STM32_F303_HD          = 0x446, /* high density */
    STLINK_CHIPID_STM32_L0_CAT5          = 0x447,
    STLINK_CHIPID_STM32_F0_CAN           = 0x448,
    STLINK_CHIPID_STM32_F7               = 0x449, /* ID found on the Nucleo F746ZG board */
    STLINK_CHIPID_STM32_H74xxx           = 0x450, /* RM0433, p.3189 */
    STLINK_CHIPID_STM32_F76xxx           = 0x451,
    STLINK_CHIPID_STM32_F72xxx           = 0x452, /* ID found on the Nucleo F722ZE board */
    STLINK_CHIPID_STM32_G0_CAT4          = 0x456, /* G051/G061 */
    STLINK_CHIPID_STM32_L011             = 0x457,
    STLINK_CHIPID_STM32_F410             = 0x458,
    STLINK_CHIPID_STM32_G0_CAT2          = 0x460, /* G070/G071/G081 */
    STLINK_CHIPID_STM32_L496x_L4A6x      = 0x461,
    STLINK_CHIPID_STM32_L45x_L46x        = 0x462,
    STLINK_CHIPID_STM32_F413             = 0x463,
    STLINK_CHIPID_STM32_L41x_L42x        = 0x464,
    STLINK_CHIPID_STM32_G0_CAT1          = 0x466, /* G030/G031/G041 */
    STLINK_CHIPID_STM32_G0_CAT3          = 0x467, /* G0B1/G0C1 */
    STLINK_CHIPID_STM32_G4_CAT2          = 0x468, /* RM0440, s46.6.1 "MCU device ID code" */
    STLINK_CHIPID_STM32_G4_CAT3          = 0x469,
    STLINK_CHIPID_STM32_L4Rx             = 0x470, /* RM0432, p. 2247, found on the STM32L4R9I-DISCO board */
    STLINK_CHIPID_STM32_L4PX             = 0x471, /* RM0432, p. 2247 */
    STLINK_CHIPID_STM32_G4_CAT4          = 0x479,
    STLINK_CHIPID_STM32_H7Ax             = 0x480, /* RM0455, p.2863 */
    STLINK_CHIPID_STM32_H72x             = 0x483, /* RM0468, p.3199 */
    STLINK_CHIPID_STM32_WB55             = 0x495,
    STLINK_CHIPID_STM32_WLE              = 0x497
};

#define CHIP_F_HAS_DUAL_BANK    (1 << 0)
#define CHIP_F_HAS_SWO_TRACING  (1 << 1)

/** Chipid parameters */
struct stlink_chipid_params {
    uint32_t chip_id;
    char *description;
    enum stlink_flash_type flash_type;
    uint32_t flash_size_reg;
    uint32_t flash_pagesize;
    uint32_t sram_size;
    uint32_t bootrom_base;
    uint32_t bootrom_size;
    uint32_t option_base;
    uint32_t option_size;
    uint32_t flags;
  struct stlink_chipid_params * next;
};


struct stlink_chipid_params *stlink_chipid_get_params(uint32_t chipid);
  void init_chipids(char *dir_to_scan); 

#ifdef __cplusplus
}
#endif

#endif // STLINK_CHIPID_H_
