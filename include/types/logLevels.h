/**
 * @file logLevels.h
 * @author itai lupo
 * @brief this is the all levels you can print you logs
 * @version 0.1
 * @date 2023-12-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#define LOG_TRACE_LEVEL 0
#define LOG_DEBUG_LEVEL 1
#define LOG_INFO_LEVEL 2
#define LOG_WARN_LEVEL 3
#define LOG_ERR_LEVEL 4
#define LOG_CRITICAL_LEVEL 5
#define NO_LOG 6

/**
 * @brief this is a enum for all the log level, unless it is for precompile, stuff, use this
 */
typedef enum
{
	traceLevel = LOG_TRACE_LEVEL,
	debugLevel = LOG_DEBUG_LEVEL,
	infoLevel = LOG_INFO_LEVEL,
	warnLevel = LOG_WARN_LEVEL,
	errorLevel = LOG_ERR_LEVEL,
	criticalLevel = LOG_CRITICAL_LEVEL,
} logLevel;

#ifdef __cplusplus
constexpr
#endif
	static const char logLevelShortMessage[] = {
		[traceLevel] = 'T', [debugLevel] = 'D', [infoLevel] = 'I',
		[warnLevel] = 'W',	[errorLevel] = 'E', [criticalLevel] = 'C',
};
