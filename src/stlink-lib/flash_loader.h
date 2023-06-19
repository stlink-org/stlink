/*
 * File: flash_loader.h
 *
 * Flash loaders
 */
 
#ifndef FLASH_LOADER_H
#define FLASH_LOADER_H

int32_t stlink_flash_loader_init(stlink_t *sl, flash_loader_t* fl);
// static int32_t loader_v_dependent_assignment(stlink_t *sl,
//                                             const uint8_t **loader_code, uint32_t *loader_size,
//                                             const uint8_t *high_v_loader, uint32_t high_v_loader_size,
//                                             const uint8_t *low_v_loader, uint32_t low_v_loader_size);
int32_t stlink_flash_loader_write_to_sram(stlink_t *sl, stm32_addr_t* addr, uint32_t* size);
int32_t stlink_flash_loader_run(stlink_t *sl, flash_loader_t* fl, stm32_addr_t target, const uint8_t* buf, uint32_t size);


/* === Functions from old header file flashloader.h === */

int32_t stm32l1_write_half_pages(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len, uint32_t pagesize);
// static void set_flash_cr_pg(stlink_t *sl, uint32_t bank);
// static void set_dma_state(stlink_t *sl, flash_loader_t *fl, int32_t bckpRstr);
int32_t stlink_flashloader_start(stlink_t *sl, flash_loader_t *fl);
int32_t stlink_flashloader_write(stlink_t *sl, flash_loader_t *fl, stm32_addr_t addr, uint8_t *base, uint32_t len);
int32_t stlink_flashloader_stop(stlink_t *sl, flash_loader_t *fl);

#endif // FLASH_LOADER_H
