
#define PRINT_FUNCTION(logData) printLog(logData);
#include "log.h"

#include "logServer/server.h"

#include "defines/colors.h"
#include "types/err_t.h"

#include "files.h"
#include "sockets.h"

#include <fcntl.h>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include <fcntl.h>
#include <string>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define SECS_IN_DAY (24 * 60 * 60)

#define THREAD_NAME_PATH_FMT "/proc/{}/task/{}/comm"
#define PROCESS_NAME_PATH_FMT "/proc/{}/comm"
#define LOG_FMT "[{:^16s}:{:d} {:%Y/%m/%d %T}.{:d}] {} ({}) {:<128s}{} from {:s}:{}:{}\t({:^16s}:{}) \n"

static fd_t listenSock = INVALID_FD;
static fd_t dieOnEventfd = INVALID_FD;
static const char *listenSockName = NULL;

constexpr const char logLevelShortMessage[] = {'T', 'D', 'I', 'W', 'E', 'C'};

THROWS static err_t getThreadName(const pid_t pid, const pid_t tid, char *buf, int bufSize)
{
	err_t err = NO_ERRORCODE;
	std::string commPath = fmt::format(THREAD_NAME_PATH_FMT, pid, tid);
	ssize_t threadNameSize = -1;

	fd_t fd = INVALID_FD;

	QUITE_RETHROW(safeOpen(commPath.data(), 0, O_RDONLY, &fd));
	QUITE_RETHROW(safeRead(fd, buf, bufSize, &threadNameSize));
	buf[threadNameSize - 1] = '\0';

cleanup:
	if (IS_VALID_FD(fd))
	{
		safeClose(&fd);
	}

	return err;
}

THROWS static err_t getProcessName(const pid_t pid, char *buf, int bufSize)
{
	err_t err = NO_ERRORCODE;
	std::string commPath = fmt::format(PROCESS_NAME_PATH_FMT, pid);
	ssize_t processNameSize = -1;

	fd_t fd = INVALID_FD;

	QUITE_RETHROW(safeOpen(commPath.data(), 0, O_RDONLY, &fd));
	QUITE_RETHROW(safeRead(fd, buf, bufSize, &processNameSize));
	buf[processNameSize - 1] = '\0';

cleanup:
	if (IS_VALID_FD(fd))
	{
		safeClose(&fd);
	}

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

	RETHROW(safeWrite({STDERR_FILENO}, logRes.data(), logRes.size(), &bytesWritten));
cleanup:
	return err;
}

THROWS static err_t createListenSocket(const char *const listenSockName)
{
	err_t err = NO_ERRORCODE;
	struct sockaddr_un serverName = {0, {0}};
	serverName.sun_family = AF_UNIX;
	strncpy(serverName.sun_path, listenSockName, 108);

	RETHROW(createSocket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0, &listenSock));
	RETHROW(safeBind(listenSock, (const struct sockaddr *)&serverName, sizeof(struct sockaddr_un)));

cleanup:
	return err;
}

err_t pollCalback(struct pollfd *fds, bool *shouldCountinue, void *ptr)
{
	err_t err = NO_ERRORCODE;
	ssize_t bytesRead = 0;
	logInfo_t *buf = (logInfo_t *)ptr;
	CHECK(buf != NULL);
	if (fds[0].revents & POLLIN)
	{
		RETHROW(safeRead(listenSock, buf, sizeof(logInfo_t), &bytesRead));
		CHECK(bytesRead == sizeof(logInfo_t));

		RETHROW(printLog(*buf));
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

err_t logServer()
{
	err_t err = NO_ERRORCODE;
	logInfo_t buf;
	int nfds = 2;
	struct pollfd fds[2]{
		[0] = {.fd = listenSock.fd,	.events = POLLIN},
		[1] = {.fd = dieOnEventfd.fd, .events = POLLIN},
	};

	RETHROW(safePpoll(fds, nfds, NULL, NULL, pollCalback, &buf));

cleanup:

	return err;
}

__attribute__((__noreturn__)) void initLogServer(const char *const _listenSockName, fd_t killLogServerEfd,
												 fd_t waitForLoggerEfd)
{
	err_t err = NO_ERRORCODE;
	ssize_t bytesWritten = 0;
	uint64_t u = 1;

	prctl(PR_SET_NAME, (unsigned long)"logger", 0, 0, 0);

	dieOnEventfd = killLogServerEfd;
	listenSockName = _listenSockName;

	RETHROW(createListenSocket(listenSockName));

	RETHROW(safeWrite(waitForLoggerEfd, &u, sizeof(uint64_t), &bytesWritten));
	REWARN(safeClose(&waitForLoggerEfd));

	RETHROW(logServer());
cleanup:
	exit((int)err.errorCode);
}

err_t closeLogServer()
{
	err_t err = NO_ERRORCODE;

	REWARN(safeClose(&listenSock));
	REWARN(safeClose(&dieOnEventfd));

	WARN(unlink(listenSockName) == 0);

	return err;
}
