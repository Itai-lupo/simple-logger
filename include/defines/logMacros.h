#pragma once

#ifndef PRINT_FUNCTION
#define PRINT_FUNCTION(logData) writeLog(logData)
#endif // !PRINT_FUNCTION

#define LOG_MACRO(severity, msg, ...)                                                                                  \
	do                                                                                                                 \
	{                                                                                                                  \
		CONTRACT_LOG_INFO(logData, severity);                                                                          \
		FORMAT_FUNC(logData, msg __VA_OPT__(, ) __VA_ARGS__);                                                          \
		PRINT_FUNCTION(logData);                                                                                       \
	} while (0)

#if LOG_TRACE_LEVEL >= ACTIVE_LOG_LEVEL
#define LOG_TRACE(msg, ...) LOG_MACRO(trace, msg __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_TRACE(msg, ...)
#endif

#if LOG_DEBUG_LEVEL >= ACTIVE_LOG_LEVEL
#define LOG_DEBUG(msg, ...) LOG_MACRO(debug, msg __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_DEBUG(msg, ...)
#endif

#if LOG_INFO_LEVEL >= ACTIVE_LOG_LEVEL
#define LOG_INFO(msg, ...) LOG_MACRO(info, msg __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_INFO(msg, ...)
#endif

#if LOG_WARN_LEVEL >= ACTIVE_LOG_LEVEL
#define LOG_WARN(msg, ...) LOG_MACRO(warn, msg __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_WARN(msg, ...)
#endif

#if LOG_ERR_LEVEL >= ACTIVE_LOG_LEVEL
#define LOG_ERR(msg, ...) LOG_MACRO(err, msg __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_ERR(msg, ...)
#endif

#if LOG_CRITICAL_LEVEL >= ACTIVE_LOG_LEVEL
#define LOG_CRITICAL(msg, ...) LOG_MACRO(critical, msg __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_CRITICAL(msg, ...)
#endif