#include "../includes/list.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
typedef enum server_states {
  WAITING,
  PROCESSING_REQUEST_LINE,
  PROCESSING_HEADERS,
  WORKING,
  ENDING,
} server_states;
typedef struct server_machine {
  server_states state;
  int client_fd;
  size_t route_idx;
  list_t headers;
  list_t params;
} server_machine;

void server_machine_init(server_machine *machine) {
  list_new(&machine->headers);
  list_new(&machine->params);
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
  machine->state = state;
}
inline void set_route_index(server_machine *machine, size_t route_index) {
  machine->route_idx = route_index;
}
bool add_header(server_machine *machine, const char *header) {
  if (machine == nullptr || header == nullptr)
    return false;

  printf("DEBUG: adding header %s", header);
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
  const list_t *headers = &machine->headers; // Simplificar

  for (size_t i = 0; i < headers->len; i++) {
    const char *cur_head = (const char *)headers->array[i];
    printf("DEBUG: checking header '%s' against '%s'\n", cur_head, header_name);

    // Buscar ':' en el header
    const char *colon = strchr(cur_head, ':');
    if (colon == nullptr)
      continue;

    // Comparar el key (hasta ':') con header_name
    size_t key_len = colon - cur_head;
    if (strncasecmp(cur_head, header_name, key_len) == 0) {
      // Retornar el valor (después de ':' y espacios)
      return machine->headers.array[i]; // Retorna el header completo, el caller
                                        // debe extraer el valor después de ':'
    }
  }
  return nullptr;
}
inline void set_client_fd(server_machine *machine, int fd) {
  machine->client_fd = fd;
}
const char *get_param(const server_machine *machine, const char *param_name) {
  const list_t *params = &machine->params; // Simplificar

  for (size_t i = 0; i < params->len; i++) {
    const char *cur_param = (const char *)params->array[i];
    printf("DEBUG: checking param '%s' against '%s'\n", cur_param, param_name);

    // Buscar '=' en el param
    const char *equal_sign = strchr(cur_param, '=');
    if (equal_sign == nullptr) //?
      continue;

    // Comparar el key (hasta '=') con param_name
    size_t key_len = equal_sign - cur_param;
    if (strncasecmp(cur_param, param_name, key_len) == 0) {
      return equal_sign + 1;
      // Retornar el valor (después de '=' y espacios)
      /* return machine->params.array[i]; // Retorna el param completo, el
       * caller */

      // debe extraer el valor después de '='
    }
  }
  return nullptr;
}
