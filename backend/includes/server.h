#ifndef SERVER_H
#define SERVER_H
#include <netinet/in.h> // for sockaddr_in structure.
typedef struct SERVER_ADDR {
  struct sockaddr_in addr;
  socklen_t len;
} server_addr;
typedef struct SERVER {
  int server_fd;
  int client_fd;
  server_addr addr;
} server;
int init_server_addr(server_addr *saddr);
int init_sock_server(server *s);
int init_server(server *s);
void *server_job(void *args);
int init_client(server *s);
#endif
