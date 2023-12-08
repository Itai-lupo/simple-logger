#pragma once
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // !_GNU_SOURCE

#ifdef __cplusplus
#define USE_FMT_LOG
#endif

#ifndef __cplusplus
#define USE_C_LOG
#endif

#include <linux/limits.h>
#include <unistd.h>

#include "defines/colors.h"
#include "defines/logInfo.h"
#include "defines/logLevels.h"

#ifdef USE_C_LOG
#include "formatters/cFormatter.h"
#define ERROR_FMT "error from (%d:%d) with code %d"
#endif

#ifdef USE_FMT_LOG
#include "formatters/fmtFormatter.h"
#define ERROR_FMT "error from ({}:{}) with code {}"
#endif

#ifndef ACTIVE_LOG_LEVEL
#define ACTIVE_LOG_LEVEL LOG_TRACE_LEVEL
#endif // !ACTIVE_LOG_LEVEL

#include "defines/logMacros.h"

#ifndef TRACE_MACRO
#define TRACE_MACRO(msg, ...) LOG_ERR(ERROR_FMT msg, (uint64_t)err.fileId, (uint64_t)err.line, (uint64_t)err.errorCode __VA_OPT__(,) __VA_ARGS__)
#endif // !TRACE_MACRO

#include "err.h"

#ifdef __cplusplus
extern "C"
{
#endif

	err_t writeLog(logInfo logData);

	THROWS err_t initLogger();
	THROWS err_t closeLogger();
#ifdef __cplusplus
}
#endif
