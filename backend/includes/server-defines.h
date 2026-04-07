#ifndef SERVER_DEFINES_H

#define SERVER_DEFINES_H
#include "http-sanitize.h"
#include <regex.h>
#include <stdbool.h>
#include <string.h>

#define PORT 1600
#define BACKLOG 1
#define BUFFER 8192


#define HTTP_VERSION "HTTP/1.1"

enum http_methods_t { HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_DELETE };

typedef struct key_value_t {
  const char *key;
  enum key_value_types type;
  bool required;
  const char ** contents;
  validator_func_t **validators;
} key_value_t;
typedef struct server_routes_t {
  const char *uri_regex;
  enum http_methods_t method;
  key_value_t *params;
  key_value_t *headers;
  bool need_body;
} server_routes_t;
const server_routes_t *routes = (server_routes_t[]){
    {
        .uri_regex ="^https?://[[:digit:]]{1,3}\\.[[:digit:]]{1,3}\\.[[:digit:]]{1,3}\\.[[:digit:]]{1,3}:[[:digit:]]{1,5}/api/image\\?variant=(thumbnail|compressed|original)[[:space:]]",
        .method = HTTP_GET,
        .params =
            (key_value_t[]){
                {
                    .key = "variant=",
                    .type = PARAM_STRING,
                    .required = true,
                    .contents=(const char *[]){"thumbnail","compressed","original",NULL},
                    .validators =
                        (validator_func_t *[]){
                            validator_param_key,
                            validator_param_type,
                            validator_param_key_content,
                            NULL,
                        },
                },
                NULL,
            },
        .headers = NULL,
        .need_body=false,
    },
    {

        .uri_regex = "^https?://[[:digit:]]{1,3}\\.[[:digit:]]{1,3}\\.[[:digit:]]{1,3}\\.[[:digit:]]{1,3}:[[:digit:]]{1,5}/api/image/cursor\\?current=[[:digit:]]+&limit=[[:digit:]]+[[:space:]]",

        .method = HTTP_GET,
        .params =
            (key_value_t[]){
                {
                    .key = "current=",
                    .type = PARAM_INT,
                    .required = true,
                    .validators =
                        (validator_func_t *[]){
                            validator_param_key,
                            validator_param_type,
                            NULL,
                        },
                },
                {
                    .key = "limit=",
                    .type = PARAM_INT,
                    .required = true,
                    .validators =
                        (validator_func_t *[]){
                            validator_param_key,
                            validator_param_type,
                            NULL,
                        },
                },

                NULL,
            },

        .headers =
            (key_value_t[]){
                {
                    .key = "Connection:",
                    .required = true,
                    .validators = (validator_func_t *[]){validator_header_key},
                },
                {
                    .key = "Keep-Alive:",
                    .required = true,
                    .validators = (validator_func_t *[]){validator_header_key},
                },
                NULL,
            },
        .need_body=false,
    },
    {

        .uri_regex = "^https?://[[:digit:]]{1,3}\\.[[:digit:]]{1,3}\\.[[:digit:]]{1,3}\\.[[:digit:]]{1,3}:[[:digit:]]{1,5}/api/image[[:space:]]",

        .method = HTTP_POST,
        .params =
            (key_value_t[]){
                NULL,
            },

        .headers =
            (key_value_t[]){
                {
                    .key = "Content-Type:", // multipart/form-data
                    .required = true,
                    .validators = (validator_func_t *[]){validator_header_key},
                },
                NULL,
            },
        .need_body=true,
    },
    NULL,
};

#endif // !#ifndef SERVER_DEFINES_H
