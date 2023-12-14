#pragma once
#include "err.h"
#include "files.h"

#ifdef __cplusplus
extern "C"
{
#endif

	__attribute__((__noreturn__)) void initLogServer(const char *const _listenSockName, fd_t _efd[2]);
	THROWS err_t closeLogServer();

#ifdef __cplusplus
}
#endif
