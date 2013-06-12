/*
 * File:   stlink-hex.h
 *
 * Typedef and public functions for handling Intel HEX format
 *
 */

#include <stdint.h>

#ifndef STLINK_HEX_H
#define STLINK_HEX_H

typedef struct intel_hex {
    uint8_t     *data;
    uint32_t     len;
    uint32_t     start_addr;
    uint32_t     end_addr;
} intel_hex_t;

int stlink_hex_parse (intel_hex_t *hex, const char *hex_path);

#endif
