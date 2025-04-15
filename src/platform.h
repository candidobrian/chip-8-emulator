#pragma once

#if defined(__linux__) || defined(__linux) || defined(linux)
#define _POSIX_C_SOURCE 200809L
#endif

#include <errno.h>
#include <stdbool.h>
#include <time.h>

static inline int sleep_ms(unsigned long ms) {
  errno = 0;
  struct timespec rem;
  struct timespec req = {ms / 1000, (ms % 1000) * 1000000};

  while (true) {
    if (nanosleep(&req, &rem) == 0)
      return 0;

    if (errno == EINTR) {
      req = rem;
      continue;
    }

    return -1;
  }
}
