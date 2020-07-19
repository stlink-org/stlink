#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "flash.h"

static bool starts_with(const char * str, const char * prefix) {
    size_t n = strlen(prefix);

    if (strlen(str) < n) { return(false); }

    return (0 == strncmp(str, prefix, n));
}

// support positive integer from 0 to UINT64_MAX
// support decimal, hexadecimal, octal, binary format like 0xff 12 1k 1M, 0b1001
// negative numbers are not supported
// return 0 if success else return -1
static int get_long_integer_from_char_array (const char *const str, uint64_t *read_value) {
    uint64_t value;
    char *tail;

    if (starts_with (str, "0x") || starts_with (str, "0X")) {          // hexadecimal
        value = strtoul (str + 2, &tail, 16);
    } else if (starts_with (str, "0b") || starts_with (str, "0B")) {   // binary
        value = strtoul (str + 2, &tail, 2);
    } else if (starts_with (str, "0")) {                               // octal
        value = strtoul (str + 1, &tail, 8);
    } else {                                                           // decimal
        value = strtoul (str, &tail, 10);
    }

    if (((tail[0] == 'k') || (tail[0] == 'K')) && (tail[1] == '\0')) {
        value = value * 1024;
    } else if (((tail[0] == 'm') || (tail[0] == 'M')) && (tail[1] == '\0')) {
        value = value * 1024 * 1024;
    } else if (tail[0] == '\0') {
        /* value not changed */
    } else {
        return(-1);
    }

    *read_value = value;
    return(0);
}

// support positive integer from 0 to UINT32_MAX
// support decimal, hexadecimal, octal, binary format like 0xff 12 1k 1M, 0b1001
// negative numbers are not supported
// return 0 if success else return -1
static int get_integer_from_char_array (const char *const str, uint32_t *read_value) {
    uint64_t value;
    int result = get_long_integer_from_char_array (str, &value);

    if (result != 0) {
        return(result);
    } else if (value > UINT32_MAX) {
        fprintf (stderr, "*** Error: Integer greater than UINT32_MAX, cannot convert to int32_t\n");
        return(-1);
    } else {
        *read_value = (uint32_t)value;
        return(0);
    }
}

static int invalid_args(const char *expected) {
    fprintf(stderr, "*** Error: Expected args for this command: %s\n", expected);
    return(-1);
}

static int bad_arg(const char *arg) {
    fprintf(stderr, "*** Error: Invalid value for %s\n", arg);
    return(-1);
}

