/*
 * File: getopt.h
 *
 *
 */

#ifndef GETOPT_H
#define GETOPT_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>


#if defined(__cplusplus)
extern "C" {
#endif

/* Defines to initialize static/global struct option arrays */
// These must be compile-time constants rather than extern variables.
#define no_argument 0
#define required_argument 1
#define optional_argument 2


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
