/*
 * File: chipid.h
 *
 * Chip-ID parametres
 */

#ifndef CHIPID_H
#define CHIPID_H

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
  struct stlink_chipid_params *next;
};

struct stlink_chipid_params *stlink_chipid_get_params(uint32_t chipid);

void dump_a_chip(struct stlink_chipid_params *dev);
void process_chipfile(char *fname);
void init_chipids(char *dir_to_scan);

#endif // CHIPID_H
