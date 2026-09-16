#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

/*
 * If string starts with a whitespace or a sign, the function fails and sets
 * ERRNO to EDOM If the number from `string`  would cause overflow then ERRNO is
 * set to ERANGE
 */
[[nodiscard]] bool utils_str_to_uint64(const char *string, uint64_t *out);
#endif // UTILS_H
