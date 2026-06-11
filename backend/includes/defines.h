#ifndef DEFINES_H
#define DEFINES_H
#include <bits/posix1_lim.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#define fperror                                                                \
  fprintf(stderr, "DEBUG: %s. errno: %d, %s\n", __func__, errno,               \
          strerror(errno))

#define BUFFER 1024 * 8
static_assert(BUFFER < SSIZE_MAX);

typedef struct {
  size_t curr;
  size_t offset;

  char memory[BUFFER];

  bool empty_mem;
  size_t curr_mem;
} stream_cursor;
int find_carriage(stream_cursor *cursor, const char *stream);

#endif // DEFINES_H
