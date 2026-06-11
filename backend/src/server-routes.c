#include "../includes/server-routes.h"
#define KEY_VALUE_T_SENTINEL                                                   \
  (key_value_t) { 0 }
#define SERVER_ROUTES_T_SENTINEL                                               \
  (server_routes_t) { 0 }
server_routes_t _routes[] = {
    {
        .uri_regex = "GET "
                     "/api/"
                     "image\\?variant=(thumbnail|compressed|original)&id=[[:"
                     "digit:]]+[[:space:]]",
        .method = HTTP_GET,
        .headers = nullptr,
    },
    {

        .uri_regex =
            "GET "
            "/api/image/"
            "cursor\\?current=[[:digit:]]+&limit=[[:digit:]]+[[:space:]]",

        .method = HTTP_GET,
        .headers = nullptr,
        /* (const key_value_t[]){ */
        /*     { */
        /*         .key = "Connection:", */
        /*         .required = true, */
        /*         .contents = */
        /*             (const char *[]){":[[:space:]]Keep-Alive", nullptr}, */
        /*         .validators = */
        /*             (validator_func_t *[]){validator_header_key, */
        /*                                    validator_header_content, */
        /*                                    nullptr}, */
        /*     }, */
        /*     { */
        /*         .key = "Keep-Alive:", */
        /*         .required = true, */
        /*         .contents = */
        /*             (const char *[]){":[[:space:]]timeout=[[:digit:]]+,", */
        /*                              "max=[[:digit:]]+", nullptr}, */
        /*         .validators = */
        /*             (validator_func_t *[]){validator_header_key, */
        /*                                    validator_header_content, */
        /*                                    nullptr}, */
        /*     }, */
        /*     {0}, */
        /* }, */
    },
    {

        .uri_regex = "POST /api/image[[:space:]]",

        .method = HTTP_POST,
        .headers =
            (const key_value_t[]){
                {
                    .key = "Content-Length:",
                    .required = true,
                    .contents =
                        (const char *[]){":[[:space:]][[:digit:]]+", nullptr},
                    .validators =
                        (validator_func_t *[]){validator_header_key,
                                               validator_header_content,
                                               nullptr},
                },
                {0},
            },
    },
    {
        .uri_regex = "GET /api/image/cursor/start+[[:space:]]",
        .method = HTTP_GET,
        .headers = nullptr,
    },
    {
        .uri_regex = "OPTIONS /api/image/cursor/start+[[:space:]]",
        .method = HTTP_OPTIONS,
        .headers = nullptr,
    },
    {

        .uri_regex =
            "OPTIONS "
            "/api/image/"
            "cursor\\?current=[[:digit:]]+&limit=[[:digit:]]+[[:space:]]",

        .method = HTTP_OPTIONS,
        .headers = nullptr,
    },
    {
        .uri_regex = "OPTIONS "
                     "/api/"
                     "image\\?variant=(thumbnail|compressed|original)&id=[[:"
                     "digit:]]+[[:space:]]",
        .method = HTTP_OPTIONS,
        .headers = nullptr,
    },
    {

        .uri_regex = "OPTIONS /api/image[[:space:]]",

        .method = HTTP_OPTIONS,
        .headers = nullptr,
    },
    {0}, // SENTINEL VALUE
};
const server_routes_t *routes = _routes;
int routes_init() {
  for (size_t i = 0; _routes[i].uri_regex != nullptr; i++) {
    int result = regcomp(&_routes[i].compiled_uri_regex, _routes[i].uri_regex,
                         REG_EXTENDED | REG_NOSUB);
    if (result != 0) {
      return i;
    }
  }
  return 0;
}
int routes_destroy() {

  for (size_t i = 0; _routes[i].uri_regex != nullptr; i++) {
    regfree(&_routes[i].compiled_uri_regex);
  }
  return 0;
}

enum http_methods_t get_http_method(size_t index) {
  // TODO: out of bounds check
  return _routes[index].method;
}
inline const server_routes_t *get_routes() { return routes; }
