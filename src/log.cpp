
#define PRINT_FUNCTION(logData)                                                                                        \
	printf("%s from %s:%d\n", logData.msg, logData.metadata.fileName, logData.metadata.line);

#include "log.h"

#include "files.h"
#include "logServer/server.h"
#include "sockets.h"

#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define NAME "logger socket"

static struct sockaddr_un name;
static fd_t logWriteSock = INVALID_FD;
static fd_t efd = INVALID_FD;

static pid_t loggerPid = -1;
static pid_t fatherPid = -1;

THROWS static err_t createWriteLogSocket()
{
	err_t err = NO_ERRORCODE;
	RETHROW(createSocket(AF_UNIX, SOCK_DGRAM, 0, &logWriteSock));

	name.sun_family = AF_UNIX;
	strncpy(name.sun_path, NAME, 108);

cleanup:
	return err;
}

THROWS err_t initLogger()
{
	err_t err = NO_ERRORCODE;
	fd_t waitForLoggerEfd = {.fd = eventfd(0, 0)};
	uint64_t u = 0;
	ssize_t bytesRead = 0;
	efd.fd = eventfd(0, 0);

	CHECK(IS_VALID_FD(efd));
	CHECK(IS_VALID_FD(waitForLoggerEfd));

	loggerPid = fork();
	QUITE_CHECK(loggerPid != -1);
	if (loggerPid == 0)
	{
		initLogServer(NAME, (fd_t[2]){efd, waitForLoggerEfd});
	}
	else
	{

		fatherPid = getpid();
		RETHROW(safeRead(waitForLoggerEfd, &u, sizeof(u), &bytesRead));
		RETHROW(createWriteLogSocket());
	}

cleanup:
	return err;
}

THROWS err_t closeLogger()
{
	uint64_t u = 1;
	err_t err = NO_ERRORCODE;
	ssize_t bytesWritten = 0;

	// this is a destructor function that will run on exit or after main for both the main process and the logger
	// process and any other fork
	if (loggerPid == 0)
	{
		err = closeLogServer();
	}
	else if (fatherPid == getpid())
	{
		REWARN(safeWrite(efd, &u, sizeof(uint64_t), &bytesWritten));

		waitpid(loggerPid, NULL, 0);
	}
	else
	{
		LOG_TRACE("process with pid: {} closed", getpid());
	}

	return err;
}

err_t writeLog(logInfo lineData)
{
	err_t err = NO_ERRORCODE;
	ssize_t bytesWritten = 0;
	RETHROW(safeSendto(logWriteSock, &lineData, sizeof(logInfo), 0, (const struct sockaddr *)&name,
						sizeof(struct sockaddr_un), &bytesWritten));
	CHECK(bytesWritten == sizeof(logInfo));

cleanup:
	return err;
}
