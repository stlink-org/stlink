/**
  ******************************************************************************
  * @file             helper.h
  * @brief            General helper functions
  * @copyright        Copyright (c) 2026 stlink-org. All rights reserved.
  * @date             2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */


#ifndef HELPER_H
#define HELPER_H

#ifdef STLINK_HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <sys_time.h>
#endif // STLINK_HAVE_SYS_TIME_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>


/** @cond STLINK_INTERNAL */
/**
 * Milliseconds of the wall clock, for timeouts (wraps around).
 * @ingroup api_internal
 * @return the time in ms
 */
uint32_t time_ms();
/** @endcond */

/**
 * Parse a frequency given on the command line.
 * @ingroup api_util
 * @param str decimal number in kHz, optionally followed by "k"/"K" (kHz) or "M" (MHz), e.g. "1800", "4M"
 * @return    the frequency in kHz, or -1 for NULL or an invalid suffix (an empty
 *            string gives 0, negative numbers are passed through)
 */
int32_t arg_parse_freq(const char *str);

#endif // HELPER_H
