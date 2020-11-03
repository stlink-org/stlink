#ifndef STLINK_TIME_H
#define STLINK_TIME_H

#ifdef STLINK_HAVE_SYS_TIME_H
#include <sys/time.h>
#else

struct timeval {
    long tv_sec;
    long tv_usec;
};

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

#ifdef __cplusplus
extern "C" {
#endif

int gettimeofday(struct timeval *tv, struct timezone *tz);

#ifdef __cplusplus
}
#endif

#endif /* STLINK_HAVE_SYS_TIME_H */

#endif /* STLINK_TIME_H */