int flash_get_opts(struct flash_opts* o, int ac, char** av) {

    // defaults
    memset(o, 0, sizeof(*o));
    o->log_level = STND_LOG_LEVEL;

    // options
    int result;

    while (ac >= 1) {
        if (strcmp(av[0], "--version") == 0) {
            printf("v%s\n", STLINK_VERSION);
            exit(EXIT_SUCCESS);
        } else if (strcmp(av[0], "--debug") == 0) {
            o->log_level = DEBUG_LOG_LEVEL;
        } else if (strcmp(av[0], "--opt") == 0) {
            o->opt = ENABLE_OPT;
        } else if (strcmp(av[0], "--reset") == 0) {
            o->reset = 1;
        } else if (strcmp(av[0], "--serial") == 0 || starts_with(av[0], "--serial=")) {
            const char * serial;

            if (strcmp(av[0], "--serial") == 0) {
                ac--;
                av++;

                if (ac < 1) { return(-1); }

                serial = av[0];
            } else {
                serial = av[0] + strlen("--serial=");
            }

            /** @todo This is not really portable, as strlen really returns size_t we need to obey
                      and not cast it to a signed type. */
            int j = (int)strlen(serial);
            int length = j / 2; // the length of the destination-array

            if (j % 2 != 0) { return(-1); }

            for (size_t k = 0; j >= 0 && k < sizeof(o->serial); ++k, j -= 2) {
                char buffer[3] = {0};
                memcpy(buffer, serial + j, 2);
                o->serial[length - k] = (uint8_t)strtol(buffer, NULL, 16);
            }
        } else if (strcmp(av[0], "--area") == 0 || starts_with(av[0], "--area=")) {
            const char * area;

            if (strcmp(av[0], "--area") == 0) {
                ac--;
                av++;

                if (ac < 1) { return(-1); }

                area = av[0];
            } else {
                area = av[0] + strlen("--area=");
            }

            if (strcmp(area, "main") == 0) {
                o->area = FLASH_MAIN_MEMORY;
            } else if (strcmp(area, "system") == 0) {
                o->area = FLASH_SYSTEM_MEMORY;
            } else if (strcmp(area, "otp") == 0) {
                o->area = FLASH_OTP;
            } else if (strcmp(area, "option") == 0) {
                o->area = FLASH_OPTION_BYTES;
            } else if (strcmp(area, "option_boot_add") == 0) {
                o->area = FLASH_OPTION_BYTES_BOOT_ADD;
            } else if (strcmp(area, "optcr") == 0) {
                o->area = FLASH_OPTCR;
            } else if (strcmp(area, "optcr1") == 0) {
                o->area = FLASH_OPTCR1;
            } else {
                return(-1);
            }

        } else if (strcmp(av[0], "--freq") == 0 || starts_with(av[0], "--freq=")) {
            const char* freq;

            if (strcmp(av[0], "--freq") == 0) {
                ac--;
                av++;

                if (ac < 1) {
                    return(-1);
                }

                freq = av[0];
            } else {
                freq = av[0] + strlen("--freq=");
            }

            if (strcmp(freq, "5K") == 0 || strcmp(freq, "5k") == 0) {
                o->freq = 5;
            } else if (strcmp(freq, "15K") == 0 || strcmp(freq, "15k") == 0) {
                o->freq = 15;
            } else if (strcmp(freq, "25K") == 0 || strcmp(freq, "25k") == 0) {
                o->freq = 25;
            } else if (strcmp(freq, "50K") == 0 || strcmp(freq, "50k") == 0) {
                o->freq = 50;
            } else if (strcmp(freq, "100K") == 0 || strcmp(freq, "100k") == 0) {
                o->freq = 100;
            } else if (strcmp(freq, "125K") == 0 || strcmp(freq, "125k") == 0) {
                o->freq = 125;
            } else if (strcmp(freq, "240K") == 0 || strcmp(freq, "240k") == 0) {
                o->freq = 240;
            } else if (strcmp(freq, "480K") == 0 || strcmp(freq, "480k") == 0) {
                o->freq = 480;
            } else if (strcmp(freq, "950K") == 0 || strcmp(freq, "950k") == 0) {
                o->freq = 950;
            } else if (strcmp(freq, "1200K") == 0 || strcmp(freq, "1200k") == 0 ||
                       strcmp(freq, "1.2M") == 0 || strcmp(freq, "1.2m") == 0) {
                o->freq = 1200;
            } else if (strcmp(freq, "1800K") == 0 || strcmp(freq, "1800k") == 0 ||
                       strcmp(freq, "1.8M") == 0 || strcmp(freq, "1.8m") == 0) {
                o->freq = 1800;
            } else if (strcmp(freq, "4000K") == 0 || strcmp(freq, "4000k") == 0 ||
                       strcmp(freq, "4M") == 0 || strcmp(freq, "4m") == 0) {
                o->freq = 4000;
            } else {
                return(-1);
            }
        } else if (strcmp(av[0], "--format") == 0 || starts_with(av[0], "--format=")) {
            const char * format;

            if (strcmp(av[0], "--format") == 0) {
                ac--;
                av++;

                if (ac < 1) { return(-1); }

                format = av[0];
            } else {
                format = av[0] + strlen("--format=");
            }

            if (strcmp(format, "binary") == 0) {
                o->format = FLASH_FORMAT_BINARY;
            } else if (strcmp(format, "ihex") == 0) {
                o->format = FLASH_FORMAT_IHEX;
            } else {
                return(bad_arg("format"));
            }
        } else if ( starts_with(av[0], "--flash=")) {
            const char *arg = av[0] + strlen("--flash=");

            uint32_t flash_size;
            result = get_integer_from_char_array(arg, &flash_size);

            if (result != 0) {
                return(bad_arg ("--flash"));
            } else {
                o->flash_size = (size_t)flash_size;
            }
        } else if (strcmp(av[0], "--connect-under-reset") == 0) {
            o->connect_under_reset = true;
        } else {
            break; // non-option found

        }

        ac--;
        av++;
    }

    // command and (optional) device name
    while (ac >= 1) {
        if (strcmp(av[0], "erase") == 0) {
            if (o->cmd != FLASH_CMD_NONE) { return(-1); }
            o->cmd = FLASH_CMD_ERASE;
        } else if (strcmp(av[0], "read") == 0) {
            if (o->cmd != FLASH_CMD_NONE) { return(-1); }

            o->cmd = FLASH_CMD_READ;
        } else if (strcmp(av[0], "write") == 0) {
            if (o->cmd != FLASH_CMD_NONE) { return(-1); }

            o->cmd = FLASH_CMD_WRITE;
        } else if (strcmp(av[0], "reset") == 0) {
            if (o->cmd != FLASH_CMD_NONE) { return(-1); }

            o->cmd = CMD_RESET;
        } else {
            break;
        }

        ac--;
        av++;
    }

    switch (o->cmd) {
    case FLASH_CMD_NONE:     // no command found
        return(-1);

    case FLASH_CMD_ERASE:    // no more arguments expected
        if (ac != 0) { return(-1); }

        break;

    case FLASH_CMD_READ:     // expect filename, addr and size
        if ((o->area == FLASH_MAIN_MEMORY) || (o->area == FLASH_SYSTEM_MEMORY)) {
            if (ac != 3) { return invalid_args("read <path> <addr> <size>"); }
            
            o->filename = av[0];
            uint32_t address;
            result = get_integer_from_char_array(av[1], &address);
            if (result != 0) {
                return bad_arg ("addr");
            } else {
                o->addr = (stm32_addr_t) address;
            }

            uint32_t size;
            result = get_integer_from_char_array(av[2], &size);
            if (result != 0) {
                return bad_arg ("size");
            } else {
                o->size = (size_t) size;
            }

            break;
        } else if (o->area == FLASH_OTP) {
            return bad_arg("TODO: otp not implemented yet");
            if (ac > 1) { return invalid_args("otp read: [path]"); }
            if (ac > 0) { o->filename = av[0]; }
            break;
        } else if (o->area == FLASH_OPTION_BYTES) {
            if (ac > 2) { return invalid_args("option bytes read: [path] [size]"); }
            if (ac > 0) { o->filename = av[0]; }
            if (ac > 1) {
                uint32_t size;
                result = get_integer_from_char_array(av[1], &size);
                if (result != 0) {
                    return bad_arg("option bytes read: invalid size");
                } else {
                    o->size = (size_t) size;
                }
            }
            break;
        } else if (o->area == FLASH_OPTION_BYTES_BOOT_ADD) {
            if (ac > 0) { return invalid_args("option bytes boot_add read"); }
            break;
        } else if (o->area == FLASH_OPTCR) {
            if (ac > 0) { return invalid_args("option control register read"); }
            break;
        } else if (o->area == FLASH_OPTCR1) {
            if (ac > 0) { return invalid_args("option control register 1 read"); }
            break;
        }

        break;

    case FLASH_CMD_WRITE:
        // TODO: should be boot add 0 and boot add 1 uint32
        if (o->area == FLASH_OPTION_BYTES) { // expect filename and optional address
            if (ac >=1 && ac <= 2) {
                o->filename = av[0];
            } else {
                return invalid_args("write <path> [addr]");
            }

            if (ac == 2) {
                uint32_t addr;
                result = get_integer_from_char_array(av[1], &addr);
                if (result != 0) {
                    return bad_arg ("addr");
                } else {
                    o->addr = (stm32_addr_t) addr;
                }
            }
        } else if (o->area == FLASH_OPTION_BYTES_BOOT_ADD) { // expect option bytes boot address
            if (ac != 1) { return invalid_args("option bytes boot_add write <value>"); }

            uint32_t val;
            result = get_integer_from_char_array(av[0], &val);

            if (result != 0) {
                return(bad_arg ("val"));
            } else {
                o->val = (uint32_t)val;
            }
        } else if (o->area == FLASH_OPTCR) { // expect option control register value
            if (ac != 1) { return invalid_args("option control register write <value>"); }
            
            uint32_t val;
            result = get_integer_from_char_array(av[0], &val);
            
            if (result != 0) {
                return bad_arg ("val");
            } else {
                o->val = (uint32_t) val;
            }
        } else if (o->area == FLASH_OPTCR1) { // expect option control register 1 value
            if (ac != 1) { return invalid_args("option control register 1 write <value>"); }
            
            uint32_t val;
            result = get_integer_from_char_array(av[0], &val);
            if (result != 0) {
                return bad_arg ("val");
            } else {
                o->val = (uint32_t) val;
            }
        } else if (o->format == FLASH_FORMAT_BINARY) {    // expect filename and addr
            if (ac != 2) { return invalid_args("write <path> <addr>"); }
            
            o->filename = av[0];
            uint32_t addr;
            result = get_integer_from_char_array(av[1], &addr);

            if (result != 0) {
                return(bad_arg ("addr"));
            } else {
                o->addr = (stm32_addr_t)addr;
            }
        } else if (o->format == FLASH_FORMAT_IHEX) { // expect filename
            if (ac != 1) { return(invalid_args("write <path>")); }

            o->filename = av[0];
        } else {
            return(-1); // should have been caught during format parsing
        }

        break;

    default: break;
    }

    return(0);
}
