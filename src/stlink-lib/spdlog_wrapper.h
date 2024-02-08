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