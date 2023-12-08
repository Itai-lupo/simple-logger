#include <cstdint>
#define PRINT_FUNCTION(logData) printLog(logData);

#include "log.h"

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

using fd_t = int;

fd_t logWriteSock = -1;
pthread_t loggerThread = -1;
fd_t listenSock = -1;
struct sockaddr_un name;
fd_t efd = -1;

constexpr const char logLevelShortMessage[] = {'T', 'D', 'I', 'W', 'E', 'C'};

THROWS static err_t safe_open(const char *const path, int flags, mode_t mode, fd_t *fd)
{
	err_t err = NO_ERRORCODE;

	QUITE_CHECK(path != NULL);
	QUITE_CHECK(fd != NULL);
	QUITE_CHECK(*fd == -1);

	*fd = open(path, flags, mode);
	QUITE_CHECK(*fd != -1);
cleanup:
	return err;
}

THROWS static err_t safe_read(fd_t fd, void *buf, size_t size, ssize_t *outSize)
{
	err_t err = NO_ERRORCODE;
	ssize_t tempOutSize = 0;

	QUITE_CHECK(outSize != NULL);
	QUITE_CHECK(buf != NULL);

	tempOutSize = read(fd, buf, size);
	QUITE_CHECK(tempOutSize != -1);

cleanup:
	*outSize = tempOutSize;
	return err;
}

THROWS static err_t safe_write(fd_t fd, void *buf, size_t size, ssize_t *bytesWritten)
{
	err_t err = NO_ERRORCODE;
	ssize_t tempBytesWritten = 0;

	QUITE_CHECK(bytesWritten != NULL);
	QUITE_CHECK(buf != NULL);

	tempBytesWritten = write(fd, buf, size);
	QUITE_CHECK(tempBytesWritten != -1);

cleanup:
	*bytesWritten = tempBytesWritten;
	return err;
}

static err_t safe_close(fd_t *fd)
{
	err_t err = NO_ERRORCODE;
	QUITE_CHECK(close(*fd));

cleanup:
	*fd = -1;
	return err;
}
THROWS static err_t safe_poll(struct pollfd *fds, int ntfds, int timeout,
							  err_t callback(pollfd *events, bool *shouldCountinue, void *ptr), void *ptr)
{
	err_t err = NO_ERRORCODE;
	bool shouldCountinue = true;
	int pollNum = 0;

	CHECK(callback != NULL);
	CHECK(fds != NULL);
	CHECK(ntfds != 0);
	while (shouldCountinue)
	{
		pollNum = poll(fds, ntfds, timeout);
		if (pollNum > 0)
		{
			callback(fds, &shouldCountinue, ptr);
		}
	}

cleanup:
	return err;
}

THROWS static err_t safe_sendto(fd_t sd, const void *const buf, size_t size, int flags, const struct sockaddr *destAddr,
								socklen_t addrlen, ssize_t *bytesWritten)
{
	err_t err = NO_ERRORCODE;
	ssize_t tempBytesWritten = 0;

	QUITE_CHECK(sd != -1);
	QUITE_CHECK(destAddr != NULL);
	QUITE_CHECK(bytesWritten != NULL);
	QUITE_CHECK(buf != NULL);

	tempBytesWritten = sendto(sd, buf, size, flags, destAddr, addrlen);
	QUITE_CHECK(tempBytesWritten != -1);

cleanup:
	*bytesWritten = tempBytesWritten;
	return err;
}

THROWS static err_t create_socket(int domain, int type, int protocal, fd_t *sd)
{
	err_t err = NO_ERRORCODE;
	QUITE_CHECK(sd != NULL);
	QUITE_CHECK(*sd == -1);

	*sd = socket(domain, type, protocal);
	QUITE_CHECK(*sd != -1);

cleanup:
	return err;
}

THROWS static err_t safe_bind(fd_t sd, const struct sockaddr *const addr, socklen_t addrlen)
{
	err_t err = NO_ERRORCODE;
	QUITE_CHECK(sd != -1);
	QUITE_CHECK(addr != NULL);

	CHECK(bind(sd, addr, addrlen) == 0);
cleanup:
	return err;
}

THROWS static err_t getThreadId(const pid_t tid, char *buf, int bufSize)
{
	err_t err = NO_ERRORCODE;
	std::string commPath = fmt::format("/proc/self/task/{}/comm", tid);
	ssize_t threadNameSize = -1;

	fd_t fd = -1;

	QUITE_RETHROW(safe_open(commPath.data(), 0, O_RDONLY, &fd));

	QUITE_RETHROW(safe_read(fd, buf, bufSize, &threadNameSize));
	QUITE_CHECK(threadNameSize != -1);
	buf[threadNameSize - 1] = '\0';

cleanup:
	if (fd != -1)
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
	if (IS_ERROR(err) && listenSock != -1)
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

	REWARN(getThreadId(logToPrint.metadata.tid, threadName, sizeof(threadName)));
	logRes = fmt::format("[{:%Y/%m/%d %T}.{:d}] {} ({}) {:<128s} {} from {}:{}({}:{}) id {}\n",
						 fmt::localtime(logToPrint.metadata.logtime.tv_sec), logToPrint.metadata.logtime.tv_nsec,
						 logLevelColors[logToPrint.metadata.severity],
						 logLevelShortMessage[logToPrint.metadata.severity], logToPrint.msg, CEND, fileName,
						 logToPrint.metadata.line, threadName, logToPrint.metadata.tid, logToPrint.metadata.fileId);
	RETHROW(safe_write(STDERR_FILENO, logRes.data(), logRes.size(), &bytesWritten));
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
	struct pollfd fds[2]{
		[0] = {.fd = listenSock, .events = POLLIN},
		[1] = {.fd = efd,		  .events = POLLIN},
	};

	RETHROW(safe_poll(fds, nfds, -1, pollCalback, &buf));

cleanup:
	while (read(listenSock, &buf, sizeof(logInfo)) > 0)
	{
		printLog(buf);
	}

	return NULL;
}

THROWS err_t initLogger()
{
	err_t err = NO_ERRORCODE;
	efd = eventfd(0, 0);
	CHECK(efd != -1);
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
