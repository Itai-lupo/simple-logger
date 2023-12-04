
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

#define NAME "socket"

#define SECS_IN_DAY (24 * 60 * 60)

int logWriteSock = -1;
pthread_t loggerThread = -1;
int listenSock = -1;
struct sockaddr_un name;
int efd = -1;

constexpr const char logLevelShortMessage[] = {'T', 'D', 'I', 'W', 'E', 'C'};

static err_t getThreadId(const pid_t tid, char *buf, int bufSize)
{
	std::string commPath = fmt::format("/proc/self/task/{}/comm", tid);

	int fd = open(commPath.data(), 0, O_RDONLY);
	int threadNameSize = read(fd, buf, bufSize);
	close(fd);
	buf[threadNameSize - 1] = '\0';
	return NO_ERRORCODE;
}

static err_t createListenSocket()
{

	struct sockaddr_un serverName;

	unlink(NAME);
	listenSock = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);

	serverName.sun_family = AF_UNIX;
	strcpy(serverName.sun_path, NAME);

	if (bind(listenSock, (const struct sockaddr *)&serverName, sizeof(struct sockaddr_un)))
	{
		perror("could not start logger");
		exit(1);
	}

	return NO_ERRORCODE;
}

static err_t createWriteLogSocket()
{
	logWriteSock = socket(AF_UNIX, SOCK_DGRAM, 0);

	name.sun_family = AF_UNIX;
	strcpy(name.sun_path, NAME);

	return NO_ERRORCODE;
}

static err_t printLog(const logInfo &logToPrint)
{

	char threadName[17] = {0};
	const char *fileName = "";
	std::string logRes = {0};

#ifdef USE_FILENAME
	fileName = logToPrint.metadata.fileName;
#endif

	getThreadId(logToPrint.metadata.tid, threadName, sizeof(threadName));
	logRes = fmt::format("[{:%Y/%m/%d %T}.{:d}] {} ({}) {:<128s} {} from {}:{}({}:{}) id {}\n",
						 fmt::localtime(logToPrint.metadata.logtime.tv_sec), logToPrint.metadata.logtime.tv_nsec,
						 logLevelColors[logToPrint.metadata.severity],
						 logLevelShortMessage[logToPrint.metadata.severity], logToPrint.msg, CEND, fileName,
						 logToPrint.metadata.line, threadName, logToPrint.metadata.tid, logToPrint.metadata.fileId);
	write(STDERR_FILENO, logRes.data(), logRes.size());

	return NO_ERRORCODE;
}

void *logServer(__attribute_maybe_unused__ void *args)
{
	logInfo buf;
	bool shouldRun = true;
	int nfds = 2;
	int pollNum = 0;
	struct pollfd fds[2]{
		[0] = {.fd = listenSock, .events = POLLIN},
			[1] = {.fd = efd,		  .events = POLLIN}
	   };

	shouldRun = true;
	while (shouldRun)
	{
		pollNum = poll(fds, nfds, -1);
		if (pollNum > 0 && (fds[0].revents & POLLIN))
		{
			read(listenSock, &buf, sizeof(logInfo));
			printLog(buf);
		}
		if (pollNum > 0 && (fds[1].revents & POLLIN))
		{
			shouldRun = false;
			printf("a %d\n", shouldRun);
		}
	}

	while (read(listenSock, &buf, sizeof(logInfo)) > 0)
	{
		printLog(buf);
	}

	return NULL;
}

err_t initLogger()
{
	efd = eventfd(0, 0);
	if (efd == -1)
	{
		exit(EXIT_FAILURE);
	}

	createListenSocket();

	pthread_create(&loggerThread, NULL, logServer, NULL);

	createWriteLogSocket();
	return NO_ERRORCODE;
}

THROWS err_t closeLogger()
{
	uint64_t u = 1;
	write(efd, &u, sizeof(uint64_t));
	pthread_join(loggerThread, NULL);

	close(listenSock);
	unlink(NAME);

	return NO_ERRORCODE;
}

err_t writeLog(logInfo lineData)
{
	sendto(logWriteSock, &lineData, sizeof(logInfo), 0, (const struct sockaddr *)&name, sizeof(struct sockaddr_un));
    return NO_ERRORCODE;
}
