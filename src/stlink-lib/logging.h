/**
  ******************************************************************************
  * @file             logging.h
  * @brief            Logging for stlink-lib and the stlink tools
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef LOGGING_H
#define LOGGING_H

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include <stlink.h>

#ifdef  __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @addtogroup api_logging
 * @{
 */

/** Log levels; a message is logged if its level is at most the current log level */
enum ugly_loglevel {
    UDEBUG = 90,    ///< debug messages, including data dumps
    UINFO  = 50,    ///< informational messages (default of the tools)
    UWARN  = 30,    ///< warnings
    UERROR = 20     ///< errors
};

/** @name Log levels of the stlink_log_* interface (same values as enum ugly_loglevel) */
/** @{ */
#define STLINK_LOG_DEBUG UDEBUG     ///< debug messages
#define STLINK_LOG_INFO  UINFO      ///< informational messages
#define STLINK_LOG_WARN  UWARN      ///< warnings
#define STLINK_LOG_ERROR UERROR     ///< errors
/** @} */

/**
 * Log handler, see stlink_log_set_handler()
 * @param user    pointer given to stlink_log_set_handler()
 * @param level   STLINK_LOG_ERROR, STLINK_LOG_WARN, STLINK_LOG_INFO or STLINK_LOG_DEBUG
 * @param tag     origin of the message: name of the source file, or "libusb"
 * @param message the formatted message, without a trailing line break
 */
typedef void (*stlink_log_handler_t)(void *user, int32_t level, const char *tag, const char *message);

/**
 * Route all log messages of stlink-lib to the application.
 * Without a handler (default, or handler == NULL) messages are written to stderr.
 * The handler is process wide: set it before opening a device. It must be thread
 * safe, as it is called from several threads at once, e.g. by stlink_probe_usb().
 * While a handler is set, it also receives the messages of libusb (tag "libusb"):
 * for this, stlink-lib sets the process wide log callback of libusb (not on FreeBSD).
 * @param handler function receiving the messages, NULL restores the stderr output
 * @param user    pointer passed to every call of the handler
 */
void stlink_log_set_handler(stlink_log_handler_t handler, void *user);

/**
 * Set the log level: messages with a higher level are dropped.
 * Once set with this function, the level is no longer changed by the verbose
 * argument of stlink_open_usb(), stlink_open_remote() or stlink_v1_open().
 * The level is STLINK_LOG_DEBUG until it is set here or by an open function.
 * @param level STLINK_LOG_ERROR, STLINK_LOG_WARN, STLINK_LOG_INFO or STLINK_LOG_DEBUG
 */
void stlink_log_set_level(int32_t level);

/**
 * @return the current log level
 */
int32_t stlink_log_get_level(void);

/** @} */

/** @cond STLINK_INTERNAL */
#if defined(__GNUC__)
#define PRINTF_ARRT __attribute__ ((format (printf, 3, 4)))
#else
#define PRINTF_ARRT
#endif // __GNUC__
/** @endcond */

/**
 * Set the log level (legacy interface).
 * @deprecated Use stlink_log_set_level(). Unlike that function, the level set
 *             here is overridden by the verbose argument of the open functions.
 * @ingroup api_legacy
 * @param maximum_threshold the log level
 * @return 0
 */
int32_t ugly_init(int32_t maximum_threshold);

/**
 * Log a message, used by the DLOG(), ILOG(), WLOG() and ELOG() macros.
 * The message is dropped if @p level is above the current log level, otherwise
 * passed to the log handler or written to stderr.
 * @ingroup api_logging
 * @param level  UERROR, UWARN, UINFO or UDEBUG
 * @param tag    origin of the message, usually the source file name
 * @param format printf() style format, followed by its arguments
 * @return 1 if the message was logged, 0 if it was dropped
 */
int32_t ugly_log(int32_t level, const char *tag, const char *format, ...) PRINTF_ARRT;

/** @cond STLINK_INTERNAL */
/**
 * @addtogroup api_internal
 * @{
 */

/**
 * Map a log level to the log level of libusb.
 * @param v log level
 * @return the matching libusb log level (on FreeBSD: debug level of its libusb)
 */
int32_t ugly_libusb_log_level(enum ugly_loglevel v);

/**
 * Log a message regardless of the log level, for errors the user has to see in
 * any case (ELOG_ALWAYS()).
 * @param level  log level passed to the handler
 * @param tag    origin of the message
 * @param format printf() style format, followed by its arguments
 * @return 1
 */
int32_t stlink_log_unfiltered(int32_t level, const char *tag, const char *format, ...) PRINTF_ARRT;

/**
 * Apply the verbose argument of the open functions as log level, unless the
 * application has set one with stlink_log_set_level().
 * @param level the log level
 */
void stlink_log_open_level(int32_t level);

/** @} */

#define UGLY_LOG_FILE (strstr(__FILE__, "/") != NULL ? \
                       strrchr(__FILE__, '/')  + 1 : strstr(__FILE__, "\\") != NULL ? \
                       strrchr(__FILE__, '\\') + 1 : __FILE__)

// TODO: we need to write this in a more generic way, for now this should compile
// on visual studio (See http://stackoverflow.com/a/8673872/1836746)
#define DLOG_HELPER(format, ...)   ugly_log(UDEBUG, UGLY_LOG_FILE, format, __VA_ARGS__)
#define DLOG(...) ugly_log(UDEBUG, UGLY_LOG_FILE, __VA_ARGS__)
#define ILOG_HELPER(format, ...)   ugly_log(UINFO, UGLY_LOG_FILE, format, __VA_ARGS__)
#define ILOG(...) ugly_log(UINFO, UGLY_LOG_FILE, __VA_ARGS__)
#define WLOG_HELPER(format, ...)   ugly_log(UWARN, UGLY_LOG_FILE, format, __VA_ARGS__)
#define WLOG(...) ugly_log(UWARN, UGLY_LOG_FILE, __VA_ARGS__)
#define ELOG_HELPER(format, ...)   ugly_log(UERROR, UGLY_LOG_FILE, format, __VA_ARGS__)
#define ELOG(...) ugly_log(UERROR, UGLY_LOG_FILE, __VA_ARGS__)
#define ELOG_ALWAYS(...) stlink_log_unfiltered(UERROR, UGLY_LOG_FILE, __VA_ARGS__)
/** @endcond */

#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // LOGGING_H
