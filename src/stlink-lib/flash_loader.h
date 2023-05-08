/*
 * File: flash_loader.h
 *
 * Flash loader
 */
 
#ifndef FLASH_LOADER_H
#define FLASH_LOADER_H

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#include <stlink.h>

int32_t stlink_flash_loader_init(stlink_t *sl, flash_loader_t* fl);
int32_t stlink_flash_loader_write_to_sram(stlink_t *sl, stm32_addr_t* addr, size_t* size);
int32_t stlink_flash_loader_run(stlink_t *sl, flash_loader_t* fl, stm32_addr_t target, const uint8_t* buf, size_t size);

int32_t stlink_flashloader_start(stlink_t *sl, flash_loader_t *fl);
int32_t stlink_flashloader_write(stlink_t *sl, flash_loader_t *fl, stm32_addr_t addr, uint8_t* base, uint32_t len);
int32_t stlink_flashloader_stop(stlink_t *sl, flash_loader_t *fl);

#endif // FLASH_LOADER_H
