#include <helper.h>

#include <stddef.h>
#include <stdlib.h>

#ifdef STLINK_HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <sys_time.h>
#endif

unsigned time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int arg_parse_freq(const char *str) {
    char *tail;
    int value = (int)strtol(str, &tail, 10);

    if (tail[0] == 'M' && tail[1] == '\0') {
        value = value*1000;
    } else if ((tail[0] != 'k' || tail[1] != '\0') && tail[0] != '\0') {
        return -1;
    }

    return value;
}
