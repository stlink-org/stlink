/*
 * File: tests/progress.c
 *
 * Checks the progress reporting without any hardware: the events passed to a
 * handler and the default output on stdout, which has to stay unchanged.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stlink.h>
#include <map_file.h>
#include <md5.h>
#include <progress.h>

#define MAX_EVENTS 16

static struct stlink_progress events[MAX_EVENTS];
static char event_path[256];
static int32_t event_count = 0;
static int32_t failures = 0;

#define CHECK(cond) do { if(!(cond)) { fprintf(stderr, "FAILED line %d: %s\n", __LINE__, #cond); failures++; } } while (0)

static void handler(stlink_t *sl, void *user, const struct stlink_progress *p) {
    CHECK(sl != NULL && user == (void *) sl);

    if(event_count < MAX_EVENTS) {
        events[event_count] = *p;

        if(p->path != NULL) {
            snprintf(event_path, sizeof(event_path), "%s", p->path);
            events[event_count].path = event_path;
        }
    }

    event_count++;
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if(f == NULL) { return (NULL); }
    char *buf = calloc(1, 4096);
    size_t len = fread(buf, 1, 4095, f);
    buf[len] = '\0';
    fclose(f);
    return (buf);
}

int32_t main(void) {
    stlink_t *sl = calloc(1, sizeof(stlink_t));
    const char *data_file = "test-progress-data.bin";
    const char *stdout_file = "test-progress-stdout.txt";

    FILE *f = fopen(data_file, "wb");
    if(f == NULL || fwrite("abc", 1, 3, f) != 3) { fprintf(stderr, "cannot write %s\n", data_file); return (1); }
    fclose(f);

    mapped_file_t mf = MAPPED_FILE_INITIALIZER;
    if(map_file(&mf, data_file) == -1) { fprintf(stderr, "cannot map %s\n", data_file); return (1); }

    // md5 digest and checksum of "abc"
    static const uint8_t abc_md5[16] = {0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
                                        0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72};
    uint8_t digest[16];
    md5_calculate_digest(&mf, digest);
    CHECK(memcmp(digest, abc_md5, sizeof(digest)) == 0);
    CHECK(stlink_checksum_calculate(&mf) == 0x126);

    // with a handler: all events arrive, also those printed only with sl->verbose >= 1
    sl->verbose = 0;
    stlink_set_progress_handler(sl, handler, sl);
    stlink_progress_file(sl, data_file, &mf);
    stlink_progress_event(sl, STLINK_PROGRESS_MASS_ERASE_START, false);
    stlink_progress_page_erased(sl, 0x08000000, 0x800);
    stlink_progress_write(sl, STLINK_PROGRESS_UNIT_BYTES, 64, 128, true);
    stlink_progress_event(sl, STLINK_PROGRESS_WRITE_DONE, true);
    CHECK(event_count == 5);
    CHECK(events[0].event == STLINK_PROGRESS_FILE && strcmp(events[0].path, data_file) == 0);
    CHECK(events[0].size == 3 && events[0].checksum == 0x126);
    CHECK(memcmp(events[0].md5, abc_md5, sizeof(abc_md5)) == 0);
    CHECK(events[1].event == STLINK_PROGRESS_MASS_ERASE_START);
    CHECK(events[2].event == STLINK_PROGRESS_PAGE_ERASED && events[2].addr == 0x08000000 && events[2].size == 0x800);
    CHECK(events[3].event == STLINK_PROGRESS_WRITE && events[3].unit == STLINK_PROGRESS_UNIT_BYTES);
    CHECK(events[3].done == 64 && events[3].total == 128);
    CHECK(events[4].event == STLINK_PROGRESS_WRITE_DONE);

    // without a handler: the output on stdout as in previous versions
    stlink_set_progress_handler(sl, NULL, NULL);
    if(freopen(stdout_file, "w", stdout) == NULL) { fprintf(stderr, "cannot redirect stdout\n"); return (1); }

    // reference: what stlink_fwrite_flash() printed before
    printf("file %s ", data_file);
    md5_calculate(&mf);
    stlink_checksum(&mf);
    printf("--\n");

    sl->verbose = 1;
    stlink_progress_file(sl, data_file, &mf);
    stlink_progress_event(sl, STLINK_PROGRESS_MASS_ERASE_START, false);
    stlink_progress_event(sl, STLINK_PROGRESS_MASS_ERASE_TICK, false);
    stlink_progress_event(sl, STLINK_PROGRESS_MASS_ERASE_TICK, false);
    stlink_progress_event(sl, STLINK_PROGRESS_MASS_ERASE_DONE, false);
    stlink_progress_page_erased(sl, 0x08000000, 0x800);
    stlink_progress_event(sl, STLINK_PROGRESS_ERASE_DONE, false);
    stlink_progress_write(sl, STLINK_PROGRESS_UNIT_PAGES, 2, 16, false);
    stlink_progress_write(sl, STLINK_PROGRESS_UNIT_HALFPAGES, 3, 32, true);
    stlink_progress_write(sl, STLINK_PROGRESS_UNIT_BYTES, 64, 128, true);
    stlink_progress_event(sl, STLINK_PROGRESS_WRITE_DONE, true);

    // sl->verbose = 0: only the progress shown regardless of verbose
    sl->verbose = 0;
    printf("--\n");
    stlink_progress_write(sl, STLINK_PROGRESS_UNIT_PAGES, 1, 4, true);
    stlink_progress_event(sl, STLINK_PROGRESS_WRITE_DONE, true);
    stlink_progress_write(sl, STLINK_PROGRESS_UNIT_PAGES, 1, 4, false);
    stlink_progress_event(sl, STLINK_PROGRESS_WRITE_DONE, false);
    fflush(stdout);
    fclose(stdout);

    char expected[1024];
    snprintf(expected, sizeof(expected),
             "file %s md5 checksum: 90150983cd24fb0d6963f7d28e17f72, stlink checksum: 0x00000126\n"
             "--\n"
             "file %s md5 checksum: 90150983cd24fb0d6963f7d28e17f72, stlink checksum: 0x00000126\n"
             "Mass erasing.....\n"
             "-> Flash page at 0x8000000 erased (size: 0x800)\n"
             "\n"
             "  2/16  pages written\n"
             "  3/ 32 halfpages written\n"
             "64/128 bytes written\n"
             "\n"
             "--\n"
             "  1/4   pages written\n"
             "\n",
             data_file, data_file);

    char *output = read_file(stdout_file);
    if(output == NULL || strcmp(output, expected) != 0) {
        fprintf(stderr, "FAILED: stdout output\n--- expected ---\n%s--- got ---\n%s", expected, output ? output : "(none)");
        failures++;
    }

    free(output);
    unmap_file(&mf);
    remove(stdout_file);
    remove(data_file);
    free(sl);

    fprintf(stderr, "%s\n", failures ? "test-progress FAILED" : "test-progress passed");
    return (failures ? 1 : 0);
}
