/*
 * File: common_flash.h
 *
 * Flash operations
 */

#ifndef COMMON_FLASH_H
#define COMMON_FLASH_H

#define BANK_1 0
#define BANK_2 1

uint32_t get_stm32l0_flash_base(stlink_t *);
uint32_t read_flash_cr(stlink_t *, uint32_t);
void lock_flash(stlink_t *);
// static inline int32_t write_flash_sr(stlink_t *sl, uint32_t bank, uint32_t val);
void clear_flash_error(stlink_t *);
uint32_t read_flash_sr(stlink_t *sl, uint32_t bank);
uint32_t is_flash_busy(stlink_t *sl);
void wait_flash_busy(stlink_t *);
int32_t check_flash_error(stlink_t *);
// static inline uint32_t is_flash_locked(stlink_t *sl);
// static void unlock_flash(stlink_t *sl);
int32_t unlock_flash_if(stlink_t *);
int32_t lock_flash_option(stlink_t *);
// static bool is_flash_option_locked(stlink_t *sl);
// static int32_t unlock_flash_option(stlink_t *sl);
int32_t unlock_flash_option_if(stlink_t *);
void write_flash_cr_psiz(stlink_t *, uint32_t, uint32_t);
void clear_flash_cr_pg(stlink_t *, uint32_t);
// static void wait_flash_busy_progress(stlink_t *sl);
// static inline void write_flash_ar(stlink_t *sl, uint32_t n, uint32_t bank);
// static inline void write_flash_cr_snb(stlink_t *sl, uint32_t n, uint32_t bank);
// static void set_flash_cr_per(stlink_t *sl, uint32_t bank);
// static void clear_flash_cr_per(stlink_t *sl, uint32_t bank);
// static inline void write_flash_cr_bker_pnb(stlink_t *sl, uint32_t n);
// static void set_flash_cr_strt(stlink_t *sl, uint32_t bank);
// static void set_flash_cr_mer(stlink_t *sl, bool v, uint32_t bank);
int32_t stlink_erase_flash_page(stlink_t *sl, stm32_addr_t flashaddr);
int32_t stlink_erase_flash_section(stlink_t *sl, stm32_addr_t base_addr, uint32_t size, bool align_size);
int32_t stlink_erase_flash_mass(stlink_t *sl);
int32_t stlink_mwrite_flash(stlink_t *sl, uint8_t *data, uint32_t length, stm32_addr_t addr);
int32_t stlink_fwrite_flash(stlink_t *sl, const char *path, stm32_addr_t addr);
int32_t stlink_fcheck_flash(stlink_t *sl, const char *path, stm32_addr_t addr);
int32_t stlink_verify_write_flash(stlink_t *sl, stm32_addr_t address, uint8_t *data, uint32_t length);
int32_t stlink_check_address_range_validity(stlink_t *sl, stm32_addr_t addr, uint32_t size);
int32_t stlink_check_address_alignment(stlink_t *sl, stm32_addr_t addr);
int32_t stlink_write_flash(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len, uint8_t eraseonly);
void stlink_fwrite_finalize(stlink_t *, stm32_addr_t);

#endif // COMMON_FLASH_H
