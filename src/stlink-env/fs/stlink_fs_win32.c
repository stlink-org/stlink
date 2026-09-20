/**
  ******************************************************************************
  * @file           : stlink_fs_win32.c
  * @brief          : Filesystem lookups on Windows
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-09-17
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#include <windows.h>
#include <fileapi.h>
#include <strsafe.h>

#include <stlink_fs.h>

#include "chipid.h"
#include "logging.h"

bool stlink_exe_dir(char *buf, size_t len) {
  DWORD n = GetModuleFileNameA(NULL, buf, (DWORD)len);
  char *cut;

  if((n == 0) || (n >= len)) { return (false); }

  /* Win32 takes either separator but not a mixture, and the caller appends
   * with '/'. Settle on that here rather than leave it to chance. */
  for(char *p = buf; *p != '\0'; p++) {
    if(*p == '\\') { *p = '/'; }
  }

  cut = strrchr(buf, '/');

  if(cut == NULL) { return (false); }

  *cut = '\0';

  return (true);
}

bool search_for_chips(const char *dir) {
  WIN32_FIND_DATAA ffd;
  char path[MAX_PATH] = {0};
  HANDLE hFind;
  bool found = false;

  DLOG("Looking for chip description files in %s\n", dir);

  if(FAILED(StringCchCopyA(path, sizeof(path), dir)) ||
      FAILED(StringCchCatA(path, sizeof(path), "/*" CHIP_FILE_EXT))) {
    ELOG("Path too long: %s\n", dir);
    return (false);
  }

  hFind = FindFirstFileA(path, &ffd);

  if(INVALID_HANDLE_VALUE == hFind) { return (false); }

  do {
    if(FAILED(StringCchCopyA(path, sizeof(path), dir)) ||
        FAILED(StringCchCatA(path, sizeof(path), "/")) ||
        FAILED(StringCchCatA(path, sizeof(path), ffd.cFileName))) {
      ELOG("Path too long: %s/%s\n", dir, ffd.cFileName);
      continue;
    }

    process_chipfile(path);
    found = true;
  } while (FindNextFileA(hFind, &ffd) != 0);

  FindClose(hFind);

  return (found);
}
