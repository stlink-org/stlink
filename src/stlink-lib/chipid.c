/*
 * File: chipid.c
 *
 * Chip-ID parametres
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stm32.h>
#include <stlink.h>
#include "chipid.h"

#include "logging.h"

// #include <ctype.h> // TODO: Check use
// #include <errno.h> // TODO: Check use

static struct stlink_chipid_params *devicelist;

void dump_a_chip(struct stlink_chipid_params *dev) {
  DLOG("# Device Type: %s\n", dev->dev_type);
  DLOG("# Reference Manual: RM%s\n", dev->ref_manual_id);
  DLOG("#\n");
  DLOG("chip_id 0x%x\n", dev->chip_id);
  DLOG("flash_type %d\n", dev->flash_type);
  DLOG("flash_size_reg 0x%x\n", dev->flash_size_reg);
  DLOG("flash_pagesize 0x%x\n", dev->flash_pagesize);
  DLOG("sram_size 0x%x\n", dev->sram_size);
  DLOG("bootrom_base 0x%x\n", dev->bootrom_base);
  DLOG("bootrom_size 0x%x\n", dev->bootrom_size);
  DLOG("option_base 0x%x\n", dev->option_base);
  DLOG("option_size 0x%x\n", dev->option_size);
  DLOG("flags %d\n\n", dev->flags);
  DLOG("otp_base %d\n\n", dev->otp_base);
  DLOG("otp_size %d\n\n", dev->otp_size);
}

struct stlink_chipid_params *stlink_chipid_get_params(uint32_t chip_id) {
  struct stlink_chipid_params *params = NULL;
  for (params = devicelist; params != NULL; params = params->next)
    if (params->chip_id == chip_id) {
      DLOG("detected chip_id parameters\n\n");
      dump_a_chip(params);
      break;
    }

  return (params);
}

void process_chipfile(char *fname) {
  FILE *fp;
  char *p, buf[256];
  char word[64], value[64];
  struct stlink_chipid_params *ts;
  int32_t nc;

  // fprintf (stderr, "processing chip-id file %s.\n", fname);
  fp = fopen(fname, "r");

  if (!fp) {
    perror(fname);
    return;
  }

  ts = calloc(sizeof(struct stlink_chipid_params), 1);

  while (fgets(buf, sizeof(buf), fp) != NULL) {

    if (strncmp(buf, "#", strlen("#")) == 0)
      continue; // ignore comments

    if ((strncmp(buf, "\n", strlen("\n")) == 0) ||
        (strncmp(buf, " ", strlen(" ")) == 0))
      continue; // ignore empty lines

    if (sscanf(buf, "%63s %63s", word, value) != 2) {
      fprintf(stderr, "Failed to read keyword or value\n");
      continue;
    }

    if (strcmp(word, "dev_type") == 0) {
      buf[strlen(buf) - 1] = 0; // chomp newline
      sscanf(buf, "%*s %n", &nc);
      ts->dev_type = strdup(buf + nc);
    } else if (strcmp(word, "ref_manual_id") == 0) {
      buf[strlen(buf) - 1] = 0; // chomp newline
      sscanf(buf, "%*s %n", &nc);
      ts->ref_manual_id = strdup(buf + nc);
    } else if (strcmp(word, "chip_id") == 0) {
      buf[strlen(buf) - 1] = 0; // chomp newline
      sscanf(buf, "%*s %n", &nc);
      if (sscanf(value, "%i", &ts->chip_id) < 1) {
        fprintf(stderr, "Failed to parse chip-id\n");
      }
    } else if (strcmp(word, "flash_type") == 0) {
      buf[strlen(buf) - 1] = 0; // chomp newline
      sscanf(buf, "%*s %n", &nc);
      // Match human readable flash_type with enum stm32_flash_type { }.
      if(strcmp(value, "C0") == 0) {
        ts->flash_type = STM32_FLASH_TYPE_C0;
      } else if (strcmp(value, "F0_F1_F3") == 0) {
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
      } else if (strcmp(value, "L4") == 0) {
        ts->flash_type = STM32_FLASH_TYPE_L4;
      } else if (strcmp(value, "L5_U5_H5") == 0) {
        ts->flash_type = STM32_FLASH_TYPE_L5_U5_H5;
      } else if (strcmp(value, "WB_WL") == 0) {
        ts->flash_type = STM32_FLASH_TYPE_WB_WL;
      } else {
        ts->flash_type = STM32_FLASH_TYPE_UNKNOWN;
      }
    } else if (strcmp(word, "flash_size_reg") == 0) {
      buf[strlen(buf) - 1] = 0; // chomp newline
      sscanf(buf, "%*s %n", &nc);
      if (sscanf(value, "%i", &ts->flash_size_reg) < 1) {
        fprintf(stderr, "Failed to parse flash size reg\n");
      }
    } else if (strcmp(word, "flash_pagesize") == 0) {
      buf[strlen(buf) - 1] = 0; // chomp newline
      sscanf(buf, "%*s %n", &nc);
      if (sscanf(value, "%i", &ts->flash_pagesize) < 1) {
        fprintf(stderr, "Failed to parse flash page size\n");
      }
    } else if (strcmp(word, "sram_size") == 0) {
      buf[strlen(buf) - 1] = 0; // chomp newline
      sscanf(buf, "%*s %n", &nc);
      if (sscanf(value, "%i", &ts->sram_size) < 1) {
        fprintf(stderr, "Failed to parse SRAM size\n");
      }
    } else if (strcmp(word, "bootrom_base") == 0) {
      buf[strlen(buf) - 1] = 0; // chomp newline
      sscanf(buf, "%*s %n", &nc);
      if (sscanf(value, "%i", &ts->bootrom_base) < 1) {
        fprintf(stderr, "Failed to parse BootROM base\n");
      }
    } else if (strcmp(word, "bootrom_size") == 0) {
      buf[strlen(buf) - 1] = 0; // chomp newline
      sscanf(buf, "%*s %n", &nc);
      if (sscanf(value, "%i", &ts->bootrom_size) < 1) {
        fprintf(stderr, "Failed to parse BootROM size\n");
      }
    } else if (strcmp(word, "option_base") == 0) {
      buf[strlen(buf) - 1] = 0; // chomp newline
      sscanf(buf, "%*s %n", &nc);
      if (sscanf(value, "%i", &ts->option_base) < 1) {
        fprintf(stderr, "Failed to parse option base\n");
      }
    } else if (strcmp(word, "option_size") == 0) {
      buf[strlen(buf) - 1] = 0; // chomp newline
      sscanf(buf, "%*s %n", &nc);
      if (sscanf(value, "%i", &ts->option_size) < 1) {
        fprintf(stderr, "Failed to parse option size\n");
      }
    } else if (strcmp(word, "flags") == 0) {
      buf[strlen(buf) - 1] = 0; // chomp newline
      sscanf(buf, "%*s %n", &nc);
      p = strtok(buf, " \t\n");

      while ((p = strtok(NULL, " \t\n"))) {
        if (strcmp(p, "none") == 0) {
          // NOP
        } else if (strcmp(p, "dualbank") == 0) {
          ts->flags |= CHIP_F_HAS_DUAL_BANK;
        } else if (strcmp(p, "swo") == 0) {
          ts->flags |= CHIP_F_HAS_SWO_TRACING;
        } else {
          fprintf(stderr, "Unknown flags word in %s: '%s'\n", fname, p);
        }
      }

      sscanf(value, "%x", &ts->flags);
    } else if (strcmp(word, "otp_base") == 0) {
      buf[strlen(buf) - 1] = 0; // chomp newline
      sscanf(buf, "%*s %n", &nc);
      if (sscanf(value, "%i", &ts->otp_base) < 1) {
        fprintf(stderr, "Failed to parse option size\n");
      }
    } else if (strcmp(word, "otp_size") == 0) {
      buf[strlen(buf) - 1] = 0; // chomp newline
      sscanf(buf, "%*s %n", &nc);
      if (sscanf(value, "%i", &ts->otp_size) < 1) {
        fprintf(stderr, "Failed to parse option size\n");
      }
    } else {
      fprintf(stderr, "Unknown keyword in %s: %s\n", fname, word);
    }
  }
  fclose(fp);
  ts->next = devicelist;
  devicelist = ts;
}

#if defined(STLINK_HAVE_DIRENT_H)
#include <dirent.h>

void init_chipids(char *dir_to_scan) {
  DIR *d;
  uint32_t nl; // namelen
  struct dirent *dir;

  if (!dir_to_scan) {
    dir_to_scan = "./";
  }

  devicelist = NULL;
  d = opendir(dir_to_scan);

  if (d) {
    while ((dir = readdir(d)) != NULL) {
      nl = (uint32_t)strlen(dir->d_name);

      if (strcmp(dir->d_name + nl - 5, ".chip") == 0) {
        char buf[1024];
        sprintf(buf, "%s/%s", dir_to_scan, dir->d_name);
        process_chipfile(buf);
      }
    }

    closedir(d);
  } else {
    perror(dir_to_scan);
    return;
  }
}

#endif // STLINK_HAVE_DIRENT_H

#if defined(_WIN32) && !defined(STLINK_HAVE_DIRENT_H)
#include <fileapi.h>
#include <strsafe.h>

void init_chipids(char *dir_to_scan) {
  HANDLE hFind = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATAA ffd;
  char filepath[MAX_PATH] = {0};
  StringCchCopyA(filepath, STLINK_ARRAY_SIZE(filepath), dir_to_scan);

  if (FAILED(
          StringCchCatA(filepath, STLINK_ARRAY_SIZE(filepath), "\\*.chip"))) {
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

#endif // defined(_WIN32) && !defined(STLINK_HAVE_DIRENT_H)
