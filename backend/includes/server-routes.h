#ifndef SERVER_ROUTES_H
#define SERVER_ROUTES_H
#include "../includes/server-defines.h"

#include <stddef.h>
typedef struct server_routes_t {
  const char *uri_regex;
  const enum http_methods_t method;
  const key_value_t *const headers;
  regex_t compiled_uri_regex;
} server_routes_t;
int routes_init();
extern const server_routes_t *routes;
const server_routes_t *get_routes();
int routes_destroy();
enum http_methods_t get_http_method(size_t index);
#endif
