#include "common-response.h"
#include "defines.h"
#include "event-handler.h"
#include "files.h"
#include "list.h"
#include "logger.h"
#include "server-defines.h"
#include "server-machine.h"
#include "server-routes.h"
#include "server-timer.h"
#include "server.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef IS_BUNDLED
#include "./includes/libs/vips/include/vips/vips.h"
#else
#include <vips/vips.h>
#endif
#include "./includes/file-controller.h"

#define MAX_EVENTS 10
#warning "HANDLE 100 continue header"
#warning "errs on content-length 0"
#warning                                                                       \
    "here start time out to kill the request maybe with timerfd and hook it to epoll"

uint64_t get_next_idx();

uint64_t get_idx();

int main(int argc, char *argv[]) {

  logger_init();

  if (VIPS_INIT(argv[0])) {
    return 1;
  }
  signal(SIGPIPE, SIG_IGN);
  assert(routes_init() == 0);

  attempt_create_dir(IMG_ORIGINAL_DIR, S_IRUSR | S_IWUSR | S_IXUSR);
  attempt_create_dir(IMG_THUMBNAIL_DIR, S_IRUSR | S_IWUSR | S_IXUSR);
  attempt_create_dir(IMG_CACHE_DIR, S_IRUSR | S_IWUSR | S_IXUSR);
  file_controller_init();

  server s;
  if (init_server_addr(&s.addr) < 0) {
    return -1;
  }
  if (init_sock_server(&s) < 0) {
    return -1;
  }

  /* int epoll_fd = epoll_create1(0); */
  /* if (epoll_fd == -1) */
  /*   return -1; */

  event_handler_t *ev_handler = event_handler_init(MAX_EVENTS);
  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = s.server_fd;
  logger_log(LOG_HIGH | LOG_MEDIUM, "%s: Server: ev.events=EPOLLIN", __func__);
  if (!event_handler_add_event(ev_handler, s.server_fd, event)) {
    logger_log(LOG_HIGH | LOG_MEDIUM, "FAILED AT event_handler_add_event: %s\n",
               strerror(errno));
    return 1;
  }
  /* if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, s.server_fd, &event) == -1) { */
  /*   return -1; */
  /* } */

  /* struct epoll_event events[MAX_EVENTS]; */
  server_machine machine[MAX_EVENTS] = {0};
#pragma unroll 10
  for (size_t i = 0; i < MAX_EVENTS; i++) {
    server_machine_init(&machine[i]);

    logger_log(LOG_HIGH | LOG_MEDIUM, "FD IS TIMER FOR MACHINE %p ",
               &machine[i]);
    event.data.fd = server_timer_get_fd(machine[i].timer);
    event_handler_add_event(ev_handler, server_timer_get_fd(machine[i].timer),
                            event);

    set_client_fd(&machine[i], -1);
  }

#ifdef DEBUG_MAX_CYCLES
  logger_log(LOG_HIGH | LOG_MEDIUM, "DEBUG_MAX_CYCLES: %d\n", DEBUG_MAX_CYCLES);
  for (size_t i = 0; i < DEBUG_MAX_CYCLES; i++) {
    printf("CYCLE: %d out of %d\n", i, DEBUG_MAX_CYCLES);
#else
  logger_log(LOG_HIGH | LOG_MEDIUM, "DEBUG_MAX_CYCLES: UNDEFINED\n");
  for (;;) {
#endif
    /* const struct epoll_event events = event_handler_get_event(ev_handler); */
    /* int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1); */
    /* if (n_events == -1) */
    /*   return -1; */

    const events_t received_events = event_handler_get_events(ev_handler);
    const struct epoll_event *events = received_events.events;
    for (size_t i = 0; i < received_events.len; i++) {
      logger_log(LOG_HIGH | LOG_MEDIUM,
                 "RECEIVED EVENTS: i: %lu len: %lu fd[i]: %d\n", i,
                 received_events.len, events[i].data.fd);

      if (events[i].events & (EPOLLHUP | EPOLLRDHUP)) {
#pragma unroll 10
        for (size_t w = 0; w < MAX_EVENTS; w++) {
          if (events[i].data.fd == get_client_fd(&machine[w])) {
            server_machine *selected = nullptr;
            selected = &machine[w];
            if (server_machine_free_endpont_ctx(selected)) {

              server_machine_set_free_endpoint_ctx(selected, nullptr);
              server_machine_set_endpoint_ctx(selected, nullptr);

            } else {

              logger_log(LOG_HIGH | LOG_MEDIUM, "161 no free_endpoint_ctx\n");
            }
            event_handler_remove_event(ev_handler, events[i].data.fd);
            logger_log(LOG_HIGH | LOG_MEDIUM, "EPOLLHUP|EPOLLRDHUP\n");
            server_machine_reset(selected);

            break;
          }
        }
        continue;
      }

      // new conn
      if (events[i].data.fd == s.server_fd) {
        logger_log(LOG_HIGH | LOG_MEDIUM,
                   "%s event[i]_fd is server, client_fd: %d\n", __func__,
                   s.client_fd);
        if (s.client_fd < 0) {
          break;
        }
        logger_log(
            LOG_HIGH | LOG_MEDIUM,
            "Attempt adding client/requester fd: %d into list of events\n",
            s.server_fd);

        switch (init_client(&s)) {
        case 0: {
          event.events = EPOLLIN;
          event.data.fd = s.client_fd;
          logger_log(LOG_HIGH | LOG_MEDIUM, "%s: new socket ", __func__);

          event.data.fd = s.client_fd;
          event_handler_add_event(ev_handler, s.client_fd, event);
          /* epoll_ctl(epoll_fd, EPOLL_CTL_ADD, s.client_fd, &event); */
          logger_log(LOG_HIGH | LOG_MEDIUM, "fd: %d added\n", s.client_fd);
          continue;
          break;
        }
        }
        logger_log(LOG_HIGH | LOG_MEDIUM, "fd: %d not added\n", s.client_fd);
        continue;
      }

      bool should_cont = false;
#pragma unroll 10
      for (size_t machine_idx = 0; machine_idx < MAX_EVENTS; machine_idx++) {

        if (machine[machine_idx].timer != events[i].data.fd) {
          continue;
        }
        bool should_close = true;
        if (server_timer_has_expired(machine[machine_idx].timer)) {
          logger_log(LOG_HIGH | LOG_MEDIUM, "fd: %d has expired\n",
                     machine[machine_idx].timer);
          request_timeout(get_client_fd(&machine[machine_idx]), BUFFER, nullptr,
                          "Request Timeout");
          should_close = false;
        }
        server_machine *selected = nullptr;
        selected = &machine[machine_idx];

        if (server_machine_free_endpont_ctx(selected)) {

          server_machine_set_free_endpoint_ctx(selected, nullptr);
          server_machine_set_endpoint_ctx(selected, nullptr);

        } else {
        }
        if (should_close) {

          logger_log(LOG_HIGH | LOG_MEDIUM,
                     "%s: closing `events[i].data.fd` (%d)\n", __func__,
                     events[i].data.fd);
          close(events[i].data.fd);
        }
        server_machine_reset(selected);

        should_cont = true;
      }
      if (should_cont) {
        continue;
      }

      server_machine *selected = nullptr;
      size_t w = 0;

#pragma unroll 10
      for (w = 0; w < MAX_EVENTS; w++) {
        logger_log(
            LOG_HIGH | LOG_MEDIUM,
            "events[i].data.fd == get_client_fd(&machine[w])\n%d == %d:% d\n ",
            events[i].data.fd, get_client_fd(&machine[w]),
            events[i].data.fd == get_client_fd(&machine[w]));
        if (events[i].data.fd == get_client_fd(&machine[w])) {
          selected = &machine[w];
          break;
        }
      }

      if (!selected) {
#pragma unroll 10
        for (w = 0; w < MAX_EVENTS; w++) {
          if (get_state(&machine[w]) == WAITING) {
            selected = &machine[w];
            logger_log(LOG_HIGH | LOG_MEDIUM, "!selected\n");
            server_machine_reset(selected);
            set_client_fd(selected, events[i].data.fd);

            break;
          }
        }
      }

#warning "Here change for keep alive timeout"
      server_timer_start(selected->timer, 1, 0);
      endpoint_return status = server_job((void *)selected);

      switch (status) {
      case NEED_WRITE_MORE_DATA: {
        event.events = EPOLLIN | EPOLLOUT;
        event.data.fd = events[i].data.fd;
        event_handler_mod(ev_handler, events[i].data.fd, &event);
        /* epoll_ctl(epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &event); */
        break;
      }
      case NEED_READ_MORE_DATA:
        continue;
        break;

      case FINISHED:
      case SOMETHING_WENT_WRONG: {

        if (event.events == (EPOLLIN | EPOLLOUT)) {

          event.data.fd = events[i].data.fd;
          event_handler_mod(ev_handler, events[i].data.fd, &event);

          /* epoll_ctl(epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &event); */
        }

        logger_log(LOG_HIGH | LOG_MEDIUM, "event_fd: %d\n", events[i].data.fd);
        logger_log(LOG_HIGH | LOG_MEDIUM, "FINISHED/SOMETHING_WENT_WRONG:\n");
        shutdown(events[i].data.fd, SHUT_WR);
        event_handler_del(ev_handler, events[i].data.fd);
        server_machine_reset(selected);
        break;
      }
      }
    }
  }
  routes_destroy();
#pragma unroll 10
  for (size_t i = 0; i < MAX_EVENTS; i++) {
    if (machine[i].server_ctx->free_endpoint_ctx) {
      machine[i].server_ctx->free_endpoint_ctx(
          machine[i].server_ctx->endpoint_ctx);
    } else {

      logger_log(LOG_HIGH | LOG_MEDIUM,
                 "in the eeeenddd no free_endpoint_ctx\n");
    }
    if (machine[i].server_ctx->cursor) {
      free(machine[i].server_ctx->cursor);
    }
    if (machine[i].server_ctx) {
      free(machine[i].server_ctx);
    }
    list_free(&machine[i].headers, list_default_callback);
    list_free(&machine[i].params, list_default_callback);
    server_timer_close(machine[i].timer);
    machine[i].timer = -1;
  }
  vips_shutdown();
  destroy_server(&s);
  logger_destroy();
  event_handler_destroy(&ev_handler);

  return 0;
}
uint64_t get_next_idx() { return file_controller_get_next_index(); }

uint64_t get_idx() { return file_controller_get_length(); }
