#ifndef SERVER_MACHINE_H
#define SERVER_MACHINE_H
#include "../src/server-machine.c"
#include "server-defines.h"
typedef struct server_machine server_machine;
void server_machine_init(server_machine *machine);

server_states get_state(server_machine *machine);
int get_client_fd(server_machine *machine);
size_t get_route_index(server_machine *machine);
const char *get_header(server_machine *machine, const char *header_name);

void set_client_fd(server_machine *machine, int fd);
void set_state(server_machine *machine, server_states state);
void set_route_index(server_machine *machine, size_t route_index);
bool add_header(server_machine *machine, char *header);
bool add_params(server_machine *machine, char *uri);

#endif // SERVER_MACHINE_H
