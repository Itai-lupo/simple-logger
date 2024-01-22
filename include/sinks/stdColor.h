#pragma once
#include "types/err_t.h"
#include "types/logInfo.h"

err_t initStdColorSink();
err_t stdColorSink(logInfo_t logToPrint, char *processName, char *threadName);
err_t closeStdColorSink();
