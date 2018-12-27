#pragma once

#include <winsock2.h>

inline in_addr inet_addr_winsock(const char *address)
{
	in_addr result;

	result.S_un.S_addr = (inet_addr)(address);
	return result;
}

#define close closesocket
#define inet_addr inet_addr_winsock
#define O_NONBLOCK FIONBIO
