/*
 * File: common.h
 *
 * General helper functions
 */

#ifndef COMMON_H
#define COMMON_H

int check_file(stlink_t *, mapped_file_t *, stm32_addr_t);
void md5_calculate(mapped_file_t *);
void stlink_checksum(mapped_file_t *);
void stlink_fwrite_finalize(stlink_t *, stm32_addr_t);

#endif // COMMON_H
