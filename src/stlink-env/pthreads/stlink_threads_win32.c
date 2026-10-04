/**
  ******************************************************************************
  * @file           : stlink_threads_win32.c
  * @brief          : pthreads wrapper for WIN32
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @author         : Andreas Michelis (a-michelis)
  * @date           : 2026-09-15
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <windows.h>

#include <process.h>

#include "stlink_threads.h"

/*
 * Win32 wants an entry point of unsigned __stdcall (*)(void *), which is not
 * the shape the caller supplies. Casting is not an option, because on 32-bit
 * Windows __stdcall and __cdecl genuinely differ and this project ships i686
 * binaries, so the caller's function and argument travel in a context instead.
 */
struct stlink_thread_ctx {
    stlink_thread_fn fn;
    void *arg;
};

static unsigned __stdcall stlink_thread_entry(void *raw) {
    struct stlink_thread_ctx ctx = *(struct stlink_thread_ctx *)raw;
    free(raw);

    ctx.fn(ctx.arg);

    return (0);
}

int32_t stlink_thread_create(stlink_thread_t *thread, stlink_thread_fn fn, void *arg) {
    struct stlink_thread_ctx *ctx = malloc(sizeof(*ctx));

    if(ctx == NULL) { return (-1); }

    ctx->fn = fn;
    ctx->arg = arg;

    /* _beginthreadex rather than CreateThread, because the worker calls into
     * the C runtime and that needs its per-thread state initialised. */
    uintptr_t handle = _beginthreadex(NULL, 0, stlink_thread_entry, ctx, 0, NULL);

    if(handle == 0) {
        int32_t error = (int32_t)errno;
        free(ctx);
        return ((error == 0) ? -1 : error);
    }

    *thread = (stlink_thread_t)handle;

    return (0);
}

int32_t stlink_thread_join(stlink_thread_t thread) {
    if(WaitForSingleObject((HANDLE)thread, INFINITE) != WAIT_OBJECT_0) {
        return ((int32_t)GetLastError());
    }

    CloseHandle((HANDLE)thread);

    return (0);
}