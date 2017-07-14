#ifndef STLINK_CHIPID_H_
#define STLINK_CHIPID_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Chip IDs are explained in the appropriate programming manual for the
 * DBGMCU_IDCODE register (0xE0042000)
 * stm32 chipids, only lower 12 bits..
 */
enum stlink_stm32_chipids {
	STLINK_CHIPID_STM32_F1_MEDIUM        = 0x410,
	STLINK_CHIPID_STM32_F2               = 0x411,
	STLINK_CHIPID_STM32_F1_LOW           = 0x412,
	STLINK_CHIPID_STM32_F4               = 0x413,
	STLINK_CHIPID_STM32_F1_HIGH          = 0x414,
	STLINK_CHIPID_STM32_L4               = 0x415,
	STLINK_CHIPID_STM32_L1_MEDIUM        = 0x416,
	STLINK_CHIPID_STM32_L0               = 0x417,
	STLINK_CHIPID_STM32_F1_CONN          = 0x418,
	STLINK_CHIPID_STM32_F4_HD            = 0x419,
	STLINK_CHIPID_STM32_F1_VL_MEDIUM_LOW = 0x420,
	STLINK_CHIPID_STM32_F446             = 0x421,
	STLINK_CHIPID_STM32_F3               = 0x422,
	STLINK_CHIPID_STM32_F4_LP            = 0x423,
	STLINK_CHIPID_STM32_L0_CAT2          = 0x425,
	STLINK_CHIPID_STM32_L1_MEDIUM_PLUS   = 0x427,
	STLINK_CHIPID_STM32_F1_VL_HIGH       = 0x428,
	STLINK_CHIPID_STM32_L1_CAT2          = 0x429,
	STLINK_CHIPID_STM32_F1_XL            = 0x430,
	STLINK_CHIPID_STM32_F411RE           = 0x431,
	STLINK_CHIPID_STM32_F37x             = 0x432,
	STLINK_CHIPID_STM32_F4_DE            = 0x433,
	STLINK_CHIPID_STM32_F4_DSI           = 0x434,
	/*
	* 0x435 covers STM32L43xxx and STM32L44xxx devices
	* 0x461 covers STM32L496xx and STM32L4A6xx devices
	* 0x462 covers STM32L45xxx and STM32L46xxx devices
	*/
	STLINK_CHIPID_STM32_L43X             = 0x435,
	STLINK_CHIPID_STM32_L496X            = 0x461,
	STLINK_CHIPID_STM32_L46X             = 0x462,
	/*
	* 0x436 is actually assigned to some L1 chips that are called "Medium-Plus"
	* and some that are called "High".  0x427 is assigned to the other "Medium-
	* plus" chips.  To make it a bit simpler we just call 427 MEDIUM_PLUS and
	* 0x436 HIGH.
	*/
	STLINK_CHIPID_STM32_L1_HIGH          = 0x436,
	STLINK_CHIPID_STM32_L152_RE          = 0x437,
	STLINK_CHIPID_STM32_F334             = 0x438,
	STLINK_CHIPID_STM32_F3_SMALL         = 0x439,
	STLINK_CHIPID_STM32_F0               = 0x440,
	STLINK_CHIPID_STM32_F412             = 0x441,
	STLINK_CHIPID_STM32_F09X             = 0x442,
	STLINK_CHIPID_STM32_F0_SMALL         = 0x444,
	STLINK_CHIPID_STM32_F04              = 0x445,
	STLINK_CHIPID_STM32_F303_HIGH        = 0x446,
	STLINK_CHIPID_STM32_L0_CAT5          = 0x447,
	STLINK_CHIPID_STM32_F0_CAN           = 0x448,
	STLINK_CHIPID_STM32_F7               = 0x449,
	STLINK_CHIPID_STM32_F7XXXX           = 0x451,
	STLINK_CHIPID_STM32_L011             = 0x457,
	STLINK_CHIPID_STM32_F410             = 0x458,
	STLINK_CHIPID_STM32_F413             = 0x463
};

/**
 * Chipid parameters
 */
struct stlink_chipid_params {
	uint32_t chip_id;
	char *description;
	enum stlink_flash_type flash_type;
	uint32_t flash_size_reg;
	uint32_t flash_pagesize;
	uint32_t sram_size;
	uint32_t bootrom_base;
	uint32_t bootrom_size;
};

const struct stlink_chipid_params *stlink_chipid_get_params(uint32_t chipid);

#ifdef __cplusplus
}
#endif

#endif /* STLINK_CHIPID_H_ */
