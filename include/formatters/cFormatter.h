#pragma once

#include <stdio.h>

#ifndef FORMAT_FUNC
#define FORMAT_FUNC(logData, format, ...)                                                                              \
	{                                                                                                                  \
		logData.msgSize = snprintf(logData.msg, MAX_LOG_LEN, format __VA_OPT__(, ) __VA_ARGS__);                       \
	}
#endif // !FORMAT_FUNC
