#include "../includes/server-machine.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
void server_machine_init(server_machine *machine) {
  machine->state = WAITING;
  machine->server_ctx = (server_ctx *)calloc(1, sizeof(server_ctx));
  machine->server_ctx->cursor =
      (stream_cursor *)calloc(1, sizeof(stream_cursor));
  machine->server_ctx->endpoint_ctx = nullptr;
  machine->server_ctx->free_endpoint_ctx = nullptr;
  machine->server_ctx->should_read = true;
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
