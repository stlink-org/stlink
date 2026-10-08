/**
  ******************************************************************************
  * @file             sys_time.h
  * @brief            gettimeofday() for Windows
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-10-08
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */


#ifndef SYS_TIME_H
#define SYS_TIME_H

#include <stdint.h>


#ifdef STLINK_HAVE_SYS_TIME_H

#include <sys/time.h>

#else

#include <winsock2.h> // struct timeval, including with WIN32_LEAN_AND_MEAN
#include <windows.h>

struct timezone {
    int32_t tz_minuteswest;
    int32_t tz_dsttime;
};

int32_t gettimeofday(struct timeval *tv, struct timezone *tz);

#endif // STLINK_HAVE_SYS_TIME_H

#endif // SYS_TIME_H
