#ifndef STLINK_TOOLS_FLASH_H_
#define STLINK_TOOLS_FLASH_H_

#include <stdint.h>

#include <stlink.h>

#define DEBUG_LOG_LEVEL 100
#define STND_LOG_LEVEL  50
#define ENABLE_OPT      1

enum flash_cmd {FLASH_CMD_NONE = 0, FLASH_CMD_WRITE = 1, FLASH_CMD_READ = 2, FLASH_CMD_ERASE = 3, CMD_RESET = 4};
enum flash_format {FLASH_FORMAT_BINARY = 0, FLASH_FORMAT_IHEX = 1};
enum flash_area {FLASH_MAIN_MEMORY = 0, FLASH_SYSTEM_MEMORY = 1, FLASH_OTP = 2, FLASH_OPTION_BYTES = 3, FLASH_OPTION_BYTES_BOOT_ADD = 4, FLASH_OPTCR = 5, FLASH_OPTCR1 = 6};
struct flash_opts {
    enum flash_cmd cmd;
    uint8_t serial[STLINK_SERIAL_MAX_SIZE];
    const char* filename;
    stm32_addr_t addr;
    size_t size;
    int reset;
    int log_level;
    enum flash_format format;
    enum flash_area area;
    uint32_t val;
    size_t flash_size;  // --flash=n[k][m]
    int opt;            // enable empty tail data drop optimization
    int freq;           // --freq=n[k][m] frequency of JTAG/SWD
    bool connect_under_reset;
};

#define FLASH_OPTS_INITIALIZER {0, { 0 }, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

int flash_get_opts(struct flash_opts* o, int ac, char** av);

#endif // STLINK_FLASH_H_
