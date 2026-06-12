#include "./includes/files.h"
#include "./includes/server-defines.h"
#include "./includes/server-machine.h"
#include "./includes/server-routes.h"
#include "./includes/server.h"
#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/stat.h> // S_IXUSR
#include <sys/types.h>
#include <vips/vips.h>
#define MAX_EVENTS 10
#include <stdio.h>

void server_machine_dump(const char *label, const server_machine *sm) {
  fprintf(stderr, "\n--- 🔍 DUMP SERVER MACHINE [%s] ---\n",
          label ? label : "UNNAMED");
  if (!sm) {
    fprintf(stderr, "  (null pointer)\n");
    fprintf(stderr, "----------------------------------------\n");
    return;
  }

  // 1. Estado de la máquina (Enum)
  const char *state_str = "UNKNOWN";
  switch (sm->state) {
  case WAITING:
    state_str = "WAITING";
    break;
  case PROCESSING_REQUEST_LINE:
    state_str = "PROCESSING_REQUEST_LINE";
    break;
  case PROCESSING_HEADERS:
    state_str = "PROCESSING_HEADERS";
    break;
  case WORKING:
    state_str = "WORKING";
    break;
  case ENDING:
    state_str = "ENDING";
    break;
  }
  fprintf(stderr, "  • State     : %s (%d)\n", state_str, sm->state);
  fprintf(stderr, "  • Client FD : %d  <-- 🚨 ¡Cuidado si ves un 0 aquí!\n",
          sm->client_fd);
  fprintf(stderr, "  • Route Idx : %zu\n", sm->route_idx);

  // 2. Información de las Listas Dinámicas
  fprintf(stderr, "  • Headers   : [Len: %zu, Cap: %zu]\n", sm->headers.len,
          sm->headers.capacity);
  fprintf(stderr, "  • Params    : [Len: %zu, Cap: %zu]\n", sm->params.len,
          sm->params.capacity);

  // 3. Inspección del Contexto del Servidor (server_ctx)
  fprintf(stderr, "  • Server CTX:\n");
  if (!sm->server_ctx) {
    fprintf(stderr, "    (null server_ctx)\n");
  } else {
    fprintf(stderr, "    - should_read       : %s\n",
            sm->server_ctx->should_read ? "true" : "false");
    fprintf(stderr, "    - completed_headers : %zu\n",
            sm->server_ctx->completed_headers);
    fprintf(stderr, "    - endpoint_ctx ptr  : %p\n",
            sm->server_ctx->endpoint_ctx);
    fprintf(stderr, "    - free_callback ptr : %p\n",
            (void *)sm->server_ctx->free_endpoint_ctx);

    // Muestra los primeros 40 caracteres del buffer de lectura para debuggear
    // HTTP raw
    fprintf(stderr, "    - read_buffer (preview): \"%.40s...\"\n",
            sm->server_ctx->read_buffer);

    // 4. Inspección del Cursor del Stream (stream_cursor)
    fprintf(stderr, "    - Stream Cursor:\n");
    if (!sm->server_ctx->cursor) {
      fprintf(stderr, "      (null cursor)\n");
    } else {
      stream_cursor *c = sm->server_ctx->cursor;
      fprintf(stderr, "      * curr      : %zu\n", c->curr);
      fprintf(stderr, "      * offset    : %zu\n", c->offset);
      fprintf(stderr, "      * empty_mem : %s\n",
              c->empty_mem ? "true" : "false");
      fprintf(stderr, "      * curr_mem  : %zu\n", c->curr_mem);
    }
  }
  fprintf(stderr, "------------------------------------------------\n\n");
}
void setup_signal_handlers() {
  // ignore sigpipe
  signal(SIGPIPE, SIG_IGN);

  fprintf(stderr, "broken pipe... continuing\n");
}
#define this machine

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
  if (VIPS_INIT(argv[0])) {
    return 1;
  }
  signal(SIGPIPE, SIG_IGN);
  assert(routes_init() == 0);
  int res = attempt_create_dir(IMG_ORIGINAL_DIR, S_IRUSR | S_IWUSR | S_IXUSR);
  assert(res != -1);
  res = attempt_create_dir(IMG_THUMBNAIL_DIR, S_IRUSR | S_IWUSR | S_IXUSR);
  assert(res != -1);
  res = attempt_create_dir(IMG_CACHE_DIR, S_IRUSR | S_IWUSR | S_IXUSR);
  assert(res != -1);

  if (current == 0) {

    current = get_biggest_index(IMG_ORIGINAL_DIR);
  }
  server s;
  if (init_server_addr(&s.addr) < 0) {
    return -1;
  }
  if (init_sock_server(&s) < 0) {
    return -1;
  }

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
  }
  for (;;) {

    int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (n_events == -1) {
      return -1;
    }
    for (size_t i = 0; i < n_events; i++) {

      if (events[i].data.fd == s.server_fd) {

        switch (init_client(&s)) {
        case 0: // already set to non_blocking
        {
          event.events = EPOLLIN | EPOLLET;
          event.data.fd = s.client_fd;
          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, s.client_fd, &event) == -1) {
            perror("epoll_ctl: client_fd");
            close(s.client_fd);
          }
        }
        }
      } else {
        server_machine *selected = nullptr;
        size_t w = 0;
        for (; w < MAX_EVENTS; w++) {
          if (events[i].data.fd == get_client_fd(&machine[w])) {

            selected = &machine[w];
            break;
          }
          if (!selected && get_state(&machine[w]) == WAITING) {
            selected = &machine[w];
          }
        }
        server_machine_dump("antes del set fd", selected);
        set_client_fd(selected, events[i].data.fd);
        server_machine_dump("despues del set fd", selected);

        endpoint_return status = server_job((void *)selected);

#warning "Check `server_job` res value delete its content when `status=0`"
        switch (status) {
        case NEED_WRITE_MORE_DATA: {
          printf("devolvio need write\n");
          event.events = EPOLLIN | EPOLLOUT;
          event.data.fd = events[i].data.fd;
          epoll_ctl(epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &event);
          break;
        }
        case NEED_READ_MORE_DATA:
          break;
        case FINISHED:
        case SOMETHING_WENT_WRONG: {

          printf("devolvio finished/salio mal %d\n", status);

          if (selected->server_ctx) {
            if (selected->server_ctx->free_endpoint_ctx) {
              selected->server_ctx->free_endpoint_ctx(
                  selected->server_ctx->endpoint_ctx);
            }
          }
          list_free_contents(&machine[w].params, list_default_callback);
          list_free_contents(&machine[w].headers, list_default_callback);

          set_state(selected, WAITING);
          close(events[i].data.fd);
          break;
        }
        }
      }
      /**/
      // eliminar contenido de lista cuando acabe bien la peticion
      /* list_free(&this.headers, list_default_callback); */
      /* list_free(&this.params, list_default_callback); */
    }
  }
  printf("\nHOLY SHIIIIIIT\n");
  routes_destroy();

  for (size_t i = 0; i < MAX_EVENTS; i++) {
    list_free(&machine[i].headers, list_default_callback);
    list_free(&machine[i].params, list_default_callback);
  }
  return 0;
}
