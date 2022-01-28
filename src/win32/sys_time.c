#include "sys_time.h"

#ifndef STLINK_HAVE_SYS_TIME_H

#include <time.h>

/* Simple gettimeofday implementation without converting Windows time to Linux time */
int gettimeofday(struct timeval *tv, struct timezone *tz) {
    FILETIME ftime;
    ULARGE_INTEGER ulint;
    static int tzflag = 0;

    if(NULL != tv) {
        GetSystemTimeAsFileTime(&ftime);
        ulint.LowPart = ftime.dwLowDateTime;
        ulint.HighPart = ftime.dwHighDateTime;

        tv->tv_sec = (long)(ulint.QuadPart / 10000000L);
        tv->tv_usec = (long)(ulint.QuadPart % 10000000L);
    }

    if(NULL != tz) {
        if (!tzflag) {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}

#endif //STLINK_HAVE_SYS_TIME_H
