#pragma once
#include "types/err_t.h"
#include "types/logInfo.h"

#ifdef __cplusplus
extern "C"
{
#endif
	err_t initSinks();
	err_t printToSinks(logInfo_t *logData);
	err_t closeSinks();
#ifdef __cplusplus
}
#endif
