#include "server-timer.h"
#include "logger.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

timer server_timer_create() {
  return timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
}
bool server_timer_start(timer timer, uint64_t seconds, uint64_t nanoseconds) {
  struct itimerspec ts;
  ts.it_value.tv_sec = seconds;
  ts.it_value.tv_nsec = nanoseconds;
  ts.it_interval.tv_sec = 1;
  ts.it_interval.tv_nsec = 0;
  logger_log(LOG_HIGH|LOG_MEDIUM,"%s: starting timer for fd: %d\n", __func__, timer);
  if (timerfd_settime(timer, 0, &ts, nullptr) == -1) {
    return false;
  }
  return true;
}
inline int server_timer_get_fd(timer timer) { return (int)timer; }
bool server_timer_stop(timer timer) {
  struct itimerspec ts = {0};
  logger_log(LOG_HIGH|LOG_MEDIUM,"%s: stopping timer for fd: %d\n", __func__, timer);
  bool result = timerfd_settime(timer, 0, &ts, nullptr) != -1;
  uint64_t expirations;
  ssize_t s = read(timer, &expirations, sizeof(uint64_t));
  return result;
}
inline bool server_timer_close(timer timer) {
  logger_log(LOG_HIGH|LOG_MEDIUM,"%s: closing `(int)timer` (%d)\n", __func__, (int)timer);
  return close((int)timer);
}
bool server_timer_has_expired(timer timer) {

  uint64_t bytes = 0;
  int retval = read((int)timer, &bytes, sizeof(uint64_t));

  logger_log(LOG_HIGH|LOG_MEDIUM,"%s: read result: %d, errno: %s, fd: %d\n", __func__, retval,
             strerror(errno), timer);
  return (bool)!(retval == -1 && (errno == EAGAIN || errno == EWOULDBLOCK));
}
