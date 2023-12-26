/**
 * @file server.h
 * @author itai lupo
 * @brief the api of the log server
 * @version 0.1
 * @date 2023-12-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include "err.h"
#include "files.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief start the log server
	 * @param listenSockName the name to open the unix domain socket on
	 * @param killLogServerEfd a event fd that the logger will clean stop on it
	 * @param waitForLoggerEfd the logger will write to it when it is read to receive logs
	 */
	void initLogServer(const char *const listenSockName, fd_t killLogServerEfd, fd_t waitForLoggerEfd)
		__attribute__((__noreturn__));

#ifdef __cplusplus
}
#endif
