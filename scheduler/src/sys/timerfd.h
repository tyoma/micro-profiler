#pragma once

#include <asm/unistd.h>
#include <sys/syscall.h>
#include <unistd.h>

#define TFD_TIMER_ABSTIME (1 << 0)

static inline int
timerfd_create(int clockid, int flags)
{
  return syscall(__NR_timerfd_create, clockid, flags);
}

static inline int
timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *curr_value)
{
  return syscall(__NR_timerfd_settime, fd, flags, new_value, curr_value);
}

static inline int
timerfd_gettime(int fd, struct itimerspec *curr_value)
{
  return syscall(__NR_timerfd_gettime, fd, curr_value);
}
