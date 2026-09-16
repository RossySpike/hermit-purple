#include "utils.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#define BASE_10 10
bool utils_str_to_uint64(const char *string, uint64_t *out) {
  if (isalnum(*string) == 0) {
    errno = EDOM;
    return false;
  }
  uint64_t result = (uint64_t)strtoull(string, nullptr, BASE_10);
  if (result == ULLONG_MAX && errno == ERANGE) {
    return false;
  }
  *out = result;
  return true;
}
