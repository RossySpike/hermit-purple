#ifndef SERVER_MACHINE_H
#define SERVER_MACHINE_H
#include "../includes/list.h"
#include "server-defines.h"
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
typedef struct server_machine server_machine;
void server_machine_init(server_machine *machine);

server_states get_state(const server_machine *machine);
int get_client_fd(const server_machine *machine);
size_t get_route_index(const server_machine *machine);

void set_state(server_machine *machine, server_states state);
void set_route_index(server_machine *machine, size_t route_index);
void set_client_fd(server_machine *machine, int fd);
const char *get_header(const server_machine *machine, const char *header_name);

bool add_header(server_machine *machine, const char *header);
bool add_params(server_machine *machine, const char *uri);
const char *get_param(const server_machine *machine, const char *param_name);

#endif // SERVER_MACHINE_H
