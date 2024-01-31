#ifndef SYS_TIME_H
#define SYS_TIME_H

#include <stdint.h>

#ifdef STLINK_HAVE_SYS_TIME_H

#include <sys/time.h>

#else

#include <windows.h>

struct timezone {
    int32_t tz_minuteswest;
    int32_t tz_dsttime;
};

int32_t gettimeofday(struct timeval *tv, struct timezone *tz);

#endif // STLINK_HAVE_SYS_TIME_H

#endif // SYS_TIME_H
