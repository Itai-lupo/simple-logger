#include "sinks/stdColor.h"

#include "defaultTrace.h"
#include "err.h"
#include "files.h"

#include "defines/colors.h"
#include "types/logLevels.h"

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include <fcntl.h>

#define LOG_FMT                                                                                                        \
	"{i:d} [{processName:^16s}:{pid:d} {time:%Y/%m/%d %T}.{timeNs:d}] {colorStart:s} ({severity:c}) "                  \
	"{msg:<128s}{colorEnd:s} from {fileName:s}:{line:d}:{fileId:d}\t({threadName:^16s}:{tid:d}) \n"

using namespace fmt::literals;
int i = 0;
err_t initStdColorSink()
{
	err_t err = NO_ERRORCODE;
	// QUITE_CHECK(fcntl(STDOUT_FILENO, F_SETFL, fcntl(STDOUT_FILENO, F_GETFL) | O_NONBLOCK) == 0);
	// QUITE_CHECK(fcntl(STDERR_FILENO, F_SETFL, fcntl(STDERR_FILENO, F_GETFL) | O_NONBLOCK) == 0);

	// cleanup:
	return err;
}

err_t stdColorSink(logInfo_t *logToPrint, char *processName, char *threadName)
{
	err_t err = NO_ERRORCODE;
	const char *fileName = "";
	std::string logRes = {0};
	ssize_t bytesWritten = 0;

#ifdef USE_FILENAME
	fileName = logToPrint->metadata.fileName;
#endif

	logRes = fmt::format(
		FMT_COMPILE(LOG_FMT), "i"_a = i, "processName"_a = processName, "pid"_a = logToPrint->metadata.pid,
		"time"_a = fmt::localtime(logToPrint->metadata.logtime.tv_sec), "timeNs"_a = logToPrint->metadata.logtime.tv_nsec,
		"colorStart"_a = logLevelColors[logToPrint->metadata.severity],
		"severity"_a = logLevelShortMessage[logToPrint->metadata.severity], "msg"_a = logToPrint->msg,
		"colorEnd"_a = CEND, "fileName"_a = fileName, "line"_a = logToPrint->metadata.line,
		"fileId"_a = logToPrint->metadata.fileId, "threadName"_a = threadName, "tid"_a = logToPrint->metadata.tid);
	i++;
	QUITE_RETHROW(safeWrite({(logToPrint->metadata.severity > logLevel::warnLevel) ? STDERR_FILENO : STDOUT_FILENO},
							logRes.data(), logRes.size(), &bytesWritten));

cleanup:
	if (err.errorCode == EAGAIN || err.errorCode == EWOULDBLOCK)
	{
		return NO_ERRORCODE;
	}

	return err;
}

err_t closeStdColorSink()
{
	err_t err = NO_ERRORCODE;
	QUITE_CHECK(fcntl(STDOUT_FILENO, F_SETFL, fcntl(STDOUT_FILENO, F_GETFL) & ~O_NONBLOCK) == 0);
	QUITE_CHECK(fcntl(STDERR_FILENO, F_SETFL, fcntl(STDERR_FILENO, F_GETFL) & ~O_NONBLOCK) == 0);

cleanup:
	return err;
}
