#ifndef SERVER_H
#define SERVER_H
#include "./defines.h"
#include "server-defines.h"
#include <netinet/in.h> // for sockaddr_in structure.
#include <sys/types.h>
typedef struct SERVER_ADDR {
  struct sockaddr_in addr;
  socklen_t len;
} server_addr;
typedef struct SERVER {
  int server_fd;
  int client_fd;
  server_addr addr;
} server;

typedef struct server_ctx {

  char read_buffer[BUFFER];
  stream_cursor *cursor;
  ssize_t n;

  size_t completed_headers;
  bool should_read;
  void *endpoint_ctx;
  int (*free_endpoint_ctx)(void *);
} server_ctx;

int init_server_addr(server_addr *saddr);
int init_sock_server(server *s);
int init_server(server *s);
endpoint_return server_job(void *args);
int init_client(server *s);
#endif
