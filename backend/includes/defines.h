#ifndef DEFINES_H
#define DEFINES_H
#include <assert.h>
#include <bits/posix1_lim.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
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

/**
 * TODO: documentar
 */
int find_carriage(stream_cursor *cursor, const char *stream) {
  size_t local_cur = cursor->curr;
  if (local_cur == 0 &&
      (stream[local_cur] == '\r' || stream[local_cur] == '\n') &&
      (char)*cursor->memory != '\0') {

    bool found = false;
    if (stream[local_cur] == '\r') {

      found = cursor->memory[BUFFER - 1 - (stream[0] - '\n')] == '\n' &&
              cursor->memory[BUFFER - 2 - (stream[0] - '\n')] == '\r' &&
              stream[local_cur + 1] == '\n';
    }
    if (stream[local_cur] == '\n') {
      found = cursor->memory[BUFFER - 1 - (stream[0] - '\n')] == '\r' &&
              cursor->memory[BUFFER - 2 - (stream[0] - '\n')] == '\n' &&
              cursor->memory[BUFFER - 3 - (stream[0] - '\n')] == '\r';
    }

    if (found)
      return 2;
  }
  for (; local_cur != BUFFER; // buffer is len of stream
       local_cur++, cursor->offset = cursor->offset == BUFFER - 2
                                         ? cursor->offset
                                         : cursor->offset + 1) {
    if (stream[local_cur] == '\r') {

      return 0;
    }
  }
  if (cursor->empty_mem) {
    strncpy(cursor->memory, stream, BUFFER);
    cursor->curr_mem = cursor->curr;
    cursor->curr = 0;
    cursor->offset = 0;
    return 1;
  } else {
    return -1;
  }
}
#endif // DEFINES_H
