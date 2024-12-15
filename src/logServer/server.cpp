#include "types/logInfo.h"
#include "types/safeQueue.h"
#include <cstdint>
#include <sys/types.h>

#define PRINT_FUNCTION(logData) printToSinks(logData);
#include "defaultTrace.h"

#include "files.h"
#include "logServer/server.h"
#include "sinks/printToSinks.h"
#include "types/err_t.h"

#include <fcntl.h>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include <fcntl.h>
#include <mqueue.h>
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

static fd_t dieOnEventfd = INVALID_FD;

static mqd_t logsQueue = -1;


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
	ssize_t bytesRead = 0;
	uint64_t logsCount = 0;
	logInfo_t buf;

	if (fds[0].revents & POLLIN)
	{
		bytesRead = 0;
		RETHROW(safeRead( sharedLogsQueue->eventFd, &logsCount, sizeof(uint64_t), &bytesRead));
		for (uint64_t i = 0; i < logsCount; i++) {
			RETHROW(safeQueuePop(sharedLogsQueue, &buf, sizeof(logInfo_t)));
			RETHROW(printToSinks(&buf));
		}
	}
	
	if (fds[1].revents & POLLIN)
	{
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
		[0] = {.fd = sharedLogsQueue->eventFd.fd,		.events = POLLIN, .revents = 0},
		[1] = {.fd = dieOnEventfd.fd, .events = POLLIN, .revents = 0},
	};

	RETHROW(safePpoll(fds, nfds, NULL, NULL, pollCalback, NULL));

cleanup:
	REWARN(err);
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
