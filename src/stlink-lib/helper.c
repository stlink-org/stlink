#include <helper.h>

#include <stddef.h>

#ifdef STLINK_HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <sys_time.h>
#endif

unsigned time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
