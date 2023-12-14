/**
 * @file fmtFormatter.h
 * @author itai lupo
 * @brief this is the fmt FORMAT_FUNC code
 * @version 0.1
 * @date 2023-12-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include <fmt/format.h>
#include <string>

#ifndef FORMAT_FUNC
/**
 * @brief format the log msg before putting it in the logInfo, this is fmt based format
 */
#define FORMAT_FUNC(logData, formatStr, ...)                                                                           \
	{                                                                                                                  \
		std::string buffer = fmt::format(formatStr __VA_OPT__(, ) __VA_ARGS__);                                        \
		strncpy(logData.msg, buffer.data(), MAX_LOG_LEN);                                                              \
	}
#endif // !FORMAT_FUNC
