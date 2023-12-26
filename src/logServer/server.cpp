
#define PRINT_FUNCTION(logData) printLog(logData);
#include "log.h"

#include "logServer/server.h"

#include "defines/colors.h"
#include "processes.h"
#include "types/err_t.h"

#include "files.h"
#include "sockets.h"

#include <fcntl.h>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include <fcntl.h>
#include <mqueue.h>
#include <string>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define QUEUE_MAXMSG 10					/* Maximum number of messages. */
#define QUEUE_MSGSIZE sizeof(logInfo_t) /* Length of message. */
#define QUEUE_ATTR_INITIALIZER                                                                                         \
	((struct mq_attr){                                                                                                 \
		.mq_flags = 0,                                                                                                 \
		.mq_maxmsg = QUEUE_MAXMSG,                                                                                     \
		.mq_msgsize = QUEUE_MSGSIZE,                                                                                   \
		.mq_curmsgs = 0,                                                                                               \
		{0},                                                                                                           \
	})

#define SECS_IN_DAY (24 * 60 * 60)

#define LOG_FMT "[{:^16s}:{:d} {:%Y/%m/%d %T}.{:d}] {} ({}) {:<128s}{} from {:s}:{}:{}\t({:^16s}:{}) \n"

static fd_t dieOnEventfd = INVALID_FD;

static mqd_t logsQueue = -1;

constexpr const char logLevelShortMessage[] = {'T', 'D', 'I', 'W', 'E', 'C'};

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
	RETHROW_NOHANDLE_TRACE(
		getThreadName(logToPrint.metadata.pid, logToPrint.metadata.tid, threadName, sizeof(threadName)),
		"failed to get name of thread with tid {} ", logToPrint.metadata.tid);

	RETHROW_NOHANDLE_TRACE(getProcessName(logToPrint.metadata.pid, processName, sizeof(processName)),
						   "failed to get name of process with pid {} ", logToPrint.metadata.pid);

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

THROWS static err_t createListenSocket(const char *const listenSockName)
{
	err_t err = NO_ERRORCODE;

	struct mq_attr attr = QUEUE_ATTR_INITIALIZER;

	logsQueue = mq_open(listenSockName, O_RDONLY | O_CREAT | O_NONBLOCK, 0600, &attr);
	CHECK(logsQueue > 0);

cleanup:
	return err;
}

err_t static pollCalback(struct pollfd *fds, bool *shouldCountinue, [[maybe_unused]] void *ptr)
{
	err_t err = NO_ERRORCODE;
	uint32_t prio = 0;

	logInfo_t buf;

	if (fds[0].revents & POLLIN)
	{
		CHECK(mq_receive(logsQueue, (char *)&buf, sizeof(logInfo_t), &prio) >= 0);
		RETHROW(printLog(buf));
	}
	else if (fds[1].revents & POLLIN)
	{
		// if there are no more data to read from fds[0](the log socket) and there is an exit event(data to read from
		// fds[1]_ then exit
		*shouldCountinue = false;
	}
cleanup:
	return err;
}

err_t static logServer()
{
	err_t err = NO_ERRORCODE;
	int nfds = 2;
	struct pollfd fds[2]{
		[0] = {.fd = logsQueue,		.events = POLLIN},
		[1] = {.fd = dieOnEventfd.fd, .events = POLLIN},
	};

	RETHROW(safePpoll(fds, nfds, NULL, NULL, pollCalback, NULL));

cleanup:
	return err;
}

err_t static closeLogServer()
{
	err_t err = NO_ERRORCODE;

	WARN(mq_close(logsQueue) == 0);
	logsQueue = -1;
	REWARN(safeClose(&dieOnEventfd));

	return err;
}

__attribute__((__noreturn__)) void initLogServer(const char *const listenSockName, fd_t killLogServerEfd,
												 fd_t waitForLoggerEfd)
{
	err_t err = NO_ERRORCODE;
	ssize_t bytesWritten = 0;
	uint64_t u = 1;

	prctl(PR_SET_NAME, (unsigned long)"logger", 0, 0, 0);

	dieOnEventfd = killLogServerEfd;

	RETHROW(createListenSocket(listenSockName));

	RETHROW(safeWrite(waitForLoggerEfd, &u, sizeof(uint64_t), &bytesWritten));
	REWARN(safeClose(&waitForLoggerEfd));

	RETHROW(logServer());

cleanup:
	REWARN(closeLogServer());

	exit((int)err.errorCode);
}
