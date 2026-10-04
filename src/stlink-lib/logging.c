/**
  ******************************************************************************
  * @file           : logging.c
  * @brief          : Logging for stlink-lib and the stlink tools
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#define __STDC_WANT_LIB_EXT1__ 1

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "libusb_settings.h"


static int32_t max_level = UDEBUG;
static bool level_set_explicitly = false;   // set by stlink_log_set_level()

static stlink_log_handler_t log_handler = NULL;
static void *log_handler_user = NULL;

static void route_libusb_messages(bool enable);

void stlink_log_set_handler(stlink_log_handler_t handler, void *user) {
  log_handler_user = user;
  log_handler = handler;
  route_libusb_messages(handler != NULL);
}

void stlink_log_set_level(int32_t level) {
  max_level = level;
  level_set_explicitly = true;
}

int32_t stlink_log_get_level(void) {
  return (max_level);
}

int32_t ugly_init(int32_t maximum_threshold) {
  max_level = maximum_threshold;
  return (0);
}

void stlink_log_open_level(int32_t level) {
  // An application that set its level with stlink_log_set_level() keeps it,
  // for all others the verbose argument of the open functions applies as before.
  if(!level_set_explicitly) {
    max_level = level;
  }
}

/*
 * Format a message into buf. A message that doesn't fit is formatted into a
 * buffer from the heap instead, which the caller has to free (result != buf).
 * Returns NULL if the message can't be formatted.
 */
static char *format_message(char *buf, size_t buf_len, const char *format, va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);
  int32_t len = vsnprintf(buf, buf_len, format, args_copy);
  va_end(args_copy);

  if(len < 0) {
    return (NULL);
  }

  if((size_t) len < buf_len) {
    return (buf);
  }

  char *heap_buf = malloc((size_t) len + 1);

  if(heap_buf == NULL) {
    return (buf); // truncated message
  }

  vsnprintf(heap_buf, (size_t) len + 1, format, args);
  return (heap_buf);
}

/* Remove one trailing line break: messages to the handler come without it */
static void strip_line_break(char *msg) {
  size_t len = strlen(msg);

  if(len > 0 && msg[len - 1] == '\n') {
    msg[--len] = '\0';

    if(len > 0 && msg[len - 1] == '\r') {
      msg[--len] = '\0';
    }
  }
}

/* Default output: one line on stderr, written at once so that lines of several threads don't mix */
static void write_to_stderr(int32_t level, const char *tag, const char *msg) {
  time_t mytt = time(NULL);

  struct tm *ptt;
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) // C11
  struct tm tt;
  ptt = &tt;
# if defined (_WIN32) || defined(__STDC_LIB_EXT1__)
  localtime_s(&tt, &mytt);
# else
  localtime_r(&mytt, &tt);
# endif
#else
  ptt = localtime(&mytt);
#endif

  char header[160];
  int32_t header_len;
  const char *level_name;

  switch (level) {
  case UDEBUG:
    level_name = "DEBUG";
    break;
  case UINFO:
    level_name = "INFO";
    break;
  case UWARN:
    level_name = "WARN";
    break;
  case UERROR:
    level_name = "ERROR";
    break;
  default:
    level_name = NULL;
    break;
  }

  if(level_name != NULL) {
    header_len = snprintf(header, sizeof(header), "%d-%02d-%02dT%02d:%02d:%02d %s %s: ",
                          ptt->tm_year + 1900, ptt->tm_mon + 1, ptt->tm_mday,
                          ptt->tm_hour, ptt->tm_min, ptt->tm_sec, level_name, tag);
  } else {
    header_len = snprintf(header, sizeof(header), "%d-%02d-%02dT%02d:%02d:%02d %d %s: ",
                          ptt->tm_year + 1900, ptt->tm_mon + 1, ptt->tm_mday,
                          ptt->tm_hour, ptt->tm_min, ptt->tm_sec, level, tag);
  }

  if(header_len < 0) {
    header_len = 0;
    header[0] = '\0';
  } else if((size_t) header_len >= sizeof(header)) {
    header_len = (int32_t) sizeof(header) - 1; // very long tag, truncated
  }

  size_t msg_len = strlen(msg);
  char line_buf[1024];
  char *line = line_buf;

  if((size_t) header_len + msg_len + 1 > sizeof(line_buf)) {
    line = malloc((size_t) header_len + msg_len + 1);
  }

  fflush(stdout); // keep the order of stdout and stderr output on the console

  if(line == NULL) {
    fputs(header, stderr);
    fputs(msg, stderr);
  } else {
    memcpy(line, header, (size_t) header_len);
    memcpy(line + header_len, msg, msg_len + 1);
    fputs(line, stderr);

    if(line != line_buf) {
      free(line);
    }
  }

  fflush(stderr);
}

