#include <cstdint>
#include <sys/types.h>
#define PRINT_FUNCTION(logData) printLog(logData);

#include "files.h"
#include "log.h"
#include "sockets.h"

#include <err.h>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include <fcntl.h>
#include <pthread.h>
#include <string>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

static err_t printLog(const logInfo &logToPrint);

#define NAME "socket"

#define SECS_IN_DAY (24 * 60 * 60)

fd_t logWriteSock = INVALID_FD;
pthread_t loggerThread = -1;
fd_t listenSock = INVALID_FD;
struct sockaddr_un name;
fd_t efd = INVALID_FD;

constexpr const char logLevelShortMessage[] = {'T', 'D', 'I', 'W', 'E', 'C'};
THROWS static err_t getThreadName(const pid_t tid, char *buf, int bufSize)
{
	err_t err = NO_ERRORCODE;
	std::string commPath = fmt::format("/proc/self/task/{}/comm", tid);
	ssize_t threadNameSize = -1;

	fd_t fd = INVALID_FD;

	QUITE_RETHROW(safe_open(commPath.data(), 0, O_RDONLY, &fd));

	QUITE_RETHROW(safe_read(fd, buf, bufSize, &threadNameSize));
	QUITE_CHECK(threadNameSize != -1);
	buf[threadNameSize - 1] = '\0';

cleanup:
	if (IS_VALID_FD(fd))
	{
		safe_close(&fd);
	}
	return err;
}

THROWS static err_t createListenSocket()
{
	err_t err = NO_ERRORCODE;
	struct sockaddr_un serverName = {.sun_family = AF_UNIX, .sun_path = NAME};

	RETHROW(create_socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0, &listenSock));
	RETHROW(safe_bind(listenSock, (const struct sockaddr *)&serverName, sizeof(struct sockaddr_un)));

cleanup:
	if (IS_ERROR(err) && IS_VALID_FD(listenSock))
	{
		REWARN(safe_close(&listenSock));
	}
	return err;
}

THROWS static err_t createWriteLogSocket()
{
	err_t err = NO_ERRORCODE;
	RETHROW(create_socket(AF_UNIX, SOCK_DGRAM, 0, &logWriteSock));
	name.sun_family = AF_UNIX;
	strcpy(name.sun_path, NAME);

cleanup:
	return err;
}

static err_t printLog(const logInfo &logToPrint)
{
	err_t err = NO_ERRORCODE;
	char threadName[17] = {0};
	const char *fileName = "";
	std::string logRes = {0};
	ssize_t bytesWritten = 0;

#ifdef USE_FILENAME
	fileName = logToPrint.metadata.fileName;
#endif

	REWARN(getThreadName(logToPrint.metadata.tid, threadName, sizeof(threadName)));
	logRes = fmt::format("[{:%Y/%m/%d %T}.{:d}] {} ({}) {:<128s} {} from {}:{}({}:{}) id {}\n",
						 fmt::localtime(logToPrint.metadata.logtime.tv_sec), logToPrint.metadata.logtime.tv_nsec,
						 logLevelColors[logToPrint.metadata.severity],
						 logLevelShortMessage[logToPrint.metadata.severity], logToPrint.msg, CEND, fileName,
						 logToPrint.metadata.line, threadName, logToPrint.metadata.tid, logToPrint.metadata.fileId);
	RETHROW(safe_write({STDERR_FILENO}, logRes.data(), logRes.size(), &bytesWritten));
cleanup:
	return err;
}

err_t pollCalback(struct pollfd *fds, bool *shouldCountinue, void *ptr)
{
	err_t err = NO_ERRORCODE;
	ssize_t bytesRead = 0;
	logInfo *buf = (logInfo *)ptr;
	CHECK(buf != NULL);
	if (fds[0].revents & POLLIN)
	{
		RETHROW(safe_read(listenSock, buf, sizeof(logInfo), &bytesRead));
		CHECK(bytesRead == sizeof(logInfo));

		RETHROW(printLog(*buf));
	}
	if (fds[1].revents & POLLIN)
	{
		*shouldCountinue = false;
	}
cleanup:
	return err;
}

void *logServer(__attribute_maybe_unused__ void *args)
{
	err_t err = NO_ERRORCODE;
	logInfo buf;
	int nfds = 2;
	ssize_t bytesRead = 0;
	struct pollfd fds[2]{
		[0] = {.fd = listenSock.fd, .events = POLLIN},
		[1] = {.fd = efd.fd,		 .events = POLLIN},
	};

	RETHROW(safe_poll(fds, nfds, -1, pollCalback, &buf));

cleanup:
	REWARN(safe_read(listenSock, &buf, sizeof(logInfo), &bytesRead));
	while (bytesRead > 0)
	{
		printLog(buf);
		REWARN(safe_read(listenSock, &buf, sizeof(logInfo), &bytesRead));
	}

	return NULL;
}

THROWS err_t initLogger()
{
	err_t err = NO_ERRORCODE;
	efd.fd = eventfd(0, 0);
	CHECK(IS_VALID_FD(efd));
	RETHROW(createListenSocket());

	CHECK(pthread_create(&loggerThread, NULL, logServer, NULL) == 0);

	RETHROW(createWriteLogSocket());
cleanup:
	return err;
}

THROWS err_t closeLogger()
{
	uint64_t u = 1;
	err_t err = NO_ERRORCODE;
	ssize_t bytesWritten = 0;
	RETHROW(safe_write(efd, &u, sizeof(uint64_t), &bytesWritten));

	pthread_join(loggerThread, NULL);

cleanup:
	safe_close(&listenSock);
	CHECK(unlink(NAME) == 0);

	return err;
}

err_t writeLog(logInfo lineData)
{
	err_t err = NO_ERRORCODE;
	ssize_t bytesWritten = 0;
	RETHROW(safe_sendto(logWriteSock, &lineData, sizeof(logInfo), 0, (const struct sockaddr *)&name,
						sizeof(struct sockaddr_un), &bytesWritten));
	CHECK(bytesWritten == sizeof(logInfo));
cleanup:
	return err;
}
