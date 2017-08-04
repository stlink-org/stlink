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
#include <string.h>

#include "stlink/logging.h"

static int max_level = UINFO;

#define TOSTR(s) #s
#define STR(s) TOSTR(s)

static const char* source_dir = STR(CMAKE_SOURCE_DIR);
static const char* build_dir = STR(CMAKE_BINARY_DIR);
static int tagskip = 0;

int ugly_init(int maximum_threshold) {
    max_level = maximum_threshold;
    tagskip = 0;

    // If build_dir is a subdir of source_dir
   	if (strstr(build_dir, source_dir) != 0) {
   	   	int len = strlen(source_dir);
   	   	if (build_dir[len] == '/') {
   	   	   	for (unsigned idx=len; idx<strlen(build_dir); idx++) {
   	   	   	   	if (build_dir[idx] == '/')
   	   	   	   	   	tagskip += 3; // "../"
   	   	   	}
   	   	}
   	}

   	// Then build_dir is not a subdir
   	if (tagskip == 0)
   	   	tagskip = strlen(source_dir) + 1;
    return 0;
}

int ugly_log(int level, const char *tag, const char *format, ...) {
    if (level > max_level) {
        return 0;
    }
    tag += tagskip;
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
