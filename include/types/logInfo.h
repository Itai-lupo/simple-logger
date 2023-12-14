/**
 * @file logInfo_t.h
 * @author itai lupo
 * @brief this is the info the log backend need to het in order to output good logs.
 * @version 0.1
 * @date 2023-12-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "types/logLevels.h"

#include <linux/limits.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>

/**
 * @brief the maximum length a log message can be
 */
#define MAX_LOG_LEN 128

#ifndef CLOCK_TO_USE
/**
 * @brief the linux clock to use, can be defined to change to other clock
 * defaults to CLOCK_REALTIME_COARSE
 *
 * @see https://man7.org/linux/man-pages/man3/clock_gettime.3.html
 */
#define CLOCK_TO_USE CLOCK_REALTIME_COARSE
#endif

#ifndef FILE_ID
/**
 * @brief the FILE_ID should be unique int at compile time
 * if not define defaults to -1
 *
 * @see https://github.com/Itai-lupo/cpp-project-template/blob/master/.scripts/Makefile.fileId for example
 */
#define FILE_ID -1
#endif

/**
 * @struct lineInfo_t
 * @brief all the data to be passed to she log server for one log line
 *
 * @var lineInfo_t::fileName
 * the name of the file that the log came from
 * @var lineInfo_t::fileId
 * the id of the file the log came from
 * @var lineInfo_t::line
 * the line the log happened on
 * @var lineInfo_t::severity
 * the serverity of the log from trace(0) to critical(5)
 * @see logLevel
 * @var lineInfo_t::logtime
 * the time the log happened on
 * @var lineInfo_t::tid
 * the tid of thread that wrote the log
 * @var lineInfo_t::pid
 * the pid of the process that write the log(for multi process apps).
 */
typedef struct
{
#ifdef USE_FILENAME
	char fileName[PATH_MAX];
#endif
	int fileId;
	int line;
	logLevel severity;
	struct timespec logtime;
	pid_t tid;
	pid_t pid;
} lineInfo_t;

/**
 * @struct logInfo_t
 * @brief the log strucure to be passed to the log backend
 *
 * @var logInfo_t::metadata
 * all the context the is relevant for the log
 * @var msg
 * the formatted msg
 */
typedef struct
{
	lineInfo_t metadata;
	char msg[MAX_LOG_LEN];
} logInfo_t;

#ifdef USE_FILENAME

#define setFileName(filePath) .fileName = filePath,
#else
/**
 * @brief if USE_FILENAME is defined init the fileName to filePath other wise do nothing
 */
#define setFileName(filePath)
#endif

/**
 * @brief constract the log context.
 *
 * @param logDataName the name of the logData struct to create
 * @param severitryLevel the severitry of the log of type logLevel
 */
#define CONSTRACT_LOG_INFO(logDataName, severitryLevel)                                                                \
	logInfo_t logDataName = {                                                                                          \
		.metadata =                                                                                                    \
			{                                                                                                          \
					   setFileName(__FILE__).fileId = FILE_ID,                                                                \
					   .line = __LINE__,                                                                                      \
					   .severity = severitryLevel,                                                                            \
					   .tid = gettid(),                                                                                       \
					   .pid = getpid(),                                                                                       \
					   },                                                                                                         \
	};                                                                                                                 \
	clock_gettime(CLOCK_TO_USE, &logDataName.metadata.logtime);
