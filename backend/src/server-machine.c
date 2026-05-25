#include "../includes/list.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
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

server_states get_state(server_machine *machine) { return machine->state; }
int get_client_fd(server_machine *machine) { return machine->client_fd; }
size_t get_route_index(server_machine *machine) { return machine->route_idx; }

void set_state(server_machine *machine, server_states state) {
  machine->state = state;
}
void set_route_index(server_machine *machine, size_t route_index) {
  machine->route_idx = route_index;
}
bool add_header(server_machine *machine, char *header) {
  assert(&machine->headers != NULL);

  printf("DEBUG: adding header %s", header);
  list_append(&machine->headers, (void *)header);

  return true;
}
bool add_params(server_machine *machine, char *uri) {
  assert(&machine->params != NULL);

  list_append(&machine->params, (void *)uri);
  return true;
}
const char *get_header(server_machine *machine, const char *header_name) {
  list_t *headers = &machine->headers; // Simplificar

  for (size_t i = 0; i < headers->len; i++) {
    const char *cur_head = (const char *)headers->array[i];
    printf("DEBUG: checking header '%s' against '%s'\n", cur_head, header_name);

    // Buscar ':' en el header
    const char *colon = strchr(cur_head, ':');
    if (colon == NULL)
      continue;

    // Comparar el key (hasta ':') con header_name
    size_t key_len = colon - cur_head;
    if (strncasecmp(cur_head, header_name, key_len) == 0) {
      // Retornar el valor (después de ':' y espacios)
      return machine->headers.array[i]; // Retorna el header completo, el caller
                                        // debe extraer el valor después de ':'
    }
  }
  return NULL;
}
void set_client_fd(server_machine *machine, int fd) { machine->client_fd = fd; }
