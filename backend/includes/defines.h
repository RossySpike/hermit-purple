#ifndef DEFINES_H
#define DEFINES_H
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <unistd.h>
#define fperror                                                                \
  fprintf(stderr, "DEBUG: %s. errno: %d, %s\n", __func__, errno,               \
          strerror(errno))
#ifndef SSIZE_MAX
#define SSIZE_MAX LONG_MAX
#endif
#define BUFFER (1024 * 8)
static_assert(BUFFER < SSIZE_MAX);

typedef struct {
  size_t curr;
  size_t offset;

  char memory[BUFFER];

  bool empty_mem;
  size_t curr_mem;
  size_t read_bytes;
} stream_cursor;
// has an index to the current character of the stream and a buffer to
// store old values from the stream that need to be used

typedef enum {
  CARRIAGE_NOT_FOUND_MEM_NOT_EMPTY = -1,
  CARRIAGE_FOUND = 0,
  CARRIAGE_NOT_FOUND_MEM_PUSHED = 1,
  CARRIAGE_DOUBLE = 2,
} carriage_status;
carriage_status find_carriage(stream_cursor *cursor, const char *stream);

#endif // DEFINES_H
