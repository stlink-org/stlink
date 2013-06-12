#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#include "stlink-hex.h"

#define DATA_RECORD               00
#define EOF_RECORD                01
#define EXT_SEGMENT_ADDR_RECORD   02
#define EXT_LINEAR_ADDR_RECORD    04
#define START_LINEAR_ADDR_RECORD  05

typedef enum {
    HEX_FILL_ADDR_BOUNDRIES = 0,
    HEX_FILL_DATA
} hex_fill_mode_t;

typedef struct intel_hex_record {
    unsigned int type;
    unsigned int addr;
    unsigned int data_len;
    uint8_t      data[256];
} intel_hex_record_t;

/*
 *  Each record of an Intel Hex file is mad up of five fields:
 *     ":llaaaatt[dd...]cc"
 *
 * The colon starts every record.
 *
 * ll   - number of data bytes (dd)
 *
 * aaaa - the address field
 *
 * tt   - the record type, can be one of the following:
 *        00 - data record
 *        01 - end-of-file
 *        02 - extended segment address record
 *        04 - extebded linear address record
 *        05 - start linear address record
 *
 * dd   - data field that represents one byte of data.
 *
 * cc   - checksum field
 *
 * source: www.keil.com
 */
static int stlink_hex_get_record (intel_hex_record_t *record, char *buffer) {
    unsigned int  n;
    char         *data_pos;

    n = sscanf(buffer, ":%2x%4x%2x",
               &record->data_len,
               &record->addr,
               &record->type);
    if (n != 3) {
        return -1;
    }

    data_pos = &buffer[1 + 2 + 4 + 2]; /* colon + ll + aaaa + tt */
    memcpy(record->data, data_pos, record->data_len * 2);
    record->data[record->data_len * 2] = 0;

    return 0;
}

static int stlink_hex_fill(intel_hex_t *hex, FILE *fp, hex_fill_mode_t mode) {
    char               buffer[512];
    intel_hex_record_t record;
    uint16_t           ext_addr_mode = 0;
    uint32_t           ext_addr = 0;

    if (mode == HEX_FILL_ADDR_BOUNDRIES) {
        hex->end_addr   = 0;
        hex->start_addr = 0xFFFFFFFF;
    }

    while (!feof(fp)) {
        if (fgets(buffer, 512, fp)) {

            if (stlink_hex_get_record(&record, buffer) < 0) {
                return -1;
            }

            if (record.type == EXT_LINEAR_ADDR_RECORD ||
                record.type == EXT_SEGMENT_ADDR_RECORD) {
                ext_addr      = strtoul((char *) record.data, NULL, 16);
                ext_addr_mode = record.type;

            } else if (record.type == DATA_RECORD) {
                uint32_t addr;

                if (ext_addr_mode == EXT_LINEAR_ADDR_RECORD) {
                    addr = (ext_addr << 16) | record.addr;
                } else if (ext_addr_mode == EXT_SEGMENT_ADDR_RECORD) {
                    addr = (((uint16_t) ext_addr) << 4) | record.addr;
                } else {
                    addr = record.addr;
                }

                if (mode == HEX_FILL_ADDR_BOUNDRIES) {

                    if (addr > hex->end_addr) {
                        hex->end_addr = addr + record.data_len;
                    }
                    if (addr < hex->start_addr) {
                        hex->start_addr = addr;
                    }

                } else if (mode == HEX_FILL_DATA) {
                    char    *pos;
                    uint32_t offset;
                    int      i;

                    offset = addr - hex->start_addr;
                    pos = (char *) record.data;

                    for (i = 0; i < record.data_len; i++) {
                        sscanf(pos, "%2X", (unsigned int *) &hex->data[offset + i]);
                        pos += 2;
                    }
                }
            }
        }
    }
    return 0;
}

int stlink_hex_parse (intel_hex_t *hex, const char *hex_path) {
    FILE *fp;
    int   ret = 0;

    fp = fopen(hex_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "fopen(%s) == NULL\n", hex_path);
        return -1;
    }

    /* first parse the file to get the address boundries */
    if (stlink_hex_fill(hex, fp, HEX_FILL_ADDR_BOUNDRIES) < 0) {
        fprintf(stderr, "failed to read hex\n");
        ret = -1;
        goto out;
    }

    hex->len  = hex->end_addr - hex->start_addr;
    hex->data = malloc (hex->len);

    /* fill "holes" with 0xFF */
    memset(hex->data, 0xFF, hex->len);

    /* rewind stream to be able to fill data */
    if (fseek(fp, 0L, SEEK_SET) < 0) {
        fprintf(stderr, "fseek == -1\n");
        ret = -1;
        goto out;
    }

    if (stlink_hex_fill(hex, fp, HEX_FILL_DATA) < 0) {
        ret = -1;
        goto out;
    }

 out:
    fclose (fp);
    return ret;
}
