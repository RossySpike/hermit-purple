#include "./includes/files.h"
#include "./includes/server-defines.h"
#include "./includes/server-machine.h"
#include "./includes/server-routes.h"
#include "./includes/server.h"
#include "includes/list.h"
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vips/vips.h>

#define MAX_EVENTS 10
int logger;

void server_machine_reset(server_machine *machine) {
  if (!machine)
    return;

  if (machine->headers.array && machine->headers.len > 0) {
    list_free_contents(&machine->headers, list_default_callback);
  }
  if (machine->headers.array) {
    for (size_t i = 0; i < machine->headers.capacity; i++) {
      machine->headers.array[i] = nullptr;
    }
  }
  machine->headers.len = 0;

  if (machine->params.array && machine->params.len > 0) {
    list_free_contents(&machine->params, list_default_callback);
  }
  if (machine->params.array) {
    for (size_t i = 0; i < machine->params.capacity; i++) {
      machine->params.array[i] = nullptr;
    }
  }
  machine->params.len = 0;

  if (machine->server_ctx) {
    machine->server_ctx->completed_headers = 0;
    machine->server_ctx->should_read = true;
    bzero(machine->server_ctx->read_buffer, BUFFER);

    if (machine->server_ctx->cursor) {
      machine->server_ctx->cursor->curr = 0;
      machine->server_ctx->cursor->offset = 0;
      machine->server_ctx->cursor->empty_mem = false;
      machine->server_ctx->cursor->curr_mem = 0;
      machine->server_ctx->cursor->read_bytes = 0;
      bzero(machine->server_ctx->cursor->memory, BUFFER);
    }

    /* printf("machine->server_ctx: %p        \n", machine->server_ctx); */
    /* printf("machine->server_ctx->free_endpoint_ctx: %p        \n", */
    /*        (void *)machine->server_ctx->free_endpoint_ctx); */
    if (machine->server_ctx->endpoint_ctx &&
        machine->server_ctx->free_endpoint_ctx) {
      machine->server_ctx->free_endpoint_ctx(machine->server_ctx->endpoint_ctx);
      machine->server_ctx->endpoint_ctx = nullptr;
      machine->server_ctx->free_endpoint_ctx = nullptr;
    }
  }

  machine->state = WAITING;
  machine->prev_state = WAITING;
  machine->client_fd = -1;
  machine->route_idx = 0;
}

unsigned long long current = 0;

unsigned long long get_next_idx() {
  if (current == 0) {
    current = get_biggest_index(IMG_ORIGINAL_DIR);
  }
  current++;
  return current;
}

unsigned long long get_idx() { return current; }

