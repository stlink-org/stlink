/*
 * File: tests/logging.c
 *
 * Checks the log handler interface without any hardware.
 */


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stlink.h>
#include <logging.h>

#include "libusb_settings.h"


#define MAX_ENTRIES 16

struct entry {
    int32_t level;
    char tag[32];
    char *message;
};

static struct entry entries[MAX_ENTRIES];
static int32_t entry_count = 0;
static int32_t libusb_count = 0;
static int32_t libusb_bad = 0;
static int32_t failures = 0;

#define CHECK(cond) do { if(!(cond)) { fprintf(stderr, "FAILED line %d: %s\n", __LINE__, #cond); failures++; } } while (0)

static void handler(void *user, int32_t level, const char *tag, const char *message) {
    CHECK(user == (void *) &entries);

    if(strcmp(tag, "libusb") == 0) {
        // messages of libusb are counted, their text depends on the libusb build
        size_t len = strlen(message);
        libusb_count++;
        if(len > 0 && message[len - 1] == '\n') { libusb_bad++; }
        return;
    }

    if(entry_count < MAX_ENTRIES) {
        entries[entry_count].level = level;
        snprintf(entries[entry_count].tag, sizeof(entries[entry_count].tag), "%s", tag);
        entries[entry_count].message = strdup(message);
    }

    entry_count++;
}

static void reset_entries(void) {
    for(int32_t i = 0; i < entry_count && i < MAX_ENTRIES; i++) { free(entries[i].message); }
    entry_count = 0;
}

int32_t main(void) {
    stlink_log_set_handler(handler, &entries);

    // level filter and fields
    stlink_log_set_level(STLINK_LOG_INFO);
    CHECK(stlink_log_get_level() == STLINK_LOG_INFO);
    DLOG("debug %d\n", 1);
    ILOG("info %d\n", 2);
    WLOG("warn %d\n", 3);
    ELOG("error %d\n", 4);
    CHECK(entry_count == 3);
    CHECK(entries[0].level == STLINK_LOG_INFO && strcmp(entries[0].message, "info 2") == 0);
    CHECK(entries[1].level == STLINK_LOG_WARN && strcmp(entries[1].message, "warn 3") == 0);
    CHECK(entries[2].level == STLINK_LOG_ERROR && strcmp(entries[2].message, "error 4") == 0);
    CHECK(strcmp(entries[0].tag, "logging.c") == 0);
    reset_entries();

    // only one trailing line break is removed, messages without one stay unchanged
    ILOG("no line break");
    ILOG("windows\r\n");
    ILOG("two\n\n");
    CHECK(entry_count == 3);
    CHECK(strcmp(entries[0].message, "no line break") == 0);
    CHECK(strcmp(entries[1].message, "windows") == 0);
    CHECK(strcmp(entries[2].message, "two\n") == 0);
    reset_entries();

    // long messages are not truncated
    char long_text[3000];
    memset(long_text, 'x', sizeof(long_text) - 1);
    long_text[sizeof(long_text) - 1] = '\0';
    ILOG("%s\n", long_text);
    CHECK(entry_count == 1);
    CHECK(entries[0].message != NULL && strcmp(entries[0].message, long_text) == 0);
    reset_entries();

    // the open functions don't override a level set by the application ...
    stlink_log_open_level(STLINK_LOG_DEBUG);
    CHECK(stlink_log_get_level() == STLINK_LOG_INFO);
    // ... but the legacy ugly_init() still sets it
    ugly_init(STLINK_LOG_WARN);
    CHECK(stlink_log_get_level() == STLINK_LOG_WARN);
    stlink_log_open_level(STLINK_LOG_DEBUG);
    CHECK(stlink_log_get_level() == STLINK_LOG_WARN);

    // ELOG_ALWAYS ignores the level
    stlink_log_set_level(0);
    ELOG("dropped\n");
    ELOG_ALWAYS("shown %s\n", "anyway");
    CHECK(entry_count == 1);
    CHECK(entries[0].level == STLINK_LOG_ERROR && strcmp(entries[0].message, "shown anyway") == 0);
    reset_entries();

    // libusb messages arrive with tag "libusb" and without line break (if libusb logs at all)
    stlink_log_set_level(STLINK_LOG_DEBUG);
    libusb_context *ctx = NULL;
    if(libusb_init(&ctx) == 0) {
        libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
        libusb_device **list = NULL;
        ssize_t cnt = libusb_get_device_list(ctx, &list);
        if(cnt >= 0) { libusb_free_device_list(list, 1); }
        libusb_exit(ctx);
    }
    CHECK(libusb_bad == 0);
    printf("libusb messages received: %d\n", libusb_count);

    // without a handler: one line on stderr in the established format
    stlink_log_set_handler(NULL, NULL);
    stlink_log_set_level(STLINK_LOG_INFO);
    const char *stderr_file = "test-logging-stderr.txt";
    if(freopen(stderr_file, "w", stderr) == NULL) { printf("cannot redirect stderr\n"); return (1); }
    WLOG("to stderr %d\n", 5);
    DLOG("filtered\n");
    fflush(stderr);
    fclose(stderr);

    FILE *f = fopen(stderr_file, "r");
    char line[256] = "";
    char extra[256] = "";
    int32_t lines = 0;
    if(f != NULL) {
        if(fgets(line, sizeof(line), f) != NULL) { lines++; }
        if(fgets(extra, sizeof(extra), f) != NULL) { lines++; }
        fclose(f);
    }
    remove(stderr_file);

    int32_t year, mon, day, hour, min, sec;
    char rest[128] = "";
    int32_t n = sscanf(line, "%4d-%2d-%2dT%2d:%2d:%2d %127[^\n]", &year, &mon, &day, &hour, &min, &sec, rest);
    if(lines != 1 || n != 7 || strcmp(rest, "WARN logging.c: to stderr 5") != 0) {
        printf("FAILED: stderr output '%s' (%d lines)\n", line, lines);
        failures++;
    }

    printf("%s\n", failures ? "test-logging FAILED" : "test-logging passed");
    return (failures ? 1 : 0);
}
