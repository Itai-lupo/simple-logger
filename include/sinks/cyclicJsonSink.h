#pragma once
#include "types/err_t.h"
#include "types/logInfo.h"

#ifdef __cplusplus
extern "C"
{
#endif
	err_t initCyclicJsonSink();
	err_t cyclicJsonSink(logInfo_t *logToPrint, char *processName, char *ThreadName);
	err_t closeCyclicJsonSink();
#ifdef __cplusplus
}
#endif