int main(int argc, char *argv[]) {
  logger = open("./log", O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (VIPS_INIT(argv[0])) {
    return 1;
  }
  signal(SIGPIPE, SIG_IGN);
  assert(routes_init() == 0);

  attempt_create_dir(IMG_ORIGINAL_DIR, S_IRUSR | S_IWUSR | S_IXUSR);
  attempt_create_dir(IMG_THUMBNAIL_DIR, S_IRUSR | S_IWUSR | S_IXUSR);
  attempt_create_dir(IMG_CACHE_DIR, S_IRUSR | S_IWUSR | S_IXUSR);

  if (current == 0) {
    current = get_biggest_index(IMG_ORIGINAL_DIR);
  }

  server s;
  if (init_server_addr(&s.addr) < 0)
    return -1;
  if (init_sock_server(&s) < 0)
    return -1;

  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1)
    return -1;

  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = s.server_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, s.server_fd, &event) == -1) {
    return -1;
  }

  struct epoll_event events[MAX_EVENTS];
  server_machine machine[MAX_EVENTS] = {0};
  for (size_t i = 0; i < MAX_EVENTS; i++) {
    server_machine_init(&machine[i]);
    set_client_fd(&machine[i], -1);
  }

  for (;;) {
    int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (n_events == -1)
      return -1;

    for (size_t i = 0; i < (size_t)n_events; i++) {
      if (events[i].events & (EPOLLHUP | EPOLLRDHUP)) {
        for (size_t w = 0; w < MAX_EVENTS; w++) {
          if (events[i].data.fd == get_client_fd(&machine[w])) {
            server_machine *selected = nullptr;
            selected = &machine[w];
            if (selected->server_ctx &&
                selected->server_ctx->free_endpoint_ctx &&
                selected->server_ctx->endpoint_ctx) {

              selected->server_ctx->free_endpoint_ctx(
                  selected->server_ctx->endpoint_ctx);

              selected->server_ctx->endpoint_ctx = nullptr;
              selected->server_ctx->free_endpoint_ctx = nullptr;
            } else {
              /* printf("147 no free_endpoint_ctx\n"); */
            }

            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
            close(events[i].data.fd);
            /* printf("EPOLLHUP|EPOLLRDHUP\n"); */
            server_machine_reset(selected);

            break;
          }
        }
        continue;
      }

      // new conn
      if (events[i].data.fd == s.server_fd) {
        switch (init_client(&s)) {
        case 0: {
          event.events = EPOLLIN;
          event.data.fd = s.client_fd;
          epoll_ctl(epoll_fd, EPOLL_CTL_ADD, s.client_fd, &event);
          break;
        }
        }
        continue;
      }

      server_machine *selected = nullptr;
      size_t w = 0;

      for (w = 0; w < MAX_EVENTS; w++) {
        /* printf( */
        /*     "events[i].data.fd == get_client_fd(&machine[w])\n%d == %d:
         * %d\n", */
        /*     events[i].data.fd, get_client_fd(&machine[w]), */
        /*     events[i].data.fd == get_client_fd(&machine[w])); */
        if (events[i].data.fd == get_client_fd(&machine[w])) {
          selected = &machine[w];
          break;
        }
      }

      if (!selected) {
        for (w = 0; w < MAX_EVENTS; w++) {
          if (get_state(&machine[w]) == WAITING) {
            selected = &machine[w];
            /* printf("!selected\n"); */
            server_machine_reset(selected);
            set_client_fd(selected, events[i].data.fd);

            break;
          }
        }
      }

      if (!selected) {
        close(events[i].data.fd);
        continue;
      }

      endpoint_return status = server_job((void *)selected);

      switch (status) {
      case NEED_WRITE_MORE_DATA: {
        event.events = EPOLLIN | EPOLLOUT;
        event.data.fd = events[i].data.fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &event);
        break;
      }
      case NEED_READ_MORE_DATA:
        continue;
        break;

      case FINISHED:
      case SOMETHING_WENT_WRONG: {

        if (event.events == (EPOLLIN | EPOLLOUT)) {

          event.data.fd = events[i].data.fd;
          epoll_ctl(epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &event);
        }

        /* printf("event_fd: %d\n", events[i].data.fd); */
        /* printf("FINISHED/SOMETHING_WENT_WRONG:\n"); */
        server_machine_reset(selected);
        shutdown(events[i].data.fd, SHUT_WR);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
        break;
      }
      }
    }
  }

  routes_destroy();
  for (size_t i = 0; i < MAX_EVENTS; i++) {
    if (machine[i].server_ctx->free_endpoint_ctx) {
      machine[i].server_ctx->free_endpoint_ctx(
          machine[i].server_ctx->endpoint_ctx);
    } else {
      /* printf("in the eeeenddd no free_endpoint_ctx\n"); */
    }
    if (machine[i].server_ctx->cursor)
      free(machine[i].server_ctx->cursor);
    if (machine[i].server_ctx)
      free(machine[i].server_ctx);
    list_free(&machine[i].headers, list_default_callback);
    list_free(&machine[i].params, list_default_callback);
  }
  vips_shutdown();
  close(logger);
  destroy_server(&s);
  close(epoll_fd);
  return 0;
}
