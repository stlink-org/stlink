#include <stm32.h>
#include <stlink.h>
#include "chipid.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

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

struct stlink_chipid_params *stlink_chipid_get_params(uint32_t chip_id) {
    struct stlink_chipid_params *params = NULL;
    for (params = devicelist; params != NULL; params = params->next)
        if (params->chip_id == chip_id) {
            fprintf(stderr, "\ndetected chip_id parametres\n\n");
            dump_a_chip(stderr, params);
            break;
        }

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
        } else if (strcmp(word, "ref_manual_id") == 0) {
            // ts->ref_manual_id = strdup (value);
            buf[strlen(p) - 1] = 0; // chomp newline
            sscanf(p, "%*s %n", &nc);
            ts->ref_manual_id = strdup(p + nc);
        } else if (strcmp(word, "chip_id") == 0) {
            if (sscanf(value, "%i", &ts->chip_id) < 1) {
                fprintf(stderr, "Failed to parse chip-id\n");
            }
        } else if (strcmp(word, "flash_type") == 0) {
            if (strcmp(value, "F0_F1_F3") == 0) {
                ts->flash_type = STM32_FLASH_TYPE_F0_F1_F3;
            } else if (strcmp(value, "F1_XL") == 0) {
                ts->flash_type = STM32_FLASH_TYPE_F1_XL;
            } else if (strcmp(value, "F2_F4") == 0) {
                ts->flash_type = STM32_FLASH_TYPE_F2_F4;
            } else if (strcmp(value, "F7") == 0) {
                ts->flash_type = STM32_FLASH_TYPE_F7;
            } else if (strcmp(value, "G0") == 0) {
                ts->flash_type = STM32_FLASH_TYPE_G0;
            } else if (strcmp(value, "G4") == 0) {
                ts->flash_type = STM32_FLASH_TYPE_G4;
            } else if (strcmp(value, "H7") == 0) {
                ts->flash_type = STM32_FLASH_TYPE_H7;
            } else if (strcmp(value, "L0_L1") == 0) {
                ts->flash_type = STM32_FLASH_TYPE_L0_L1;
            } else if (strcmp(value, "L4_L4P") == 0) {
                ts->flash_type = STM32_FLASH_TYPE_L4_L4P;
            } else if (strcmp(value, "L5_U5") == 0) {
                ts->flash_type = STM32_FLASH_TYPE_L5_U5;
            } else if (strcmp(value, "WB_WL") == 0) {
                ts->flash_type = STM32_FLASH_TYPE_WB_WL;
            } else {
                ts->flash_type = STM32_FLASH_TYPE_UNKNOWN;
                fprintf(stderr, "Failed to parse flash type or unrecognized flash type\n");
            }
        } else if (strcmp(word, "flash_size_reg") == 0) {
            if (sscanf(value, "%i", &ts->flash_size_reg) < 1) {
                fprintf(stderr, "Failed to parse flash size reg\n");
            }
        } else if (strcmp(word, "flash_pagesize") == 0) {
            if (sscanf(value, "%i", &ts->flash_pagesize) < 1) {
                fprintf(stderr, "Failed to parse flash page size\n");
            }
        } else if (strcmp(word, "sram_size") == 0) {
            if (sscanf(value, "%i", &ts->sram_size) < 1) {
                fprintf(stderr, "Failed to parse SRAM size\n");
            }
        } else if (strcmp(word, "bootrom_base") == 0) {
            if (sscanf(value, "%i", &ts->bootrom_base) < 1) {
                fprintf(stderr, "Failed to parse BootROM base\n");
            }
        } else if (strcmp(word, "bootrom_size") == 0) {
            if (sscanf(value, "%i", &ts->bootrom_size) < 1) {
                fprintf(stderr, "Failed to parse BootROM size\n");
            }
        } else if (strcmp(word, "option_base") == 0) {
            if (sscanf(value, "%i", &ts->option_base) < 1) {
                fprintf(stderr, "Failed to parse option base\n");
            }
        } else if (strcmp(word, "option_size") == 0) {
            if (sscanf(value, "%i", &ts->option_size) < 1) {
                fprintf(stderr, "Failed to parse option size\n");
            }
        } else if (strcmp(word, "flags") == 0) {
            pp = strtok (p, " \t\n");

            while ((pp = strtok (NULL, " \t\n"))) {
                if (strcmp(pp, "none") == 0) {
                    // NOP
                } else if (strcmp(pp, "dualbank") == 0) {
                    ts->flags |= CHIP_F_HAS_DUAL_BANK;
                } else if (strcmp(pp, "swo") == 0) {
                    ts->flags |= CHIP_F_HAS_SWO_TRACING;
                } else {
                    fprintf(stderr, "Unknown flags word in %s: '%s'\n",
                             fname, pp);
                }
            }

            sscanf(value, "%x", &ts->flags);
        } else {
            fprintf(stderr, "Unknown keyword in %s: %s\n",
                     fname, word);
        }
    }

    ts->next = devicelist;
    devicelist = ts;
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
        return;
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
