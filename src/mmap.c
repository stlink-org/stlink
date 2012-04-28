/* mmap.c -- version of mmap for gold.  */

/* Copyright 2009 Free Software Foundation, Inc.
   Written by Vladimir Simonov <sv@sw.ru>.

   This file is part of gold.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

//#include "config.h"
//#include "ansidecl.h"

#include <string.h>
#include <sys/types.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "mmap.h"

extern ssize_t pread (int, void *, size_t, off_t);

void *
mmap (void *__addr, size_t __len, int __prot,
       int __flags, int __fd, long long  __offset)
{
  void *ret;
  ssize_t count;

  if (__addr || (__fd != -1 && (__prot & PROT_WRITE))
             || (__flags & MAP_SHARED))
    return MAP_FAILED;

  ret = malloc (__len);
  if (!ret)
    return MAP_FAILED;

  if (__fd == -1)
    return ret;

  count = pread (__fd, ret, __len, __offset);
  if ((size_t) count != __len)
    {
    free (ret);
    return MAP_FAILED;
    }

  return ret;
}

int
munmap (void *__addr, size_t __len)
{
  free (__addr);
  return 0;
}
