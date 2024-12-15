#include "sinks/printToSinks.h"

#include "sinks/stdColor.h"

#include "log.h"

#include "err.h"

#include "processes.h"

typedef err_t (*sinkInitFunc)();
typedef err_t (*sinkPrintFunc)(logInfo_t *logToPrint, char *processName, char *ThreadName);
typedef err_t (*sinkCloseFunc)();

typedef struct
{
	sinkInitFunc init;
	sinkPrintFunc print;
	sinkCloseFunc close;
} sinkCallbacks;

#if __has_include("sinksSetup.h")
#include "sinksSetup.h"
#else
#include "sinks/cyclicJsonSink.h"
#include "sinks/printToSinks.h"
constexpr const static sinkCallbacks sinks[] = {
	{initStdColorSink,   stdColorSink,	 closeStdColorSink  },
	{initCyclicJsonSink, cyclicJsonSink, closeCyclicJsonSink},
};
#endif

err_t initSinks()
{
	err_t err = NO_ERRORCODE;

	for (size_t i = 0; i < sizeof(sinks) / sizeof(sinks[0]); i++)
	{
		if (sinks[i].print != NULL)
		{
			QUITE_RETHROW(sinks[i].init());
		}
	}

cleanup:
	return err;
}

err_t closeSinks()
{
	err_t err = NO_ERRORCODE;

	for (size_t i = 0; i < sizeof(sinks) / sizeof(sinks[0]); i++)
	{
		if (sinks[i].print != NULL)
		{
			(sinks[i].close());
		}
	}

	// cleanup:
	return err;
}

err_t printToSinks(logInfo_t *logToPrint)
{
	err_t err = NO_ERRORCODE;
	char threadName[17] = {0};
	char processName[17] = {0};

	err = (getThreadName(logToPrint->metadata.pid, logToPrint->metadata.tid, threadName, sizeof(threadName)));
	err = (getProcessName(logToPrint->metadata.pid, processName, sizeof(processName)));
	err = NO_ERRORCODE;

	for (size_t i = 0; i < sizeof(sinks) / sizeof(sinks[0]); i++)
	{
		if (sinks[i].print != NULL)
		{
			REWARN(sinks[i].print(logToPrint, processName, threadName));
		}
	}

	return err;
}
