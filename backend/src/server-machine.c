#include "../includes/server-machine.h"
#include "server.h"
#include <unistd.h>

#include "logger.h"
#include "server-timer.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
void server_machine_ctx_init(server_machine *machine) {
  machine->server_ctx = (server_ctx *)calloc(1, sizeof(server_ctx));
  machine->server_ctx->cursor =
      (stream_cursor *)calloc(1, sizeof(stream_cursor));
  machine->server_ctx->endpoint_ctx = nullptr;
  machine->server_ctx->free_endpoint_ctx = nullptr;
  machine->server_ctx->should_read = true;
}
void server_machine_init(server_machine *machine) {
  machine->state = WAITING;
  machine->client_fd = -1;
  server_machine_ctx_init(machine);
  machine->timer = server_timer_create();
  list_new(&machine->headers);
  list_new(&machine->params);
}

bool add_header(server_machine *machine, const char *header) {
  if (machine == nullptr || header == nullptr)
    return false;

  list_append(&machine->headers, (void *)header);

  return true;
}
bool add_params(server_machine *machine, const char *uri) {
  if (machine == nullptr || uri == nullptr)
    return false;

  list_append(&machine->params, (void *)uri);
  return true;
}
const char *get_header(const server_machine *machine, const char *header_name) {
  const list_t *headers = &machine->headers;

  for (size_t i = 0; i < headers->len; i++) {
  }
  for (size_t i = 0; i < headers->len; i++) {
    const char *cur_head = (const char *)headers->array[i];

    const char *colon = strchr(cur_head, ':');
    if (colon == nullptr)
      continue;

    size_t key_len = colon - cur_head;
    // compares cur_head (up to ':')with  header_name
    if (strncasecmp(cur_head, header_name, key_len) == 0) {

      return machine->headers.array[i];
    }
  }
  return nullptr;
}
const char *get_param(const server_machine *machine, const char *param_name) {
  const list_t *params = &machine->params;

  for (size_t i = 0; i < params->len; i++) {
    const char *cur_param = (const char *)params->array[i];

    const char *equal_sign = strchr(cur_param, '=');
    if (equal_sign == nullptr) //?
      continue;

    size_t key_len = equal_sign - cur_param;
    // compares cur_param (up to '=') with  param_name
    if (strncasecmp(cur_param, param_name, key_len) == 0) {
      return equal_sign + 1; // value after '='
    }
  }
  return nullptr;
}
inline server_states get_state(const server_machine *machine) {
  return machine->state;
}
inline int get_client_fd(const server_machine *machine) {
  return machine->client_fd;
}
inline size_t get_route_index(const server_machine *machine) {
  return machine->route_idx;
}

inline void set_state(server_machine *machine, server_states state) {
  machine->prev_state = machine->state;
  machine->state = state;
}
inline void set_route_index(server_machine *machine, size_t route_index) {
  machine->route_idx = route_index;
}
inline void set_client_fd(server_machine *machine, int fd) {
  machine->client_fd = fd;
}
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
    logger_log(LOG_HIGH | LOG_MEDIUM, "machine->server_ctx: %p        \n",
               machine->server_ctx);
    logger_log(LOG_HIGH | LOG_MEDIUM,
               "machine->server_ctx->free_endpoint_ctx: %p        \n",
               (void *)machine->server_ctx->free_endpoint_ctx);

    if (machine->server_ctx->endpoint_ctx &&
        machine->server_ctx->free_endpoint_ctx) {
      machine->server_ctx->free_endpoint_ctx(machine->server_ctx->endpoint_ctx);
      machine->server_ctx->endpoint_ctx = nullptr;
      machine->server_ctx->free_endpoint_ctx = nullptr;
    }
  } else {
    server_machine_ctx_init(machine);
  }

  machine->state = WAITING;
  machine->prev_state = WAITING;
#warning "Assert that client_fd never comes here as -1"
  if (machine->client_fd != -1) {
    logger_log(LOG_HIGH | LOG_MEDIUM,
               "%s:%d: closing `machine->client_fd` (%d)\n", __func__, __LINE__,
               machine->client_fd);
    close(machine->client_fd);
  }
  machine->client_fd = -1;
  machine->route_idx = 0;
  if (machine->timer != -1) {
    server_timer_stop(machine->timer);
  }
}
bool server_machine_set_endpoint_ctx(server_machine *machine, server_ctx *ctx) {
  machine->server_ctx = ctx;
  return true;
}
bool server_machine_set_free_endpoint_ctx(server_machine *machine,
                                          int free_endpoint_ctx(void *)) {
  if (!machine->server_ctx) {
    return false;
  }
  machine->server_ctx->free_endpoint_ctx = free_endpoint_ctx;
  return true;
}

bool server_machine_free_endpont_ctx(server_machine *machine) {
  if (!machine->server_ctx || !machine->server_ctx->free_endpoint_ctx ||
      !machine->server_ctx->endpoint_ctx) {
    return false;
  }
  machine->server_ctx->free_endpoint_ctx(machine->server_ctx->endpoint_ctx);
  return true;
}
