#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stlink.h>
#include <stlink/tools/flash.h>
#if defined(_MSC_VER)
#include <malloc.h>
#endif

struct Test {
    const char * cmd_line;
    int res;
    struct flash_opts opts;
};

static bool cmp_strings(const char * s1, const char * s2) {
    if(s1 == NULL || s2 == NULL) return (s1 == s2);
    else return (0 == strcmp(s1, s2));
}

static bool cmp_mem(const uint8_t * s1, const uint8_t * s2, size_t size) {
    if(s1 == NULL || s2 == NULL) return (s1 == s2);
    else return (0 == memcmp(s1, s2, size));
}

static bool execute_test(const struct Test * test) {
    int ac = 0;
    char* av[32];

    // parse (tokenize) the test command line
#if defined(_MSC_VER)
    char *cmd_line = alloca(strlen(test->cmd_line));
#else
    char cmd_line[strlen(test->cmd_line)];
#endif
    strcpy(cmd_line, test->cmd_line);

    for(char * tok = strtok(cmd_line, " "); tok; tok = strtok(NULL, " ")) {
        if((size_t)ac >= sizeof(av)/sizeof(av[0])) return false;

        av[ac] = tok;
        ++ac;
    }

    // call
    struct flash_opts opts;
    int res = flash_get_opts(&opts, ac, av);

    // compare results
    bool ret = (res == test->res);

    if(ret && (res == 0)) {
        ret &= (opts.cmd == test->opts.cmd);
        ret &= cmp_strings(opts.devname, test->opts.devname);
        ret &= cmp_mem(opts.serial, test->opts.serial, sizeof(opts.serial));
        ret &= cmp_strings(opts.filename, test->opts.filename);
        ret &= (opts.addr == test->opts.addr);
        ret &= (opts.size == test->opts.size);
        ret &= (opts.reset == test->opts.reset);
        ret &= (opts.log_level == test->opts.log_level);
        ret &= (opts.format == test->opts.format);
    }

    printf("[%s] (%d) %s\n", ret ? "OK" : "ERROR", res, test->cmd_line);
    return ret;
}

static struct Test tests[] = {
    { "", -1, FLASH_OPTS_INITIALIZER },
    { "--debug --reset read /dev/sg0 test.bin 0x80000000 0x1000", 0,
        { .cmd = FLASH_CMD_READ, .devname = "/dev/sg0", .serial = { 0 }, .filename = "test.bin",
          .addr = 0x80000000, .size = 0x1000, .reset = 1, .log_level = DEBUG_LOG_LEVEL, .format = FLASH_FORMAT_BINARY } },
    { "--debug --reset write /dev/sg0 test.bin 0x80000000", 0,
        { .cmd = FLASH_CMD_WRITE, .devname = "/dev/sg0", .serial = { 0 }, .filename = "test.bin",
          .addr = 0x80000000, .size = 0, .reset = 1, .log_level = DEBUG_LOG_LEVEL, .format = FLASH_FORMAT_BINARY } },
    { "--serial A1020304 /dev/sg0 erase", -1, FLASH_OPTS_INITIALIZER },
    { "/dev/sg0 erase", 0,
        { .cmd = FLASH_CMD_ERASE, .devname = "/dev/sg0", .serial = { 0 }, .filename = NULL,
          .addr = 0, .size = 0, .reset = 0, .log_level = STND_LOG_LEVEL, .format = FLASH_FORMAT_BINARY } },
    { "--debug --reset read test.bin 0x80000000 0x1000", 0,
        { .cmd = FLASH_CMD_READ, .devname = NULL, .serial = { 0 }, .filename = "test.bin",
          .addr = 0x80000000, .size = 0x1000, .reset = 1, .log_level = DEBUG_LOG_LEVEL, .format = FLASH_FORMAT_BINARY } },
    { "--debug --reset write test.bin 0x80000000", 0,
        { .cmd = FLASH_CMD_WRITE, .devname = NULL, .serial = { 0 }, .filename = "test.bin",
          .addr = 0x80000000, .size = 0, .reset = 1, .log_level = DEBUG_LOG_LEVEL, .format = FLASH_FORMAT_BINARY } },
    { "erase", 0,
        { .cmd = FLASH_CMD_ERASE, .devname = NULL, .serial = { 0 }, .filename = NULL,
          .addr = 0, .size = 0, .reset = 0, .log_level = STND_LOG_LEVEL, .format = FLASH_FORMAT_BINARY } },
    { "--debug --reset --format=ihex write test.hex", 0,
        { .cmd = FLASH_CMD_WRITE, .devname = NULL, .serial = { 0 }, .filename = "test.hex",
          .addr = 0, .size = 0, .reset = 1, .log_level = DEBUG_LOG_LEVEL, .format = FLASH_FORMAT_IHEX } },
    { "--debug --reset --format=binary write test.hex", -1, FLASH_OPTS_INITIALIZER },
    { "--debug --reset --format=ihex write test.hex 0x80000000", -1, FLASH_OPTS_INITIALIZER },
    { "--debug --reset write test.hex sometext", -1, FLASH_OPTS_INITIALIZER },
    { "--serial A1020304 erase", 0,
        { .cmd = FLASH_CMD_ERASE, .devname = NULL, .serial = "\0\0\0\0\0\0\0\0\xA1\x02\x03\x04", .filename = NULL,
          .addr = 0, .size = 0, .reset = 0, .log_level = STND_LOG_LEVEL, .format = FLASH_FORMAT_BINARY } },
    { "--serial=A1020304 erase", 0,
        { .cmd = FLASH_CMD_ERASE, .devname = NULL, .serial = "\0\0\0\0\0\0\0\0\xA1\x02\x03\x04", .filename = NULL,
          .addr = 0, .size = 0, .reset = 0, .log_level = STND_LOG_LEVEL, .format = FLASH_FORMAT_BINARY } },
};

int main()
{
    bool allOk = true;
    for(size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
        if(!execute_test(&tests[i])) allOk = false;
    }

    return (allOk ? 0 : 1);
}

