#ifndef HTTP_SANITIZE_H
#define HTTP_SANITIZE_H
#include "defines.h"
#include <errno.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
enum key_value_types { PARAM_INT, PARAM_FLOAT, PARAM_STRING };
typedef struct key_value_t {
  const char *key;
  enum key_value_types type;
  bool required;
  const char **contents;
  bool (**validators)(const struct key_value_t, const char *);
} key_value_t;
typedef bool(validator_func_t)(const key_value_t, const char *);

int check_regex(const char *haystack, const char *pattern) {
  printf("DEBUG: %s\n", __func__);
  regex_t regex;
  int found = 0;
  if (regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB) == 0) {

    if (regexec(&regex, haystack, 0, nullptr, 0) == 0) {

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

bool validator_header_key(key_value_t header, const char *data) {
  printf("DEBUG: %s\n", __func__);
  if (data == nullptr)
    return false;

  const char *head = header.key;
  // Buscar ':' en needle PRIMERO
  const char *needle = data;
  const char *colon = strchr(needle, ':');
  if (colon == nullptr) {

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

bool validator_header_content(key_value_t header, const char *data) {
  printf("DEBUG: %s\n", __func__);
  if (data == nullptr) {

    printf("DEBUG:  `data` is null\n");
    return false;
  }

  const char **contents = header.contents;
  const char *needle = data;

  if (contents == nullptr) {
    printf("DEBUG: `contents` is null\n");

    return false;
  }

  // Buscar el ':' en needle
  const char *colon = strchr(needle, ':');
  if (colon == nullptr) {

    printf("DEBUG: `needle` doesnt have `:`\n");
    return false;
  }

  printf("DEBUG: `colon`=`%s`\n", colon);
  for (size_t i = 0; contents[i] != nullptr; i++) {
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
