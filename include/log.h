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
#endif

#ifdef USE_FMT_LOG
#include "formatters/fmtFormatter.h"
#endif

#ifndef ACTIVE_LOG_LEVEL
#define ACTIVE_LOG_LEVEL LOG_TRACE_LEVEL
#endif // !ACTIVE_LOG_LEVEL

// #undef USE_FILENAME

#ifdef __cplusplus
extern "C"
{
#endif

	void writeLog(logInfo logData);

	int initLogger();
	int closeLogger();
#ifdef __cplusplus
}
#endif

#include "defines/logMacros.h"
