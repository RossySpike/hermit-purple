#include "../includes/server-defines.h"

typedef struct server_routes_t {
  const char *uri_regex;
  enum http_methods_t method;
  key_value_t **params; // TODO: cambiar para contiguidad en memoria
  key_value_t **headers; // TODO: cambiar para contiguidad en memoria
} server_routes_t;
const server_routes_t *routes = (server_routes_t[]){
    {
        .uri_regex ="/api/image\\?variant=(thumbnail|compressed|original)[[:space:]]",
        .method = HTTP_GET,
        .params = 
            (key_value_t*[]){
                &(key_value_t){
                    .key = "variant=",
                    .type = PARAM_STRING,
                    .required = true,
                    .contents = (const char *[]){"thumbnail", "compressed",
                                                 "original", NULL},
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
        .headers =            (key_value_t*[]){
 NULL},
    },
    {

        .uri_regex ="/api/image/cursor\\?current=[[:digit:]]+&limit=[[:digit:]]+[[:space:]]",

        .method = HTTP_GET,
        .params =
            (key_value_t*[]){
                &(key_value_t){
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
                &(key_value_t){
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
            (key_value_t*[]){
                &(key_value_t){
                    .key = "Connection:",
                    .required = true,
        .contents = (const char *[]){":[[:space:]]Keep-Alive",NULL},
                    .validators = (validator_func_t *[]){validator_header_key,validator_header_content,NULL},
                },
                &(key_value_t){
                    .key = "Keep-Alive:",
                    .required = true,
        .contents = (const char *[]){":[[:space:]]timeout=[[:digit:]]+,","max=[[:digit:]]+",NULL},
                    .validators = (validator_func_t *[]){validator_header_key,validator_header_content,},
                },
                NULL,
            },
    },
    {

        .uri_regex = "/api/image[[:space:]]",

        .method = HTTP_POST,
        .params =
            (key_value_t*[]){
                NULL,
            },

        .headers =
            (key_value_t*[]){
                &(key_value_t){
                    .key = "Content-Length:",
                    .required = true,
        .contents = (const char *[]){":[[:space:]][[:digit:]]+",NULL},
                    .validators = (validator_func_t *[]){validator_header_key,validator_header_content, NULL},
                },
                NULL,
            },
    },
    { NULL }, // SENTINEL VALUE
};

enum http_methods_t get_http_method(size_t index){
  printf("DEBUG: %s\n", __func__);
  // TODO: out of bounds check
  return  routes[index].method;
}
