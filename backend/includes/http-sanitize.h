#ifndef HTTP_SANITIZE_H
#define HTTP_SANITIZE_H
#include "defines.h"
#include <errno.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum key_value_types { PARAM_INT, PARAM_FLOAT, PARAM_STRING };
typedef bool(validator_func_t)(void *, void *);
typedef struct key_value_t {
  const char *key;
  enum key_value_types type;
  bool required;
  const char **contents;
  validator_func_t **validators;
} key_value_t;

int check_regex(const char *haystack, const char *pattern) {
  printf("DEBUG: %s\n", __func__);
  regex_t regex;
  int found = 0;
  if (regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB) == 0) {

    if (regexec(&regex, haystack, 0, NULL, 0) == 0) {

      found = 1;
    }
    regfree(&regex);
    return found;
  } else {
    perror("FALLO EL PATRON");
    return -1;
  }
}
// key should be: 'key_name='
// WARNING: "content" needs to be sanitized as its expected by this functions,
// or else will break
typedef struct __http_sanitize_key_value_t {
  char *key;
  union {
    enum key_value_types value;
    const char **content;
  };
  bool required;
} __http_sanitize_key_value_t;

bool validator_param_key(void *uri, void *key);
bool validator_param_type(void *uri, void *key_value);
// Checks only key and content, assumes its already been verified that the
// content is of the correct type if theres a key thats not defined in
// key_content wont care for it if the defined key and contents are valid it
// will return true.
bool validator_param_key_content(void *uri, void *key_content);
bool validator_header_key(void *header, void *data);
bool validator_header_content(void *header, void *data);

extern __http_sanitize_key_value_t get_key(void *obj);
extern __http_sanitize_key_value_t get_key_value(void *obj);
extern __http_sanitize_key_value_t get_key_content(void *obj);

bool validator_param_key(void *uri, void *obj) {
  printf("DEBUG: %s\n", __func__);
  char *queryParams = strstr((char *)uri, "?");
  if (queryParams == NULL || *(++queryParams) == ' ')
    return false;
  __http_sanitize_key_value_t data = get_key(obj);
  if (strstr(queryParams, data.key) == NULL && data.required)
    return false;
  return true;
}
bool validator_param_type(void *uri, void *obj) {
  printf("DEBUG: %s\n", __func__);
  char *queryParams = strstr((char *)uri, "?");
  if (queryParams == NULL || *(++queryParams) == ' ')
    return false;
  __http_sanitize_key_value_t key_value = get_key_value(obj);
  char *needle = strstr(queryParams, key_value.key);
  if (needle == NULL && key_value.required)
    return false;
  needle = strstr(queryParams, "=");
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
    if (*needle == '&' && *needle == '\r' && *needle == '\0' && *needle == ' ')
      return false;

    break;
  }
  return true;
}

bool validator_param_key_content(void *uri, void *obj) {
  printf("DEBUG: %s\n", __func__);
  char *queryParams = strstr((char *)uri, "?");
  if (queryParams == NULL || *(++queryParams) == ' ')
    return false;
  __http_sanitize_key_value_t key_content = get_key_content(obj);
  char *needle = strstr(queryParams, key_content.key);

  if (needle == NULL && key_content.required)
    return false;
  // WARNING: Expects content to be sanitized for http

  needle = strstr(queryParams, "=");
  needle++;
  if (*needle == '&' || *needle == '\r' || *needle == '\0' || *needle == ' ')
    return false;
  char *endptr = needle;
  size_t index = 0;
  for (; *endptr != '&' && *endptr != '\r' && *endptr != '\0' && *endptr != ' ';
       endptr++, index = index == BUFFER - 1 ? BUFFER - 1 : index + 1)
    ;
  ;
  char previous_char = *endptr;
  needle[index] = '\0';

  for (size_t i = 0; key_content.content[i] != NULL; i++) {
    if (strcmp(needle, key_content.content[i]) == 0)

      needle[index] = previous_char;
    return true;
  }
  return true;
}
bool validator_header_key(void *header, void *data) {
  printf("DEBUG: %s\n", __func__);
  if (header == NULL || data == NULL)
    return false;

  const char *head = ((const key_value_t *)header)->key;
  char *needle = (char *)data;

  if (head == NULL || needle == NULL) {
    printf("DEBUG: `head` or `needle` is null\n");

    return false;
  }

  // Buscar ':' en needle PRIMERO
  char *colon = strchr(needle, ':');
  if (colon == NULL) {

    printf("DEBUG: `needle` doesnt have `:`\n");
    return false;
  }

  // Comparar solo hasta el ':' en needle
  size_t key_len = colon - needle;
  size_t head_len = strlen(head);

  // head puede incluir ':' al final
  if (head_len > 0 && head[head_len - 1] == ':') {
    head_len--;
  }

  printf("DEBUG VHK: head='%s', head_len=%zu\n", head, head_len);
  printf("DEBUG VHK: needle='%.*s', key_len=%zu\n", (int)key_len, needle,
         key_len);

  if (key_len != head_len) {
    printf("DEBUG: `needle` has a bad format\n");

    return false;
  }

  int result = strncasecmp(head, needle, head_len);
  printf("DEBUG VHK: strncasecmp result=%d\n", result);

  return result == 0;
}

bool validator_header_content(void *header, void *data) {
  printf("DEBUG: %s\n", __func__);
  if (header == NULL || data == NULL) {

    printf("DEBUG: `head` or `data` is null\n");
    return false;
  }

  const char **contents = ((const key_value_t *)header)->contents;
  char *needle = (char *)data;

  if (contents == NULL) {
    printf("DEBUG: `contents` is null\n");

    return false;
  }

  // Buscar el ':' en needle
  char *colon = strchr(needle, ':');
  if (colon == NULL) {

    printf("DEBUG: `needle` doesnt have `:`\n");
    return false;
  }

  printf("DEBUG: `colon`=`%s`\n", colon);
  for (size_t i = 0; contents[i] != NULL; i++) {
    printf("DEBUG: `contents[i:%lu]`:`%s`\n", i, contents[i]);
    int result = check_regex(colon, contents[i]);
    if (result == 1)
      return true;
    if (result == -1) {

      printf("DEBUG: `check_regex` failed\n");
      return false;
    }
  }
  printf("DEBUG VHC: not found\n");
  return false;
}
#endif // !#ifndef HTTP_SANITIZE_H
