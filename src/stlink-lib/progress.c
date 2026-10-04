/**
  ******************************************************************************
  * @file           : progress.c
  * @brief          : Progress reporting of longer operations
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-10-04
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#include <stdio.h>
#include <string.h>

#include <stlink.h>
#include "progress.h"

#include "md5.h"


void stlink_set_progress_handler(stlink_t *sl, stlink_progress_handler_t handler, void *user) {
    sl->progress_user = user;
    sl->progress_handler = handler;
}

/* Output without a handler, unchanged from previous versions */
static void print_progress(const struct stlink_progress *p) {
    switch (p->event) {
    case STLINK_PROGRESS_FILE:
        printf("file %s md5 checksum: ", p->path);

        for(size_t i = 0; i < sizeof(p->md5); i++) {
            printf("%x", p->md5[i]);
        }

        printf(", stlink checksum: 0x%08x\n", p->checksum);
        break;
    case STLINK_PROGRESS_MASS_ERASE_START:
        fprintf(stdout, "Mass erasing...");
        fflush(stdout);
        break;
    case STLINK_PROGRESS_MASS_ERASE_TICK:
        fprintf(stdout, ".");
        fflush(stdout);
        break;
    case STLINK_PROGRESS_PAGE_ERASED:
        fprintf(stdout, "-> Flash page at %#x erased (size: %#x)\n", p->addr, p->size);
        fflush(stdout);
        break;
    case STLINK_PROGRESS_WRITE:
        switch (p->unit) {
        case STLINK_PROGRESS_UNIT_HALFPAGES:
            fprintf(stdout, "%3u/%3u halfpages written\n", p->done, p->total);
            break;
        case STLINK_PROGRESS_UNIT_BYTES:
            fprintf(stdout, "%u/%u bytes written\n", p->done, p->total);
            break;
        case STLINK_PROGRESS_UNIT_PAGES:
        default:
            fprintf(stdout, "%3u/%-3u pages written\n", p->done, p->total);
            break;
        }

        fflush(stdout);
        break;
    case STLINK_PROGRESS_MASS_ERASE_DONE:
    case STLINK_PROGRESS_ERASE_DONE:
    case STLINK_PROGRESS_WRITE_DONE:
        fprintf(stdout, "\n");
        break;
    default:
        break;
    }
}

static void report(stlink_t *sl, const struct stlink_progress *p, bool verbose_only) {
    if(sl->progress_handler != NULL) {
        sl->progress_handler(sl, sl->progress_user, p);
    } else if(!verbose_only || sl->verbose >= 1) {
        print_progress(p);
    }
}

void stlink_progress_event(stlink_t *sl, enum stlink_progress_event event, bool verbose_only) {
    struct stlink_progress p;
    memset(&p, 0, sizeof(p));
    p.event = event;
    report(sl, &p, verbose_only);
}

void stlink_progress_write(stlink_t *sl, enum stlink_progress_unit unit, uint32_t done, uint32_t total,
                           bool verbose_only) {
    struct stlink_progress p;
    memset(&p, 0, sizeof(p));
    p.event = STLINK_PROGRESS_WRITE;
    p.unit = unit;
    p.done = done;
    p.total = total;
    report(sl, &p, verbose_only);
}

void stlink_progress_page_erased(stlink_t *sl, uint32_t addr, uint32_t size) {
    struct stlink_progress p;
    memset(&p, 0, sizeof(p));
    p.event = STLINK_PROGRESS_PAGE_ERASED;
    p.addr = addr;
    p.size = size;
    report(sl, &p, false);
}

void stlink_progress_file(stlink_t *sl, const char *path, const mapped_file_t *mf) {
    struct stlink_progress p;
    memset(&p, 0, sizeof(p));
    p.event = STLINK_PROGRESS_FILE;
    p.path = path;
    p.size = mf->len;
    md5_calculate_digest(mf, p.md5);
    p.checksum = stlink_checksum_calculate(mf);
    report(sl, &p, false);
}
