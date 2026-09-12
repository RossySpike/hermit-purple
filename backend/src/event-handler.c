#include "event-handler.h"
#include "defines.h"
#include "logger.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

struct event_handler_t {
  struct epoll_event *events;
  int epoll_fd;
  size_t events_len;
  int ready_fds;
  size_t event_cursor;
};
event_handler_t *event_handler_init(size_t max_events) {

  assert(max_events > 0 && max_events <= INT_MAX);

  event_handler_t *handler = calloc(1, sizeof(struct event_handler_t));
  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    return nullptr;
  }
  /* struct epoll_event event; */
  /* event.events = EPOLLIN; */
  /* event.data.fd = server_fd; */
  /* if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) { */
  /*   close(epoll_fd); */
  /*   return handler; */
  /* } */
  struct epoll_event *ev = calloc(1, sizeof(struct epoll_event) * max_events);
  /* int n_events = epoll_wait(epoll_fd, ev, max_events, -1); */
  /* if (n_events == -1) { */
  /*   free(ev); */
  /*   return handler; */
  /* } */
  handler->events = ev;
  handler->epoll_fd = epoll_fd;
  handler->ready_fds = 0;
  handler->events_len = max_events;
  handler->event_cursor = 0;

  // NOTE: ready_fds already 0, intended

  return handler;
}
bool event_handler_add_event(const event_handler_t *handler, int target_fd,
                             struct epoll_event event) {
  logger_log(LOG_HIGH|LOG_MEDIUM,"Listening for events provided by fd: %d\n", target_fd);
  return epoll_ctl(handler->epoll_fd, EPOLL_CTL_ADD, target_fd, &event) != -1;
}
bool event_handler_is_valid_epoll_event(struct epoll_event event) {
  return event.data.fd != -1;
}
events_t event_handler_get_events(event_handler_t *handler) {
  int num_evnts =
      epoll_wait(handler->epoll_fd, handler->events, handler->events_len, -1);

  if (num_evnts == -1) {
    num_evnts = 0;
    logger_log(LOG_HIGH|LOG_MEDIUM,"%s: wait failed with %d %s\n", __func__, errno,
               strerror(errno));
    fperror;
    sleep(10);
  }

  const events_t events = {num_evnts, handler->events};
  return events;
}

/* struct epoll_event event_handler_get_event(event_handler_t *const handler) {
 */
/**/
/*   if (handler->ready_fds <= 0 || handler->event_cursor > handler->ready_fds)
 * { */
/**/
/*     handler->ready_fds = */
/*         epoll_wait(handler->epoll_fd, handler->events, handler->events_len,
 * -1); */
/*     handler->event_cursor = 0; */
/*   } */
/*   return handler->ready_fds <= 0 ? (struct epoll_event){.data = {.fd = -1}}
 */
/*                                  : handler->events[handler->event_cursor++];
 */
/* } */

bool event_handler_remove_event(event_handler_t *handler, int target_fd) {

  if (epoll_ctl(handler->epoll_fd, EPOLL_CTL_DEL, target_fd, nullptr) != 0) {
    logger_log(LOG_HIGH|LOG_MEDIUM,"%s:%d: closing `target_fd` (%d)\n", __func__, __LINE__,
               target_fd);
    close(target_fd);
    return false;
  }
  logger_log(LOG_HIGH|LOG_MEDIUM,"%s: deleting fd: %d\n", __func__, target_fd);
  /* if (close(target_fd) != 0) { */
  /*   return false; */
  /* } */
  /* logger_log(LOG_HIGH|LOG_MEDIUM,"%s: closing fd: %d\n", __func__, target_fd); */
  return true;
}
void event_handler_destroy(event_handler_t **handler) {
  free((*handler)->events);
  logger_log(LOG_HIGH|LOG_MEDIUM,"%s: closing `(*handler)->epoll_fd` (%d)\n", __func__,
             (*handler)->epoll_fd);
  close((*handler)->epoll_fd);
  (*handler)->events = nullptr;
  (*handler)->epoll_fd = -1;
  *handler = nullptr;
}
int event_handler_get_num_events(event_handler_t *handler) {
  return handler->ready_fds;
}
bool event_handler_mod(const event_handler_t *handler, int target_fd,
                       struct epoll_event *event) {

  logger_log(LOG_HIGH|LOG_MEDIUM,"%s: modifying fd: %d\n", __func__, target_fd);
  return epoll_ctl(handler->epoll_fd, EPOLL_CTL_MOD, target_fd, event) != -1;
}
bool event_handler_del(const event_handler_t *handler, int target_fd) {
  logger_log(LOG_HIGH|LOG_MEDIUM,"%s: deleting fd: %d\n", __func__, target_fd);

  return epoll_ctl(handler->epoll_fd, EPOLL_CTL_DEL, target_fd, nullptr) != -1;
}
