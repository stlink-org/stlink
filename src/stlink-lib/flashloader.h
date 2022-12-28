/*
 * File: flashloader.h
 *
 * Flash loader
 */

#include <stdio.h>
#include <stlink.h>

int stlink_flashloader_start(stlink_t *sl, flash_loader_t *fl);
int stlink_flashloader_write(stlink_t *sl, flash_loader_t *fl, stm32_addr_t addr, uint8_t* base, uint32_t len);
int stlink_flashloader_stop(stlink_t *sl, flash_loader_t *fl);

