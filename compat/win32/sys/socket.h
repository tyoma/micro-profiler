#pragma once

#include <winsock2.h>

#define close closesocket
#define O_NONBLOCK FIONBIO
#define MSG_NOSIGNAL 0
#define SHUT_RDWR SD_BOTH
