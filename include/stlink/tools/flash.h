#ifndef STLINK_TOOLS_FLASH_H_
#define STLINK_TOOLS_FLASH_H_

#include <stdint.h>
#include <stlink.h>

#define DEBUG_LOG_LEVEL 100
#define STND_LOG_LEVEL  50

enum flash_cmd {FLASH_CMD_NONE = 0, FLASH_CMD_WRITE = 1, FLASH_CMD_READ = 2, FLASH_CMD_ERASE = 3, CMD_RESET = 4};
enum flash_format {FLASH_FORMAT_BINARY = 0, FLASH_FORMAT_IHEX = 1};
struct flash_opts
{
    enum flash_cmd cmd;
    const char* devname;
    uint8_t serial[16];
    const char* filename;
    stm32_addr_t addr;
    size_t size;
    int reset;
    int log_level;
    enum flash_format format;
    size_t flash_size;	/* --flash=n[k][m] */
};

#define FLASH_OPTS_INITIALIZER {0, NULL, { 0 }, NULL, 0, 0, 0, 0, 0, 0 }

int flash_get_opts(struct flash_opts* o, int ac, char** av);

#endif // STLINK_FLASH_H_

