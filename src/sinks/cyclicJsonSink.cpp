#include "sinks/cyclicJsonSink.h"
#include "defaultTrace.h"
// #include "log.h"

#include "err.h"
#include "files.h"
#include "types/fd_t.h"

#include <bits/types/struct_timespec.h>
#include <climits>
#include <ctime>
#include <fcntl.h>
#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#define LOG_FMT                                                                                                        \
	"{{ "                                                                                                              \
	"\"processn name\": \"{processName:s}\", "                                                                         \
	"\"pid\": {pid:d}, "                                                                                               \
	"\"timestamp\": \"{time:%Y-%m-%d %T}.{timeNs:d}\", "                                                               \
	"\"level\": \"{severity:c}\", "                                                                                    \
	"\"message\": \"{msg:s}\", "                                                                                       \
	"\"file name\": \"{fileName:s}\", "                                                                                \
	"\"line\": {line:d}, "                                                                                             \
	"\"file id\": {fileId:d}, "                                                                                        \
	"\"thread name\": \"{threadName:s}\", "                                                                            \
	"\"tid\": {tid:d}"                                                                                                 \
	" }}\n"

using namespace fmt::literals;

// fd_t file = INVALID_FD;
static fd_t memfd = INVALID_FD;

err_t initCyclicJsonSink()
{
	err_t err = NO_ERRORCODE;
	std::string filePath = fmt::format("./tmp/jsonLog.txt");

	memfd.fd = memfd_create("shared memory pool", 0);
	QUITE_CHECK(IS_VALID_FD(memfd));
	// QUITE_RETHROW(safeOpen(filePath.data(), O_CREAT | O_RDWR, 0x0777, &file));

cleanup:
	return err;
}

err_t cyclicJsonSink(logInfo_t logToPrint, char *processName, char *threadName)
{
	err_t err = NO_ERRORCODE;
	const char *fileName = "";
	std::string logRes = {0};
	ssize_t bytesWritten = 0;

#ifdef USE_FILENAME
	fileName = logToPrint.metadata.fileName;
#endif
	umask(0);
	logRes = fmt::format(
		FMT_COMPILE(LOG_FMT), "processName"_a = processName, "pid"_a = logToPrint.metadata.pid,
		"time"_a = fmt::localtime(logToPrint.metadata.logtime.tv_sec), "timeNs"_a = logToPrint.metadata.logtime.tv_nsec,
		"severity"_a = logLevelShortMessage[logToPrint.metadata.severity], "msg"_a = logToPrint.msg,
		"fileName"_a = fileName, "line"_a = logToPrint.metadata.line, "fileId"_a = logToPrint.metadata.fileId,
		"threadName"_a = threadName, "tid"_a = logToPrint.metadata.tid);

	QUITE_RETHROW(safeWrite(memfd, logRes.data(), logRes.size(), &bytesWritten));

cleanup:
	return err;
}

err_t closeCyclicJsonSink()
{
	err_t err = NO_ERRORCODE;
	timespec time;
	std::string filePath;
	fd_t resFile = INVALID_FD;
	off_t offset = 0;

	clock_gettime(CLOCK_REALTIME_COARSE, &time);
	filePath = fmt::format("./logs/log-{:%Y-%m-%d-%T}.json", fmt::localtime(time.tv_sec));
	rename("./logs/new-log.json", filePath.data());

	RETHROW(safeOpen("./logs/new-log.json", O_CREAT | O_WRONLY, 0x0777, &resFile));

	sendfile(resFile.fd, memfd.fd, &offset, UINT32_MAX);

	if (IS_VALID_FD(memfd))
	{
		RETHROW(safeClose(&memfd));
	}

cleanup:
	return err;
}