static void log_message(int32_t level, const char *tag, const char *format, va_list args) {
  char msg_buf[512];
  char *msg = format_message(msg_buf, sizeof(msg_buf), format, args);

  if(msg == NULL) {
    return;
  }

  // read once, the handler may be changed by another thread
  stlink_log_handler_t handler = log_handler;

  if(handler != NULL) {
    strip_line_break(msg);
    handler(log_handler_user, level, tag, msg);
  } else {
    write_to_stderr(level, tag, msg);
  }

  if(msg != msg_buf) {
    free(msg);
  }
}

int32_t ugly_log(int32_t level, const char *tag, const char *format, ...) {
  if(level > max_level) {
    return (0);
  }

  va_list args;
  va_start(args, format);
  log_message(level, tag, format, args);
  va_end(args);
  return (1);
}

int32_t stlink_log_unfiltered(int32_t level, const char *tag, const char *format, ...) {
  va_list args;
  va_start(args, format);
  log_message(level, tag, format, args);
  va_end(args);
  return (1);
}

/*
 * libusb messages: while a log handler is set, libusb's process wide log callback
 * passes them on to the handler (tag "libusb"). They are filtered by the libusb log
 * level set by the open functions and by the stlink log level. Without a handler
 * the libusb callback isn't touched, so libusb writes to stderr as it always did.
 * FreeBSD comes with its own libusb implementation, which keeps its own output.
 */
#if !defined(__FreeBSD__)
static void LIBUSB_CALL libusb_log_callback(libusb_context *ctx, enum libusb_log_level level, const char *str) {
  (void) ctx;
  stlink_log_handler_t handler = log_handler;

  if(handler == NULL) {
    fputs(str, stderr);
    return;
  }

  int32_t stlink_level;

  switch (level) {
  case LIBUSB_LOG_LEVEL_ERROR:
    stlink_level = UERROR;
    break;
  case LIBUSB_LOG_LEVEL_WARNING:
    stlink_level = UWARN;
    break;
  case LIBUSB_LOG_LEVEL_INFO:
    stlink_level = UINFO;
    break;
  default:
    stlink_level = UDEBUG;
    break;
  }

  if(stlink_level > max_level) {
    return;
  }

  size_t len = strlen(str);
  char msg_buf[512];
  char *msg = (len < sizeof(msg_buf)) ? msg_buf : malloc(len + 1);

  if(msg == NULL) {
    return;
  }

  memcpy(msg, str, len + 1);
  strip_line_break(msg);
  handler(log_handler_user, stlink_level, "libusb", msg);

  if(msg != msg_buf) {
    free(msg);
  }
}
#endif // __FreeBSD__

static void route_libusb_messages(bool enable) {
#if !defined(__FreeBSD__)
  libusb_set_log_cb(NULL, enable ? libusb_log_callback : NULL, LIBUSB_LOG_CB_GLOBAL);
#else
  (void) enable;
#endif
}

/*
 *  Log message levels.
 *  - LIBUSB_LOG_LEVEL_NONE (0)    : no messages ever printed by the library
 * (default)
 *  - LIBUSB_LOG_LEVEL_ERROR (1)   : error messages are printed to stderr
 *  - LIBUSB_LOG_LEVEL_WARNING (2) : warning and error messages are printed to
 * stderr
 *  - LIBUSB_LOG_LEVEL_INFO (3)    : informational messages are printed to
 * stderr
 *  - LIBUSB_LOG_LEVEL_DEBUG (4)   : debug and informational messages are
 * printed to stderr
 */
int32_t ugly_libusb_log_level(enum ugly_loglevel v) {
#ifdef __FreeBSD__
  // FreeBSD includes its own reimplementation of libusb.
  // Its libusb_set_debug() function expects a lib_debug_level
  // instead of a lib_log_level and is verbose enough to drown out
  // all other output.
  switch (v) {
  case UDEBUG:
    return (3); // LIBUSB_DEBUG_FUNCTION + LIBUSB_DEBUG_TRANSFER
  case UINFO:
    return (1); // LIBUSB_DEBUG_FUNCTION only
  case UWARN:
    return (0); // LIBUSB_DEBUG_NO
  case UERROR:
    return (0); // LIBUSB_DEBUG_NO
  }
  return (0);
#else
  switch (v) {
  case UDEBUG:
    return (4);
  case UINFO:
    return (3);
  case UWARN:
    return (2);
  case UERROR:
    return (1);
  }
  return (2);
#endif
}
