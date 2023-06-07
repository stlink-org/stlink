/* == nightwalker-87: TODO: CONTENT AND USE OF THIS SOURCE FILE IS TO BE VERIFIED (07.06.2023) == */
/* TODO: This file should be split up into new or existing modules. */

/*
 * File: common.h
 *
 *
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#include "map_file.h"
#include "md5.h"

int32_t check_file(stlink_t *, mapped_file_t *, stm32_addr_t);
void md5_calculate(mapped_file_t *);
void stlink_checksum(mapped_file_t *);
void stlink_fwrite_finalize(stlink_t *, stm32_addr_t);

#endif // COMMON_H
