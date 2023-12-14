/**
 * @file cFormatter.h
 * @author itai lupo
 * @brief this is the code for the c format func
 * @version 0.1
 * @date 2023-12-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <stdio.h>

#ifndef FORMAT_FUNC
/**
 * @brief format the log msg before putting it in the logInfo, this is c based format
 */
#define FORMAT_FUNC(logData, format, ...)                                                                              \
	{                                                                                                                  \
		snprintf(logData.msg, MAX_LOG_LEN, format __VA_OPT__(, ) __VA_ARGS__);                                         \
	}
#endif // !FORMAT_FUNC
