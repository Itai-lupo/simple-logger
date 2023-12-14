
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

#define LOG_SERVER_NAME "logger socket"

static struct sockaddr_un logServerName;
static fd_t logWriteSock = INVALID_FD;
static fd_t killLogServerEfd = INVALID_FD;

static pid_t loggerPid = -1;
static pid_t fatherPid = -1;

THROWS static err_t createWriteLogSocket()
{
	err_t err = NO_ERRORCODE;

	RETHROW(createSocket(AF_UNIX, SOCK_DGRAM, 0, &logWriteSock));

	logServerName.sun_family = AF_UNIX;
	strncpy(logServerName.sun_path, LOG_SERVER_NAME, 108);

cleanup:
	return err;
}

THROWS static err_t closeLogApi()
{
	err_t err = NO_ERRORCODE;
	uint64_t u = 1;
	ssize_t bytesWritten = 0;

	RETHROW(safeWrite(killLogServerEfd, &u, sizeof(uint64_t), &bytesWritten));
	REWARN(safeClose(&logWriteSock));
	REWARN(safeClose(&killLogServerEfd));
	CHECK(waitpid(loggerPid, NULL, 0) == loggerPid);

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

	loggerPid = fork();
	QUITE_CHECK(loggerPid != -1);
	if (loggerPid == 0)
	{
		initLogServer(LOG_SERVER_NAME, killLogServerEfd, waitForLoggerEfd);
	}
	else
	{

		fatherPid = getpid();
		RETHROW(safeRead(waitForLoggerEfd, &u, sizeof(u), &bytesRead));
		RETHROW(safeClose(&waitForLoggerEfd));
		RETHROW(createWriteLogSocket());
	}

cleanup:
	return err;
}

THROWS err_t closeLogger()
{
	err_t err = NO_ERRORCODE;

	// this is a destructor function that will run on exit or after main for both the main process and the logger
	// process and any other fork
	if (loggerPid == 0)
	{
		RETHROW(closeLogServer());
	}
	else if (fatherPid == getpid())
	{
		RETHROW(closeLogApi());
	}
	else
	{
		LOG_TRACE("process with pid: {} closed", getpid());
	}

cleanup:
	return err;
}

err_t writeLog(logInfo_t lineData)
{
	err_t err = NO_ERRORCODE;
	ssize_t bytesWritten = 0;
	RETHROW(safeSendto(logWriteSock, &lineData, sizeof(logInfo_t), 0, (const struct sockaddr *)&logServerName,
					   sizeof(struct sockaddr_un), &bytesWritten));
	CHECK(bytesWritten == sizeof(logInfo_t));

cleanup:
	return err;
}
