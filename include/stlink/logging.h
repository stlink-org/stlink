/*
 * Ugly, low performance, configurable level, logging "framework"
 */

#ifndef UGLYLOGGING_H
#define	UGLYLOGGING_H

#ifdef	__cplusplus
extern "C" {
#endif

enum ugly_loglevel {
	UDEBUG = 90,
	UINFO  = 50,
	UWARN  = 30,
	UERROR = 20,
	UFATAL = 10
};

int ugly_init(int maximum_threshold);
int ugly_log(int level, const char *tag, const char *format, ...);

#ifndef CMAKE_SOURCE_DIR_LENGTH
#define CMAKE_SOURCE_DIR_LENGTH 0
#endif
#define UGLY_LOG_FILE (__FILE__+CMAKE_SOURCE_DIR_LENGTH)

/** @todo we need to write this in a more generic way, for now this should compile
 on visual studio (See http://stackoverflow.com/a/8673872/1836746) */
#define DLOG_HELPER(format, ...)   ugly_log(UDEBUG, UGLY_LOG_FILE, format, __VA_ARGS__)
#define DLOG(...) DLOG_HELPER(__VA_ARGS__, "")
#define ILOG_HELPER(format, ...)   ugly_log(UINFO, UGLY_LOG_FILE, format, __VA_ARGS__)
#define ILOG(...) ILOG_HELPER(__VA_ARGS__, "")
#define WLOG_HELPER(format, ...)   ugly_log(UWARN, UGLY_LOG_FILE, format, __VA_ARGS__)
#define WLOG(...) WLOG_HELPER(__VA_ARGS__, "")
#define ELOG_HELPER(format, ...)   ugly_log(UERROR, UGLY_LOG_FILE, format, __VA_ARGS__)
#define ELOG(...) ELOG_HELPER(__VA_ARGS__, "")
#define fatal_helper(format, ...)  ugly_log(UFATAL, UGLY_LOG_FILE, format, __VA_ARGS__)
#define fatal(...) fatal_helper(__VA_ARGS__, "")

#ifdef	__cplusplus
}
#endif

#endif	/* UGLYLOGGING_H */

