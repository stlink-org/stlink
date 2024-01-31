/*
 * File: helper.c
 *
 * General helper functions
 */

#ifdef STLINK_HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <sys_time.h>
#endif // STLINK_HAVE_SYS_TIME_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "helper.h"

uint32_t time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int32_t arg_parse_freq(const char *str) {
    char *tail;
    int32_t value = (int32_t)strtol(str, &tail, 10);

    if (tail[0] == 'M' && tail[1] == '\0') {
        value = value*1000;
    } else if ((tail[0] != 'k' || tail[1] != '\0') && tail[0] != '\0') {
        return -1;
    }

    return value;
}
