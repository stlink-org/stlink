/* mmap.h -- mmap family functions prototypes for gold.  */

/* Copyright 2009 Free Software Foundation, Inc.
   Written by Vladimir Simonov <sv@sw.ru>

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

#ifndef GOLD_MMAP_H
#define GOLD_MMAP_H

#ifdef HAVE_SYS_MMAN_H

#include <sys/mman.h>

/* Some BSD systems still use MAP_ANON instead of MAP_ANONYMOUS.  */

#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif

#else

#define PROT_READ 0x1
#define PROT_WRITE 0x2

#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02

#define MAP_ANONYMOUS 0x20

#define MAP_FAILED ((void *) -1)
#endif /* HAVE_SYS_MMAN_H */

#ifndef HAVE_MMAP
#ifdef __cplusplus
extern "C" {
#endif
extern void *mmap (void *__addr, size_t __len, int __prot,
                    int __flags, int __fd, long long __offset);
extern int munmap (void *__addr, size_t __len);
#ifdef __cplusplus
}
#endif

#endif /* HAVE_MMAP */

#endif /* GOLD_MMAP_H */
