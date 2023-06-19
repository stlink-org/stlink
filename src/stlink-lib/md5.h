/*
 * File: md5.h
 *
 * MD5 hash function
 */

#ifndef MD5_H
#define MD5_H

#include "map_file.h"

void md5_calculate(mapped_file_t *);
void stlink_checksum(mapped_file_t *);

#endif // MD5_H