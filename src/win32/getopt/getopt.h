#ifndef GETOPT_H
#define GETOPT_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_MSC_VER)
// These may be used to initialize structures and it fails with MSVC
#define no_argument 0
#define required_argument  1
#define optional_argument  2
#else
extern const int32_t no_argument;
extern const int32_t required_argument;
extern const int32_t optional_argument;
#endif

extern char* optarg;
extern int32_t optind, opterr, optopt;

struct option {
    const char* name;
    int32_t has_arg;
    int* flag;
    int32_t val;
};

int32_t getopt(int32_t argc, char* const argv[], const char* optstring);

int32_t getopt_long(int32_t argc,
                char* const argv[],
                const char* optstring,
                const struct option* longopts,
                int* longindex);

#if defined(__cplusplus)
}
#endif

#endif // GETOPT_H
