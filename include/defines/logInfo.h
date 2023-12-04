#pragma once

#include "defines/logLevels.h"

#include <time.h>

#define MAX_LOG_LEN 128

#ifndef CLOCK_TO_USE
#define CLOCK_TO_USE CLOCK_REALTIME_COARSE
#endif //! CLOCK_TO_USE

#ifndef FILE_ID
#define FILE_ID -1
#endif // !FILE_ID

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
} lineInfo;

typedef struct
{
	lineInfo metadata;
	int msgSize;
	char msg[MAX_LOG_LEN];
} logInfo;

#ifdef USE_FILENAME
#define setFileName(filePath) .fileName = filePath,
#else
#define setFileName(filePath)
#endif

#define CONSTRACT_LOG_INFO(logDataName, severitryLevel)                                                                \
	logInfo logDataName = {                                                                                            \
		.metadata =                                                                                                    \
			{                                                                                                          \
					   setFileName(__FILE__).fileId = FILE_ID,                                                                \
					   .line = __LINE__,                                                                                      \
					   .severity = severitryLevel,                                                                            \
					   .tid = gettid(),                                                                                       \
					   },                                                                                                         \
		.msgSize = 0,                                                                                                  \
	};                                                                                                                 \
	clock_gettime(CLOCK_TO_USE, &logDataName.metadata.logtime);
