/**
  ******************************************************************************
  * @file           : helper.c
  * @brief          : General helper functions
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#include "helper.h"


uint32_t time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int32_t arg_parse_freq(const char *str) {
    if(str == NULL) {
        return -1;                    // faulty input
    }
    
    char* tail = NULL;
    int32_t value = (uint32_t) strtol(str, &tail, 10);

    /* == Input value interpretation ==
     *
     * These decimal values represent kHz.
     * The prefix "k" may be added for better understanding, but has no effect.
     * In addition, values can be interpreted as thousands of kHz thus representing MHz.
     * This is achieved by using the prefix "M".
     */

    if(tail == NULL) {
        return -1;                    // faulty integer conversion
    } else if(tail[0] == '\0') {
        return value;                 // no prefix: kHz
    } else if((tail[0] == 'k' || tail[0] == 'K') && tail[1] == '\0') {
        return value;                 // k prefix: kHz
    } else if(tail[0] == 'M' && tail[1] == '\0') {
        return (value * 1000);        // M prefix: MHz
    }
    return -1;                        // invalid prefix
}
