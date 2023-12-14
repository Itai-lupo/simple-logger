/**
 * @file logMacros.h
 * @author itai lupo
 * @brief all the log macros for the user
 * @version 0.1
 * @date 2023-12-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#ifndef PRINT_FUNCTION
/**
 * @brief this is the log function all the log will use by default, you can redefine it to whatever you want
 * 	defaults to  writeLog(logData)
 *
 * @parm logData a logInfo_t struct with all the data for the log backend might need
 * @see logInfo_t
 */
#define PRINT_FUNCTION(logData) writeLog(logData)
#endif // !PRINT_FUNCTION

/**
 * @brief this is combine all the log api ingredients into one macro to prevent some code dup
 *
 * @param severity the logLevel of the log
 * @param msg the format string of the log msg
 * @see CONSTRACT_LOG_INFO
 * @see FORMAT_FUNC
 * @see PRINT_FUNCTION
 *
 */
#define LOG_MACRO(severity, msg, ...)                                                                                  \
	do                                                                                                                 \
	{                                                                                                                  \
		CONSTRACT_LOG_INFO(logData, severity);                                                                         \
		FORMAT_FUNC(logData, msg __VA_OPT__(, ) __VA_ARGS__);                                                          \
		PRINT_FUNCTION(logData);                                                                                       \
	} while (0)

#if LOG_TRACE_LEVEL >= ACTIVE_LOG_LEVEL
/**
 * @brief log the trace level, if LOG_TRACE_LEVEL is less then ACTIVE_LOG_LEVEL will be nothing.
 */
#define LOG_TRACE(msg, ...) LOG_MACRO(traceLevel, msg __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_TRACE(msg, ...)
#endif

#if LOG_DEBUG_LEVEL >= ACTIVE_LOG_LEVEL
/**
 * @brief log the debug level, if LOG_DEBUG_LEVEL is less then ACTIVE_LOG_LEVEL will be nothing.
 */
#define LOG_DEBUG(msg, ...) LOG_MACRO(debugLevel, msg __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_DEBUG(msg, ...)
#endif

#if LOG_INFO_LEVEL >= ACTIVE_LOG_LEVEL
/**
 * @brief log the info level, if LOG_INFO_LEVEL is less then ACTIVE_LOG_LEVEL will be nothing.
 */
#define LOG_INFO(msg, ...) LOG_MACRO(infoLevel, msg __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_INFO(msg, ...)
#endif

#if LOG_WARN_LEVEL >= ACTIVE_LOG_LEVEL
/**
 * @brief log the warning level, if LOG_WARN_LEVEL is less then ACTIVE_LOG_LEVEL will be nothing.
 */
#define LOG_WARN(msg, ...) LOG_MACRO(warnLevel, msg __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_WARN(msg, ...)
#endif

#if LOG_ERR_LEVEL >= ACTIVE_LOG_LEVEL
/**
 * @brief log the error level, if LOG_ERR_LEVEL is less then ACTIVE_LOG_LEVEL will be nothing.
 */
#define LOG_ERR(msg, ...) LOG_MACRO(errorLevel, msg __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_ERR(msg, ...)
#endif

#if LOG_CRITICAL_LEVEL >= ACTIVE_LOG_LEVEL
/**
 * @brief log the critical level, if LOG_CRITICAL_LEVEL is less then ACTIVE_LOG_LEVEL will be nothing.
 */
#define LOG_CRITICAL(msg, ...) LOG_MACRO(criticalLevel, msg __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_CRITICAL(msg, ...)
#endif
