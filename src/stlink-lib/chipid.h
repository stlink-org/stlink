#ifndef STLINK_CHIPID_H_
#define STLINK_CHIPID_H_

#include <stm32.h>
#include <stlink.h>

/* Chipid parametres */
struct stlink_chipid_params {
    char *dev_type;
    char *ref_manual_id;
    uint32_t chip_id;
    enum stm32_flash_type flash_type;
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

#endif // STLINK_CHIPID_H_
