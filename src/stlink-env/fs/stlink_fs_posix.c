/**
  ******************************************************************************
  * @file           : stlink_fs_posix.c
  * @brief          : Filesystem lookups on Unix (POSIX) systems
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-09-17
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include <stlink_fs.h>

#include "chipid.h"
#include "logging.h"

bool stlink_exe_dir(char *buf, size_t len) {
  char *cut;

#if defined(__APPLE__)
  uint32_t n = (uint32_t)len;

  if(_NSGetExecutablePath(buf, &n) != 0) { return (false); }
#else
  ssize_t n = readlink("/proc/self/exe", buf, len - 1);

  if((n <= 0) || ((size_t)n >= (len - 1))) { return (false); }

  buf[n] = '\0';
#endif

  cut = strrchr(buf, '/');

  if(cut == NULL) { return (false); }

  *cut = '\0';

  return (true);
}

bool search_for_chips(const char *dir) {
  DIR *d = opendir(dir);
  struct dirent *entry;
  size_t el = strlen(CHIP_FILE_EXT);
  bool found = false;

  DLOG("Looking for chip description files in %s\n", dir);

  if(d == NULL) { return (false); }

  while ((entry = readdir(d)) != NULL) {
    char path[1024];
    size_t nl = strlen(entry->d_name);

    if((nl <= el) || (strcmp(entry->d_name + nl - el, CHIP_FILE_EXT) != 0)) { continue; }

    if(snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name) >= (int)sizeof(path)) {
      ELOG("Path too long: %s/%s\n", dir, entry->d_name);
      continue;
    }

    process_chipfile(path);
    found = true;
  }

  closedir(d);

  return (found);
}
