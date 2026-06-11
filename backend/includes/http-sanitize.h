#ifndef HTTP_SANITIZE_H
#define HTTP_SANITIZE_H
#include <regex.h>
enum key_value_types { PARAM_INT, PARAM_FLOAT, PARAM_STRING };
typedef struct key_value_t {
  const char *key;
  enum key_value_types type;
  bool required;
  const char **contents;
  bool (**validators)(const struct key_value_t, const char *);
} key_value_t;
typedef bool(validator_func_t)(const key_value_t, const char *);

int check_regex_with_regex(const char *haystack, const regex_t *regex);
int check_regex(const char *haystack, const char *pattern);

bool validator_header_key(key_value_t header, const char *data);

bool validator_header_content(key_value_t header, const char *data);
#endif // !#ifndef HTTP_SANITIZE_H
