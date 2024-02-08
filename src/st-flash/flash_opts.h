/*
 * File: flash_opts.h
 *
 * Flash Options
 */

#ifndef FLASH_OPTS_H
#define FLASH_OPTS_H

#define FLASH_OPTS_INITIALIZER {0, { 0 }, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

enum flash_cmd {FLASH_CMD_NONE = 0, FLASH_CMD_WRITE = 1, FLASH_CMD_READ = 2, FLASH_CMD_ERASE = 3, CMD_RESET = 4};
enum flash_format {FLASH_FORMAT_BINARY = 0, FLASH_FORMAT_IHEX = 1};
enum flash_area {FLASH_MAIN_MEMORY = 0, FLASH_SYSTEM_MEMORY = 1, FLASH_OTP = 2, FLASH_OPTION_BYTES = 3, FLASH_OPTION_BYTES_BOOT_ADD = 4, FLASH_OPTCR = 5, FLASH_OPTCR1 = 6};

struct flash_opts {
    enum flash_cmd cmd;
    uint8_t serial[STLINK_SERIAL_BUFFER_SIZE];
    const char* filename;
    stm32_addr_t addr;
    uint32_t size;
    int32_t reset;
    int32_t log_level;
    enum flash_format format;
    enum flash_area area;
    uint32_t val;
    uint32_t flash_size;  // --flash=n[k, M]
    int32_t opt;          // enable empty tail data drop optimization
    int32_t freq;         // --freq=n[k, M] frequency of JTAG/SWD
    enum connect_type connect;
};

// static bool starts_with(const char * str, const char * prefix);
// static int32_t get_long_integer_from_char_array (const char *const str, uint64_t *read_value);
// static int32_t get_integer_from_char_array (const char *const str, uint32_t *read_value);
// static int32_t invalid_args(const char *expected);
// static int32_t bad_arg(const char *arg);
int32_t flash_get_opts(struct flash_opts* o, int32_t ac, char** av);

#endif // FLASH_OPTS_H
