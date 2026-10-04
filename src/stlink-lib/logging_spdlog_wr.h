/*
 * File: logging_spdlog_wr.h
 *
 * spdlog: Very fast, header-only/compiled, C++ logging library.
 * This is a wrapper file for spdlog.
 */

#ifndef _SPDLOG_WRAPPER_
#define _SPDLOG_WRAPPER_

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC int spdlogLog(int level, const char *str, ...);

#undef EXTERNC

#endif