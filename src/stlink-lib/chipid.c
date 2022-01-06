#include <stlink.h>
#include "chipid.h"
#include "chipid_db_old.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>


struct stlink_chipid_params *stlink_chipid_get_params_old(uint32_t chipid) {
    struct stlink_chipid_params *params = NULL;

    for (size_t n = 0; n < STLINK_ARRAY_SIZE(devices); n++)
        if (devices[n].chip_id == chipid) {
            params = &devices[n];
            break;
        }

    return (params);
}

static struct stlink_chipid_params *devicelist;

void dump_a_chip (FILE *fp, struct stlink_chipid_params *dev) {
    fprintf(fp, "# Device Type: %s\n", dev->dev_type);
    fprintf(fp, "# Reference Manual: RM%s\n", dev->ref_manual_id);
    fprintf(fp, "#\n");
    fprintf(fp, "chip_id 0x%x\n", dev->chip_id);
    fprintf(fp, "flash_type %d\n", dev->flash_type);
    fprintf(fp, "flash_size_reg 0x%x\n", dev->flash_size_reg);
    fprintf(fp, "flash_pagesize 0x%x\n", dev->flash_pagesize);
    fprintf(fp, "sram_size 0x%x\n", dev->sram_size);
    fprintf(fp, "bootrom_base 0x%x\n", dev->bootrom_base);
    fprintf(fp, "bootrom_size 0x%x\n", dev->bootrom_size);
    fprintf(fp, "option_base 0x%x\n", dev->option_base);
    fprintf(fp, "option_size 0x%x\n", dev->option_size);
    fprintf(fp, "flags %d\n\n", dev->flags);
}

static int chipid_params_eq(const struct stlink_chipid_params *p1, const struct stlink_chipid_params *p2)
{
    return p1->chip_id == p2->chip_id &&
        p1->dev_type && p2->dev_type &&
        strcmp(p1->dev_type, p2->dev_type) == 0 &&
        p1->flash_type == p2->flash_type &&
        p1->flash_size_reg == p2->flash_size_reg &&
        p1->flash_pagesize == p2->flash_pagesize &&
        p1->sram_size == p2->sram_size &&
        p1->bootrom_base == p2->bootrom_base &&
        p1->bootrom_size == p2->bootrom_size &&
        p1->option_base == p2->option_base &&
        p1->option_size == p2->option_size &&
        p1->flags == p2->flags;
}

struct stlink_chipid_params *stlink_chipid_get_params(uint32_t chipid) {
    struct stlink_chipid_params *params = NULL;
    struct stlink_chipid_params *p2;

    for (params = devicelist; params != NULL; params = params->next)
        if (params->chip_id == chipid) {
            break;
        }

    p2 = stlink_chipid_get_params_old(chipid);

#if 1
    if (params == NULL) {
        params = p2;
    } else if (!chipid_params_eq(params, p2)) {
        // fprintf (stderr, "Error, chipid params not identical\n");
        // return NULL;
        fprintf(stderr, "---------- old ------------\n");
        dump_a_chip(stderr, p2);
        fprintf(stderr, "---------- new ------------\n");
        dump_a_chip(stderr, params);
    }
#endif
    return(params);
}

void process_chipfile(char *fname) {
    FILE *fp;
    char *p, *pp, buf[1025];
    char word[64], value[64];
    struct stlink_chipid_params *ts;
    int nc;

    // fprintf (stderr, "processing chip-id file %s.\n", fname);
    fp = fopen(fname, "r");

    if (!fp) {
        perror(fname);
        return;
    }

    ts = calloc(sizeof(struct stlink_chipid_params), 1);

    while (fgets(buf, 1024, fp) != NULL) {
        for (p = buf; isspace (*p); p++);

        if (!*p) {
            continue; // we hit end-of-line with only whitespace
        }

        if (*p == '#') {
            continue; // ignore comments
        }

        sscanf(p, "%s %s", word, value);

        if (strcmp (word, "dev_type") == 0) {
            // ts->dev_type = strdup (value);
            buf[strlen(p) - 1] = 0; // chomp newline
            sscanf(p, "%*s %n", &nc);
            ts->dev_type = strdup(p + nc);
        } else if (strcmp (word, "ref_manual_id") == 0) {
            // ts->ref_manual_id = strdup (value);
            buf[strlen(p) - 1] = 0; // chomp newline
            sscanf(p, "%*s %n", &nc);
            ts->ref_manual_id = strdup(p + nc);
        } else if (strcmp(word, "chip_id") == 0) {
            if (sscanf(value, "%i", &ts->chip_id) < 1) {
                fprintf(stderr, "Failed to parse chip-id\n");
            }
        } else if (strcmp (word, "flash_type") == 0) {
            if (sscanf(value, "%i", (int *)&ts->flash_type) < 1) {
                fprintf(stderr, "Failed to parse flash type\n");
            } else if ((ts->flash_type < STLINK_FLASH_TYPE_UNKNOWN) || (ts->flash_type >= STLINK_FLASH_TYPE_MAX)) {
                fprintf(stderr, "Unrecognized flash type\n");
            }
        } else if (strcmp (word, "flash_size_reg") == 0) {
            if (sscanf(value, "%i", &ts->flash_size_reg) < 1) {
                fprintf(stderr, "Failed to parse flash size reg\n");
            }
        } else if (strcmp (word, "flash_pagesize") == 0) {
            if (sscanf(value, "%i", &ts->flash_pagesize) < 1) {
                fprintf(stderr, "Failed to parse flash page size\n");
            }
        } else if (strcmp (word, "sram_size") == 0) {
            if (sscanf(value, "%i", &ts->sram_size) < 1) {
                fprintf(stderr, "Failed to parse SRAM size\n");
            }
        } else if (strcmp (word, "bootrom_base") == 0) {
            if (sscanf(value, "%i", &ts->bootrom_base) < 1) {
                fprintf(stderr, "Failed to parse BootROM base\n");
            }
        } else if (strcmp (word, "bootrom_size") == 0) {
            if (sscanf(value, "%i", &ts->bootrom_size) < 1) {
                fprintf(stderr, "Failed to parse BootROM size\n");
            }
        } else if (strcmp (word, "option_base") == 0) {
            if (sscanf(value, "%i", &ts->option_base) < 1) {
                fprintf(stderr, "Failed to parse option base\n");
            }
        } else if (strcmp (word, "option_size") == 0) {
            if (sscanf(value, "%i", &ts->option_size) < 1) {
                fprintf(stderr, "Failed to parse option size\n");
            }
        } else if (strcmp (word, "flags") == 0) {
            pp = strtok (p, " \t\n");

            while ((pp = strtok (NULL, " \t\n"))) {
                if (strcmp (pp, "none") == 0) {
                    ts->flags = 0;               // not necessary: calloc did this already.
                } else if (strcmp (pp, "dualbank") == 0) {
                    ts->flags |= CHIP_F_HAS_DUAL_BANK;
                } else if (strcmp (pp, "swo") == 0) {
                    ts->flags |= CHIP_F_HAS_SWO_TRACING;
                } else {
                    fprintf (stderr, "Unknown flags word in %s: '%s'\n",
                             fname, pp);
                }
            }

            sscanf(value, "%x", &ts->flags);
        } else {
            fprintf (stderr, "Unknown keyword in %s: %s\n",
                     fname, word);
        }
    }

    ts->next = devicelist;
    devicelist = ts;
}

