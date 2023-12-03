
#include "log.h"

#include <cstdio>
#include <cwchar>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include <fcntl.h>
#include <pthread.h>
#include <string>
#include <string_view>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define NAME "socket"

#define SECS_IN_DAY (24 * 60 * 60)

int logWriteSock = 0;
bool shouldRun = true;
pthread_t loggerThread = 0;
int listenSock = 0;
struct sockaddr_un name;

constexpr const char logLevelShortMessage[] = {'T', 'D', 'I', 'W', 'E', 'C'};

static int getThreadId(const pid_t tid, char *buf, int bufSize)
{
	std::string commPath = fmt::format("/proc/self/task/{}/comm", tid);

	int fd = open(commPath.data(), 0, O_RDONLY);
	int threadNameSize = read(fd, buf, bufSize);
	close(fd);
	buf[threadNameSize - 1] = '\0';
	return 0;
}

static int createListenSocket()
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

	return 0;
}

static int createWriteLogSocket()
{
	logWriteSock = socket(AF_UNIX, SOCK_DGRAM, 0);

	name.sun_family = AF_UNIX;
	strcpy(name.sun_path, NAME);

	return 0;
}

static void printLog(const logInfo &logToPrint)
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
}

void *logServer(__attribute_maybe_unused__ void *args)
{
	logInfo buf;
	int nfds = 1;
	ssize_t bytesRead = 0;
	int pollNum = 0;
	struct pollfd fds[1] = {
		{0, 0, 0}
	 };

	fds[0].fd = listenSock;
	fds[0].events = POLLIN;

	shouldRun = true;
	while (shouldRun)
	{
		pollNum = poll(fds, nfds, -1);
		if (pollNum > 0 && (fds[0].revents & POLLIN))
		{
			bytesRead = read(listenSock, &buf, sizeof(logInfo));
			printLog(buf);
		}
	}

	do
	{
		bytesRead = read(listenSock, &buf, sizeof(logInfo));
		if (bytesRead > 0)
		{
			printLog(buf);
		}
	} while (bytesRead > 0);
	return NULL;
}

int initLogger()
{
	createListenSocket();

	pthread_create(&loggerThread, NULL, logServer, NULL);

	createWriteLogSocket();
	return 0;
}

int closeLogger()
{
	shouldRun = false;
	LOG_CRITICAL("close logger");
	pthread_join(loggerThread, NULL);

	close(listenSock);
	unlink(NAME);

	return 0;
}

void writeLog(logInfo lineData)
{
	sendto(logWriteSock, &lineData, sizeof(logInfo), 0, (const struct sockaddr *)&name, sizeof(struct sockaddr_un));
}
