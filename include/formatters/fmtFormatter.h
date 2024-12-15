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
#include <fmt/compile.h>
#include <fmt/format.h>
#include <string>

#ifndef FORMAT_FUNC
/**
 * @brief format the log msg before putting it in the logInfo, this is fmt based format
 */
#define FORMAT_FUNC(logData, formatStr, ...)                                                                           \
	{                                                                                                                  \
		fmt::format_to_n(logData->msg, MAX_LOG_LEN, FMT_COMPILE(formatStr) __VA_OPT__(, ) __VA_ARGS__);                 \
	}
#endif // !FORMAT_FUNC
