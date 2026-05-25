#include "./includes/files.h"
#include "./includes/server-defines.h"
#include "./src/server.c"
server_machine machine = {0};
#define this machine

unsigned long long current = 0;
unsigned long long get_idx() {
  current++;
  printf("idx: %llu", current);
  return current;
}
int main() {
  server s;
  printf("Initialazing server state...\n");
  if (init_server_addr(&s.addr) < 0) {
    return -1;
  }
  if (init_sock_server(&s) < 0) {
    return -1;
  }

  printf("Server initialized successfully.\n");
  server_machine_init(&this);

  for (;;) {
    init_client(&s);

    set_client_fd(&this, s.client_fd);

    server_job((void *)&this);
  }
  return 0;
}
