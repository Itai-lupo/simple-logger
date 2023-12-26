#include "processes.h"
#include <asm-generic/errno-base.h>
#define PRINT_FUNCTION(logData)                                                                                        \
	printf("%s from %s:%d\n", logData.msg, logData.metadata.fileName, logData.metadata.line);

#include "log.h"

#include "files.h"
#include "logServer/server.h"
#include "sockets.h"

#include <fcntl.h>
#include <mqueue.h>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>

#define LOG_SERVER_NAME "/log queue"

#define SECS_IN_DAY (24 * 60 * 60)

#define LOG_FMT "[{:^16s}:{:d} {:%Y/%m/%d %T}.{:d}] {} ({}) {:<128s}{} from {:s}:{}:{}\t({:^16s}:{}) \n"

static fd_t killLogServerEfd = INVALID_FD;

static pid_t loggerPid = -1;
static pid_t fatherPid = -1;
static mqd_t writeQueue = -1;

constexpr const char logLevelShortMessage[] = {'T', 'D', 'I', 'W', 'E', 'C'};

THROWS static err_t createWriteLogSocket()
{
	err_t err = NO_ERRORCODE;

	writeQueue = mq_open("/log queue", O_WRONLY | O_NONBLOCK);
	CHECK(writeQueue != -1);

cleanup:
	return err;
}

THROWS err_t initLogger()
{
	err_t err = NO_ERRORCODE;
	fd_t waitForLoggerEfd = {.fd = eventfd(0, 0)};
	uint64_t u = 0;
	ssize_t bytesRead = 0;
	killLogServerEfd.fd = eventfd(0, 0);

	CHECK(IS_VALID_FD(killLogServerEfd));
	CHECK(IS_VALID_FD(waitForLoggerEfd));

	for (size_t i = 0; i < 1; i++)
	{

		loggerPid = fork();
		CHECK(loggerPid != -1);
		if (loggerPid == 0)
		{
			initLogServer(LOG_SERVER_NAME, killLogServerEfd, waitForLoggerEfd);
		}
	}

	fatherPid = getpid();
	RETHROW(safeRead(waitForLoggerEfd, &u, sizeof(u), &bytesRead));
	RETHROW(safeClose(&waitForLoggerEfd));
	RETHROW(createWriteLogSocket());

cleanup:
	return err;
}

static err_t printLog(const logInfo_t &logToPrint)
{
	err_t err = NO_ERRORCODE;
	char threadName[17] = {0};
	char processName[17] = {0};
	const char *fileName = "";
	std::string logRes = {0};
	ssize_t bytesWritten = 0;

#ifdef USE_FILENAME
	fileName = logToPrint.metadata.fileName;
#endif

	REWARN(getThreadName(logToPrint.metadata.pid, logToPrint.metadata.tid, threadName, sizeof(threadName)));
	REWARN(getProcessName(logToPrint.metadata.pid, processName, sizeof(processName)));

	logRes =
		fmt::format(LOG_FMT, processName, logToPrint.metadata.pid, fmt::localtime(logToPrint.metadata.logtime.tv_sec),
					logToPrint.metadata.logtime.tv_nsec, logLevelColors[logToPrint.metadata.severity],
					logLevelShortMessage[logToPrint.metadata.severity], logToPrint.msg, CEND, fileName,
					logToPrint.metadata.line, logToPrint.metadata.fileId, threadName, logToPrint.metadata.tid);

	RETHROW(safeWrite({(logToPrint.metadata.severity > logLevel::warnLevel) ? STDERR_FILENO : STDOUT_FILENO},
					  logRes.data(), logRes.size(), &bytesWritten));

cleanup:
	return err;
}

THROWS err_t closeLogger()
{
	err_t err = NO_ERRORCODE;
	uint64_t u = 1;
	ssize_t bytesWritten = 0;

	RETHROW(safeWrite(killLogServerEfd, &u, sizeof(uint64_t), &bytesWritten));
	REWARN(safeClose(&killLogServerEfd));
	WARN(mq_close(writeQueue) == 0);
	writeQueue = -1;
	CHECK(waitpid(loggerPid, NULL, 0) == loggerPid);

cleanup:
	return err;
}

err_t writeLog(logInfo_t lineData)
{
	err_t err = NO_ERRORCODE;
	errno = 0;
	int res = mq_send(writeQueue, (char *)&lineData, sizeof(logInfo_t), 0);
	CHECK(res == 0 || errno == EAGAIN);
	if (errno == EAGAIN) [[unlikely]]
	{
		RETHROW(printLog(lineData));
	}

cleanup:
	errno = 0;
	return err;
}
