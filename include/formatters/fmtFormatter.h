#pragma once
#include <fmt/format.h>
#include <string>

#ifndef FORMAT_FUNC
#define FORMAT_FUNC(logData, formatStr, ...)                                                                           \
	{                                                                                                                  \
		std::string buffer = fmt::format(formatStr __VA_OPT__(, ) __VA_ARGS__);                                        \
		strncpy(logData.msg, buffer.data(), MAX_LOG_LEN);                                                              \
		logData.msgSize = (buffer.size() < MAX_LOG_LEN ? buffer.size() : MAX_LOG_LEN);                                 \
	}
#endif // !FORMAT_FUNC
