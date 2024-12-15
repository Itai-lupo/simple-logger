#include "allocators/sharedMemoryPool.h"
#include "types/safeQueue.h"
#define PRINT_FUNCTION(logData) printf("%s from %d:%d\n", logData->msg, logData->metadata.fileId, logData->metadata.line);
#include "log.h"

#include "files.h"
#include "logServer/server.h"
#include "sinks/printToSinks.h"

#include <asm-generic/errno-base.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/core.h>
#include <fmt/format.h>

#define LOG_SERVER_NAME "/log queue"

safeQueue *sharedLogsQueue = NULL;

memoryAllocator *loggerAllocator = NULL;

static fd_t killLogServerEfd = INVALID_FD;

static pid_t loggerPid = -1;
static pid_t fatherPid = -1;;

THROWS err_t initLogger()
{
	err_t err = NO_ERRORCODE;
	fd_t waitForLoggerEfd = {.fd = eventfd(0, 0)};
	uint64_t u = 0;
	ssize_t bytesRead = 0;
	killLogServerEfd.fd = eventfd(0, 0);

	CHECK(IS_VALID_FD(killLogServerEfd));
	CHECK(IS_VALID_FD(waitForLoggerEfd));
	
    RETHROW(initSafeQueue(sizeof(logInfo_t), getSharedAllocator(), 100, &sharedLogsQueue))
	
	RETHROW(initSinks());

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

	CHECK(waitpid(loggerPid, NULL, 0) == loggerPid);

	RETHROW(closeSinks());
cleanup:
	return err;
}

err_t writeLog(logInfo_t *lineData)
{
	err_t err = NO_ERRORCODE;
	CHECK(lineData != NULL);
	RETHROW(safeQueuePush(sharedLogsQueue, (void*)lineData));

cleanup:
	errno = 0;
	return err;
}