void dump_chips (void) {
    struct stlink_chipid_params *ts;
    char *p, buf[100];
    FILE *fp;

    for (size_t n = 0; n < STLINK_ARRAY_SIZE(devices); n++) {
        ts = &devices[n];

        strcpy(buf, ts->dev_type);

        while ((p = strchr(buf, '/'))) // change slashes to underscore.
            *p = '_';

        strcat(buf, ".chip");
        fp = fopen(buf, "w");
        fprintf(fp, "# Device Type: %s\n", ts->dev_type);
        fprintf(fp, "# Reference Manual: RM%s\n", ts->ref_manual_id);
        fprintf(fp, "#\n");
        fprintf(fp, "chip_id %x\n", ts->chip_id);
        fprintf(fp, "flash_type %x\n", ts->flash_type);
        fprintf(fp, "flash_size_reg %x\n", ts->flash_size_reg);
        fprintf(fp, "flash_pagesize %x\n", ts->flash_pagesize);
        fprintf(fp, "sram_size %x\n", ts->sram_size);
        fprintf(fp, "bootrom_base %x\n", ts->bootrom_base);
        fprintf(fp, "bootrom_size %x\n", ts->bootrom_size);
        fprintf(fp, "option_base %x\n", ts->option_base);
        fprintf(fp, "option_size %x\n", ts->option_size);
        fprintf(fp, "flags %x\n\n", ts->flags);
        fclose(fp);
    }
}

#if defined(STLINK_HAVE_DIRENT_H)
#include <dirent.h>
void init_chipids(char *dir_to_scan) {
    DIR *d;
    size_t nl; // namelen
    struct dirent *dir;

    if (!dir_to_scan) {
        dir_to_scan = "./";
    }

    devicelist = NULL;
    // dump_chips ();
    d = opendir(dir_to_scan);

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            nl = strlen(dir->d_name);

            if (strcmp(dir->d_name + nl - 5, ".chip") == 0) {
                char buf[1024];
                sprintf(buf, "%s/%s", dir_to_scan, dir->d_name);
                process_chipfile(buf);
            }
        }

        closedir(d);
    } else {
        perror (dir_to_scan);
        return; // XXX
    }
}
#endif //STLINK_HAVE_DIRENT_H

#if defined(_WIN32) && !defined(STLINK_HAVE_DIRENT_H)
#include <fileapi.h>
#include <strsafe.h>
void init_chipids(char *dir_to_scan) {
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA ffd;
    char filepath[MAX_PATH] = {0};
    StringCchCopyA(filepath, STLINK_ARRAY_SIZE(filepath), dir_to_scan);

    if (FAILED(StringCchCatA(filepath, STLINK_ARRAY_SIZE(filepath), "\\*.chip"))) {
        ELOG("Path to chips's dir too long.\n");
        return;
    }

    hFind = FindFirstFileA(filepath, &ffd);

    if (INVALID_HANDLE_VALUE == hFind) {
        ELOG("Can't find any chip description file in %s.\n", filepath);
        return;
    }

    do {
        memset(filepath, 0, STLINK_ARRAY_SIZE(filepath));
        StringCchCopyA(filepath, STLINK_ARRAY_SIZE(filepath), dir_to_scan);
        StringCchCatA(filepath, STLINK_ARRAY_SIZE(filepath), "\\");
        StringCchCatA(filepath, STLINK_ARRAY_SIZE(filepath), ffd.cFileName);
        process_chipfile(filepath);
    } while (FindNextFileA(hFind, &ffd) != 0);

    FindClose(hFind);
}
#endif //defined(_WIN32) && !defined(STLINK_HAVE_DIRENT_H)
