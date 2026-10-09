/**
  ******************************************************************************
  * @file             sys_time.c
  * @brief            gettimeofday() for Windows
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-10-08
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */


#include <stdint.h>

#include "sys_time.h"


#ifndef STLINK_HAVE_SYS_TIME_H

#include <time.h>

/* Simple gettimeofday implementation */
int32_t gettimeofday(struct timeval *tv, struct timezone *tz) {
    FILETIME ftime;
    ULARGE_INTEGER ulint;
    static int32_t tzflag = 0;

    if(NULL != tv) {
        GetSystemTimeAsFileTime(&ftime);
        ulint.LowPart = ftime.dwLowDateTime;
        ulint.HighPart = ftime.dwHighDateTime;

        ulint.QuadPart -= 116444736000000000LL; // shift base to unix epoch
        ulint.QuadPart /= 10LL; // 100ns ticks to microseconds
        tv->tv_sec = (int32_t) (ulint.QuadPart / 1000000LL);
        tv->tv_usec = (int32_t) (ulint.QuadPart % 1000000LL);
    }

    if(NULL != tz) {
        if(!tzflag) {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}

#endif // STLINK_HAVE_SYS_TIME_H
