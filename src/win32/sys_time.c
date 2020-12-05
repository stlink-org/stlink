#include <stdlib.h>
#include <unistd.h>

#include "sys_time.h"

/* Simple gettimeofday implementation without converting Windows time to Linux time */
int gettimeofday(struct timeval *tv, struct timezone *tz) {
    FILETIME ftime;
    ULARGE_INTEGER ulint;

    GetSystemTimeAsFileTime(&ftime);
    ulint.LowPart = ftime.dwLowDateTime;
    ulint.HighPart = ftime.dwHighDateTime;

    tv->tv_sec = (long)(ulint.QuadPart / 10000000L);
    tv->tv_usec = (long)(ulint.QuadPart % 10000000L);

    return 0;
    (void)tz;
}
