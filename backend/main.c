#include "./includes/files.h"
#include "./includes/server-defines.h"
#include "./includes/server-machine.h"
#include "./includes/server-routes.h"
#include "./includes/server.h"
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <vips/vips.h>

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

  for (;;) {
    server_machine machine = {0};
    server_machine_init(&this);
    init_client(&s);

    set_client_fd(&this, s.client_fd);

    server_job((void *)&this);

    list_free(&this.headers, list_default_callback);
    list_free(&this.params, list_default_callback);
  }
  printf("\nHOLY SHIIIIIIT\n");
  routes_destroy();

  return 0;
}
