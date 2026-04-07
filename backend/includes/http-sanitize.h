#ifndef HTTP_SANITIZE_H
#define HTTP_SANITIZE_H
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
enum key_value_types { PARAM_INT, PARAM_FLOAT, PARAM_STRING };
typedef bool(validator_func_t)(void *, void *);

// key should be: 'key_name='
// WARNING: "content" needs to be sanitized as its expected by this functions,
// or else will break
typedef struct __http_sanitize_key_value_t {
  char *key;
  union {
    enum key_value_types value;
    char *content;
  };
  bool required;
} __http_sanitize_key_value_t;

extern __http_sanitize_key_value_t get_key(void *obj);
extern __http_sanitize_key_value_t get_key_value(void *obj);
extern __http_sanitize_key_value_t get_key_content(void *obj);

bool validator_param_key(void *uri, void *key);
bool validator_param_type(void *uri, void *key_value);
// Checks only key and content, assumes its already been verified that the
// content is of the correct type if theres a key thats not defined in
// key_content wont care for it if the defined key and contents are valid it
// will return true.
bool validator_param_key_content(void *uri, void *key_content);
bool validator_header_key(void *header, void *key);

bool validator_param_key(void *uri, void *obj) {
  char *queryParams = strstr((char *)uri, "?");
  if (queryParams == NULL || *(++queryParams) == ' ')
    return false;
  __http_sanitize_key_value_t data = get_key(obj);
  if (strstr(queryParams, data.key) == NULL && data.required)
    return false;
  return true;
}
bool validator_param_type(void *uri, void *obj) {
  char *queryParams = strstr((char *)uri, "?");
  if (queryParams == NULL || *(++queryParams) == ' ')
    return false;
  __http_sanitize_key_value_t key_value = get_key_value(obj);
  char *needle = strstr(queryParams, key_value.key);
  if (needle == NULL && key_value.required)
    return false;
  needle++;
  char *endptr;
  errno = 0;
  switch (key_value.value) {

    // I dont think I should test for '\0'  ' ' bc the structure is:
    // METHOD /url/ HTTP_VERSION
  case PARAM_INT:
    strtol(needle, &endptr, 10);
    if (errno != 0 || (*endptr != '&' && *endptr != '\r' && *endptr != '\0' &&
                       *endptr != ' ')) {
      return false;
    }
    break;
  case PARAM_FLOAT:
    strtod(needle, &endptr);
    if (errno != 0 || (*endptr != '&' && *endptr != '\r' && *endptr != '\0' &&
                       *endptr != ' ')) {
      return false;
    }
    break;
  case PARAM_STRING:
    needle++;
    if (*needle != '&' && *needle != '\r' && *needle != '\0' && *needle != ' ')
      return false;

    break;
  }
  return true;
}

bool validator_param_key_content(void *uri, void *obj) {
  char *queryParams = strstr((char *)uri, "?");
  if (queryParams == NULL || *(++queryParams) == ' ')
    return false;
  __http_sanitize_key_value_t key_content = get_key_content(obj);
  char *needle = strstr(queryParams, key_content.key);

  if (needle == NULL && key_content.required)
    return false;
  // WARNING: Expects content to be sanitized for http

  for (size_t i = 0; i < strlen(key_content.content); i++) {
    needle++;
    if (*needle != key_content.content[i])
      return false;
  }
  needle++;
  if (*needle != '&' && *needle != '\r' && *needle != '\0' && *needle != ' ')
    return false;
  return true;
}
bool validator_header_key(void *header, void *key) {
  size_t i = 0;
  char *head = (char *)header;
  char *needle = (char *)key;
  for (; needle[i] != '\0'; i++) {

    if (head[i] != needle[i])
      return false;
  }

  return true;
}

#endif // !#ifndef HTTP_SANITIZE_H
