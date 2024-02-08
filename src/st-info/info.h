/*
 * File: info.h
 *
 * Tool st-info 
 */

#ifndef INFO_H
#define INFO_H

// static void usage(void);
// static void stlink_print_version(stlink_t *sl);
// static void stlink_print_info(stlink_t *sl);

// static void stlink_probe(enum connect_type connect, int32_t freq) { };
static int32_t print_data(int32_t ac, char **av);
int32_t main(int32_t ac, char** av);

#endif // INFO_H