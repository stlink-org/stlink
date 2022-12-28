/*
 * File: common_flash.h
 *
 * Flash operations
 */

#ifndef COMMON_FLASH_H
#define COMMON_FLASH_H

void lock_flash(stlink_t *);
void clear_flash_error(stlink_t *);
void wait_flash_busy(stlink_t *);
int check_flash_error(stlink_t *);
int unlock_flash_if(stlink_t *);
int lock_flash_option(stlink_t *);
int unlock_flash_option_if(stlink_t *);
void write_flash_cr_psiz(stlink_t *, uint32_t, unsigned);
void clear_flash_cr_pg(stlink_t *, unsigned);

// TODO: move to private defines if possible

#define BANK_1 0
#define BANK_2 1

uint32_t read_flash_cr(stlink_t *, unsigned);
uint32_t get_stm32l0_flash_base(stlink_t *);
#endif // STLINK_H
