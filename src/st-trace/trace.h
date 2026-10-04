/**
  ******************************************************************************
  * @file           : trace.h
  * @brief          : Tool: st-trace
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @author         : John Hall (simplerobot)
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */
#ifndef TRACE_H
#define TRACE_H

#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <getopt.h>
#include <unistd.h>

#include <stlink.h>
#include <stlink_backend.h>
#include <stm32_register.h>

#include <chipid.h>
#include <logging.h>
#include <read_write.h>
#include <remote.h>
#include <usb.h>


#define DEFAULT_LOGGING_LEVEL 50
#define DEBUG_LOGGING_LEVEL 100
#define REMOTE_OPTION 0x100 /* long-only option id for --remote */

#define APP_RESULT_SUCCESS 0
#define APP_RESULT_INVALID_PARAMS 1
#define APP_RESULT_STLINK_NOT_FOUND 2
#define APP_RESULT_STLINK_MISSING_DEVICE 3
#define APP_RESULT_STLINK_UNSUPPORTED_DEVICE 4
#define APP_RESULT_STLINK_UNSUPPORTED_LINK 5
#define APP_RESULT_UNSUPPORTED_TRACE_FREQUENCY 6
#define APP_RESULT_STLINK_STATE_ERROR 7

// See D4.2 of https://developer.arm.com/documentation/ddi0403/ed/
#define TRACE_OP_IS_OVERFLOW(c) ((c) == 0x70)
#define TRACE_OP_IS_LOCAL_TIME(c) (((c)&0x0f) == 0x00 && ((c)&0x70) != 0x00)
#define TRACE_OP_IS_EXTENSION(c) (((c)&0x0b) == 0x08)
#define TRACE_OP_IS_GLOBAL_TIME(c) (((c)&0xdf) == 0x94)
#define TRACE_OP_IS_SOURCE(c) (((c)&0x03) != 0x00)
#define TRACE_OP_IS_SW_SOURCE(c) (((c)&0x03) != 0x00 && ((c)&0x04) == 0x00)
#define TRACE_OP_IS_HW_SOURCE(c) (((c)&0x03) != 0x00 && ((c)&0x04) == 0x04)
#define TRACE_OP_IS_TARGET_SOURCE(c) ((c) == 0x01)
#define TRACE_OP_GET_CONTINUATION(c) ((c)&0x80)
#define TRACE_OP_GET_SOURCE_SIZE(c) ((c)&0x03)
#define TRACE_OP_GET_SW_SOURCE_ADDR(c) ((c) >> 3)

typedef struct {
  bool show_help;
  bool show_version;
  int32_t logging_level;
  uint32_t core_frequency;
  uint32_t trace_frequency;
  bool reset_board;
  bool force;
  char *serial_number;
  char *remote;
} st_settings_t;

// We use a simple state machine to parse the trace data.
typedef enum {
  TRACE_STATE_UNKNOWN,
  TRACE_STATE_IDLE,
  TRACE_STATE_TARGET_SOURCE,
  TRACE_STATE_SKIP_FRAME,
  TRACE_STATE_SKIP_4,
  TRACE_STATE_SKIP_3,
  TRACE_STATE_SKIP_2,
  TRACE_STATE_SKIP_1,
} trace_state;

typedef struct {
  time_t start_time;
  bool configuration_checked;

  trace_state state;

  uint32_t count_raw_bytes;
  uint32_t count_target_data;
  uint32_t count_time_packets;
  uint32_t count_hw_overflow;
  uint32_t count_sw_overflow;
  uint32_t count_error;

  uint8_t unknown_opcodes[256 / 8];
  uint32_t unknown_sources;
} st_trace_t;


int32_t stlink_trace_enable(stlink_t* sl, uint32_t frequency);
int32_t stlink_trace_disable(stlink_t* sl);
int32_t stlink_trace_read(stlink_t* sl, uint8_t* buf, uint32_t size);

static void usage(void);
static bool parse_frequency(char* text, uint32_t* result);
bool parse_options(int32_t argc, char **argv, st_settings_t *settings);
static stlink_t *stlink_connect(const st_settings_t *settings);
static bool enable_trace(stlink_t *stlink, const st_settings_t *settings, uint32_t trace_frequency);
static trace_state update_trace_idle(st_trace_t *trace, uint8_t c);
static trace_state update_trace(st_trace_t *trace, uint8_t c);
static bool read_trace(stlink_t *stlink, st_trace_t *trace);
static void check_for_configuration_error(stlink_t *stlink, st_trace_t *trace, uint32_t trace_frequency);

#endif // TRACE_H