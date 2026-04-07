#include "../includes/defines.h" // for fperror,  bzero macros.
#include "../includes/server-defines.h" // for  BUFFER, PORT, BACKLOG, macros; http_methods_t enum.
#include <fcntl.h>                      // for open() function
#include <netinet/in.h> // for sockaddr_in structure.
#include <regex.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>     // for exit() function.
#include <sys/socket.h> // for socket() function.
#include <sys/stat.h>   // for fstat() function
#include <sys/types.h>  // for types.
#include <unistd.h>     // for close() function

struct __container {
  const server_routes_t route;
  size_t index;
};
#define __quick_cast                                                           \
  struct __container wtfimdoing = *(struct __container *)obj;                  \
  key_value_t param = wtfimdoing.route.params[wtfimdoing.index]

__http_sanitize_key_value_t get_key(void *obj) {
  __quick_cast;
  return (__http_sanitize_key_value_t){.key = param.key,
                                       .required = param.required};
}
__http_sanitize_key_value_t get_key_value(void *obj) {
  __quick_cast;
  return (__http_sanitize_key_value_t){
      .key = param.key, .value = param.type, .required = param.required};
}
__http_sanitize_key_value_t get_key_content(void *obj) {
  __quick_cast;
  return (__http_sanitize_key_value_t){
      .key = param.key, .content = param.contents, .required = param.required};
}

typedef struct SERVER_ADDR {
  struct sockaddr_in addr;
  socklen_t len;
} server_addr;

typedef enum server_states {
  PROCESSING_REQUEST_LINE,
  PROCESSING_HEADERS,
  PROCESSING_BODY,
  WORKING,
  FINISHED,
} server_states;

typedef struct SERVER {
  int server_fd;
  int client_fd;
  server_addr addr;
} server;

int init_server_addr(server_addr *saddr) {
  saddr->len = sizeof(saddr->addr);
  bzero(saddr, saddr->len);
  saddr->addr.sin_family = AF_INET;
  saddr->addr.sin_port = htons(PORT);
  saddr->addr.sin_addr.s_addr = htonl(INADDR_ANY);
  return 0;
}

int init_sock_server(server *s) {
  int optval = 1;
  s->server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (s->server_fd < 0) {
    fperror;
    return -1;
  }
  if (setsockopt(s->server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval,
                 sizeof(optval))) {
    close(s->server_fd);
    fperror;
    return -1;
  }
  if (bind(s->server_fd, (struct sockaddr *)&s->addr.addr, s->addr.len) < 0) {
    fperror;
    close(s->server_fd);
    return -1;
  }
  if (listen(s->server_fd, BACKLOG) < 0) {
    fperror;
    close(s->server_fd);
    return -1;
  }

  return 0;
}
// 3
int init_rserver(server *s) {
  printf("Initialazing server state...\n");
  if (init_server_addr(&s->addr) < 0) {
    return -1;
  }
  if (init_sock_server(s) < 0) {
    return -1;
  }
  printf("Server initialized successfully.\n");
  return 0;
}
int init_client(server *s) {
  printf("Waiting for client connection on PORT:%d...\n", PORT);
  s->client_fd =
      accept(s->server_fd, (struct sockaddr *)&s->addr.addr, &s->addr.len);
  if (s->client_fd < 0) {
    fperror;
    close(s->server_fd);
    return -1;
  }
  printf("Client connected successfully.\n");
  return 0;
}
int destroy_server(server *s) {
  if (s->client_fd >= 0) {
    close(s->client_fd);
    s->client_fd = -1;
  }
  if (s->server_fd >= 0) {
    close(s->server_fd);
    s->client_fd = -1;
  }
  printf("Server destroyed successfully.\n");
  return 0;
}

enum http_methods_t get_method(char *r, size_t *start_url) {
  char *first_space = strchr(r, ' ');
  if (first_space == NULL) {
    return -1; // No space found, invalid request
  }
  first_space++;
  int len = first_space - r;

  if (start_url != NULL)
    *start_url = len; // Set start_url to the position after the method
  char method[len];
  method[len - 1] = '\0';
  for (int i = 0; i < len - 1; i++) {
    method[i] = r[i];
  }
  if (strcmp(method, "GET") == 0) {
    return HTTP_GET; // GET method
  } else if (strcmp(method, "POST") == 0) {
    return HTTP_POST; // POST method
  } else if (strcmp(method, "PUT") == 0) {
    return HTTP_PUT; // PUT method
  } else if (strcmp(method, "DELETE") == 0) {
    return HTTP_DELETE; // DELETE method
  } else {
    return -1; // Unknown method
  }
}

extern struct server_machine;
typedef struct server_machine server_machine;
extern void set_state(server_machine *machine, server_states state);
extern server_states get_state(server_machine *machine);
extern int get_client_fd(server_machine *machine);
extern void set_route_index(server_machine *machine, size_t route_index);
extern size_t get_route_index(server_machine *machine);

size_t check_url(char *url) {
  regex_t regex;
  size_t i = 0;
  const server_routes_t *local_routes = routes;
  for (; local_routes != NULL; local_routes++) {
    if (regcomp(&regex, local_routes->uri_regex, REG_EXTENDED) == 0) {
      if (regexec(&regex, url, 0, NULL, 0) == 0) {
        regfree(&regex);
        return i;
      }

      regfree(&regex);
    }
  }
  return -1;
}

bool validate_params(server_routes_t route, char *url) {
  for (size_t i = 0; route.params != NULL; route.params++) {
    for (size_t f; route.params->validators != NULL;
         route.params->validators++) {
      struct __container obj = {.route = route, .index = i};
      if (route.params->validators[f]((void *)url, (void *)&obj) == false)
        return false;
      f++;
    }

    i++;
  }
  return true;
}
void *server_job(void *args) {
  server_machine *local_machine = (server_machine *)args;

  char read_buffer[BUFFER] = {0};
  char send_buffer[BUFFER] = {0};
  set_state(local_machine, PROCESSING_REQUEST_LINE);
  int n = 0;
  while ((n = read(get_client_fd(local_machine), read_buffer, BUFFER)) > 0) {

    switch (get_state(local_machine)) {

    case PROCESSING_REQUEST_LINE: {
      size_t i = 1;
      enum http_methods_t method = get_method(read_buffer, &i);
      char *url = read_buffer + i;
      size_t route_index = check_url(url);
      if (route_index == -1 || !validate_params(routes[route_index], url)) {
        snprintf(send_buffer, BUFFER, "HTTP/1.1 404 Not Found\r\n\r\n");
        write(get_client_fd(local_machine), send_buffer,
              strlen(send_buffer)); // TODO: add error handling
        set_state(local_machine, FINISHED);
        break;
      }
      set_route_index(local_machine, route_index);
      if (routes[route_index].headers != NULL) {
        set_state(local_machine, PROCESSING_HEADERS);
        break;
      }
      if (routes[route_index].need_body) {
        set_state(local_machine, PROCESSING_BODY);
        break;
      }
      set_state(local_machine, WORKING);
      break;
    }
    case PROCESSING_HEADERS: {

      set_state(local_machine, PROCESSING_BODY);
      set_state(local_machine, FINISHED);
      break;
    }
    case PROCESSING_BODY: {

      set_state(local_machine, FINISHED);
      break;
    }
    case WORKING: {
      break;
    }
    case FINISHED: {
      break;
    }
    }
  }
  return (void *)0;
}
