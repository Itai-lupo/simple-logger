/**
 * @file log.h
 * @author itai lupo
 * @brief  the api for the logger, this is the file you want to include in order to use the full logger.
 * @version 0.1
 * @date 2023-12-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // !_GNU_SOURCE

#ifdef __cplusplus
#ifndef USE_C_LOG
/**
 * @brief define that in order to use fmt::format as the format function for logs
 * will be defined by default if c++
 */
#define USE_FMT_LOG
#endif //! USE_FMT_LOG
#endif //!__cplusplus

#ifndef __cplusplus
#ifndef USE_FMT_LOG
/**
 * @brief define this to use sprintf as the format function for logs/
 * this will be defined by default if c,
 */
#define USE_C_LOG
#endif //! USE_FMT_LOG
#endif //__cplusplus

#include <linux/limits.h>
#include <unistd.h>

#include "defines/colors.h"
#include "types/logInfo.h"
#include "types/logLevels.h"

/**
 * @def ERROR_FMT
 * @brief the format string to use for checks traces.
 *
 * @see TRACE_MACRO
 *
 * @def RETHROW_FMT
 * @brief the format string to use for rethrows traces.
 *
 * @see RETRACE_MACRO
 */

#ifdef USE_C_LOG
#include "formatters/cFormatter.h"

#define ERROR_FMT "error from (%lu:%lu) with code %lu"
#define RETHROW_FMT "rethrow error from (%lu:%lu) with code %lu at %d:%d"
#endif

#ifdef USE_FMT_LOG
#include "formatters/fmtFormatter.h"
#define ERROR_FMT "error from ({}:{}) with code {} "
#define RETHROW_FMT "rethrow error from ({}:{}) with code {} at {}:{} "
#endif

#ifndef ACTIVE_LOG_LEVEL
/**
 * @brief the log level to log from, defaults to lowest which is LOG_TRACE_LEVEL.
 * log levels less then that will be compiled to nothing.
 */
#define ACTIVE_LOG_LEVEL LOG_TRACE_LEVEL
#endif

#include "defines/logMacros.h"

#ifndef TRACE_MACRO
/**
 * @brief this is used to print the error messages from checks.
 * can expect an err_t err to be defined.
 *
 * @param msg the base format of the msg to print
 * @see ERROR_FMT
 * @see CHECK
 */
#define TRACE_MACRO(msg, ...)                                                                                          \
	LOG_ERR(ERROR_FMT msg, (uint64_t)err.fileId, (uint64_t)err.line, (uint64_t)err.errorCode __VA_OPT__(, ) __VA_ARGS__)
#endif

#ifndef RETRACE_MACRO
/**
 * @brief this is used to print the error messages from rethrows.
 * can expect an err_t err to be defined.
 *
 * @param msg the base format of the msg to print
 * @see RETHROW_FMT
 * @see RETHROW
 */
#define RETRACE_MACRO(msg, ...)                                                                                        \
	LOG_ERR(RETHROW_FMT msg, (uint64_t)err.fileId, (uint64_t)err.line, (uint64_t)err.errorCode, FILE_ID,               \
			__LINE__ __VA_OPT__(, ) __VA_ARGS__)
#endif

#include "err.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief write a log to the log server over unix domain socket, the full log will be printed on the server on delay
	 * should only be used when the log server is running
	 *
	 * @param logData the data to log to the server
	 * @return err_t if there was any error
	 *
	 * @see logServer/server.h
	 * @see logInfo_t
	 */
	err_t writeLog(logInfo_t logData);

	/**
	 * @brief init the log server, will be the first(or one of the first) things to run on the program, before main
	 * @see initLogServer
	 */
	err_t initLogger();

	/**
	 * @brief closes the logServer/wait for it to close will on the exit or return from main
	 * should be the last thing to run.
	 * will be run on all forks
	 * @see closeLogServer
	 */
	err_t closeLogger();

#ifdef __cplusplus
}
#endif
