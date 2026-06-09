#include "../includes/server-defines.h"
#include <stddef.h>
#define KEY_VALUE_T_SENTINEL                                                   \
  (key_value_t) { 0 }
#define SERVER_ROUTES_T_SENTINEL                                               \
  (server_routes_t) { 0 }
typedef struct server_routes_t {
  const char *uri_regex;
  const enum http_methods_t method;
  const key_value_t *const headers;
} server_routes_t;
const server_routes_t routes[] = {
    {
        .uri_regex =
            "GET /api/image\\?variant=(thumbnail|compressed|original)&id=[[:digit:]]+[[:space:]]",
        .method = HTTP_GET,
        .headers = nullptr,
    },
    {

        .uri_regex =
            "GET /api/image/cursor\\?current=[[:digit:]]+&limit=[[:digit:]]+[[:space:]]",

        .method = HTTP_GET,
        .headers = 
    nullptr,
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
    {0}, // SENTINEL VALUE
};

enum http_methods_t get_http_method(size_t index) {
  printf("DEBUG: %s\n", __func__);
  // TODO: out of bounds check
  return routes[index].method;
}
