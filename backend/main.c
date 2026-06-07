#include "./includes/files.h"
#include "./includes/server-defines.h"
#include "./src/server.c"
#include <assert.h>
server_machine machine = {0};
#define this machine

unsigned long long current = 0;
unsigned long long get_idx() {
  if (current == 0) {

    current = get_biggest_index(IMG_ORIGINAL_DIR);
  }

  current++;

  printf("idx: %llu\n", current);
  return current;
}

int main() {
  int res = attempt_create_dir(IMG_ORIGINAL_DIR, S_IRUSR | S_IWUSR | S_IXUSR);
  assert(res != -1);

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
  list_free(&this.headers, list_default_callback);

  free(machine.params.array);
  return 0;
}
