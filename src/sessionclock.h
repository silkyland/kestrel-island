#pragma once
#include <QtGlobal>
#include <time.h>
inline qint64 sessionMilliseconds() {
  timespec value{};
  clock_gettime(CLOCK_BOOTTIME, &value);
  return qint64(value.tv_sec) * 1000 + value.tv_nsec / 1000000;
}
