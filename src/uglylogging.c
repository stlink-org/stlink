/* 
 * UglyLogging.  Slow, yet another wheel reinvented, but enough to make the 
 * rest of our code pretty enough.
 * 
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "uglylogging.h"

static int max_level;

int ugly_init(int maximum_threshold) {
    max_level = maximum_threshold;
    return 0;
}

int ugly_log(int level, const char *tag, const char *format, ...) {
    if (level > max_level) {
        return 0;
    }
    va_list args;
    va_start(args, format);
    time_t mytt = time(NULL);
    struct tm *tt;
    tt = localtime(&mytt);
    fprintf(stderr, "%d-%02d-%02dT%02d:%02d:%02d ", tt->tm_year + 1900, tt->tm_mon + 1, tt->tm_mday, tt->tm_hour, tt->tm_min, tt->tm_sec);
    switch (level) {
        case UDEBUG:
            fprintf(stderr, "DEBUG %s: ", tag);
            break;
        case UINFO:
            fprintf(stderr, "INFO %s: ", tag);
            break;
        case UWARN:
            fprintf(stderr, "WARN %s: ", tag);
            break;
        case UERROR:
            fprintf(stderr, "ERROR %s: ", tag);
            break;
        case UFATAL:
            fprintf(stderr, "FATAL %s: ", tag);
            vfprintf(stderr, format, args); 
            exit(EXIT_FAILURE);
            // NEVER GETS HERE!!!
            break;
        default:
            fprintf(stderr, "%d %s: ", level, tag);
            break;
    }
    vfprintf(stderr, format, args); 
    va_end(args);
    return 1;
}