#include "../includes/http-sanitize.h"
#include "../includes/defines.h"
#include <stdio.h>
#include <strings.h>

int check_regex_with_regex(const char *haystack, const regex_t *regex) {
  int found = 0;
  if (regexec(regex, haystack, 0, nullptr, 0) == 0) {

    found = 1;
  }
  return found;
}
int check_regex(const char *haystack, const char *pattern) {
  regex_t regex;
  int found = 0;
  if (regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB) == 0) {

    if (regexec(&regex, haystack, 0, nullptr, 0) == 0) {

      found = 1;
    }
    regfree(&regex);
    return found;
  } else {
    return -1;
  }
}
// key should be: 'key_name='
// WARNING: "content" needs to be sanitized as its expected by this functions,
// or else will break

bool validator_header_key(key_value_t header, const char *data) {
  if (data == nullptr)
    return false;

  const char *head = header.key;
  const char *needle = data;
  const char *colon = strchr(needle, ':');
  if (colon == nullptr) {

    return false;
  }

  size_t key_len = colon - needle;
  size_t head_len = strlen(head);

  if (head_len > 0 && head[head_len - 1] == ':') {
    head_len--;
  }

  if (key_len != head_len) {

    return false;
  }

  int result = strncasecmp(head, needle, head_len);

  return result == 0;
}

bool validator_header_content(key_value_t header, const char *data) {
  if (data == nullptr) {

    return false;
  }

  const char **contents = header.contents;
  const char *needle = data;

  if (contents == nullptr) {

    return false;
  }

  const char *colon = strchr(needle, ':');
  if (colon == nullptr) {

    return false;
  }

  for (size_t i = 0; contents[i] != nullptr; i++) {
    int result = check_regex(colon, contents[i]);
    if (result == 1)
      return true;
    if (result == -1) {

      return false;
    }
  }
  return false;
}
