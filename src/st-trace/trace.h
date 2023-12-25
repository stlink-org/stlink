/*
 * File: trace.h
 *
 * Tool st-trace 
 */

#ifndef TRACE_H
#define TRACE_H

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