#ifndef DEFINES_H
#define DEFINES_H
#include <errno.h>
#include <stdio.h>
#include <string.h>
#define fperror                                                                \
  fprintf(stderr, "[-] %s. errno: %d, %s\n", __func__, errno, strerror(errno))
#ifndef bzero
#define bzero(x, len_x)                                                        \
  { memset((x), 0, (len_x)); }
#endif // bzero

#define BUFFER 2048

typedef struct {
  size_t curr;
  size_t offset;

  char memory[BUFFER];
  unsigned char *umemory;

  size_t curr_mem;
} stream_cursor;

/**
 * Parses the HTTP method from the request line.
 *
 * Expects: r is a null-terminated string with the structure "METHOD URL\r\n".
 *
 * @param r The request line string.
 * @param start_url Pointer to size_t where the function will store the position
 * where the URL starts.
 * @return The http_methods_t enum value for the method, or -1 if not found.
 *
 * Side effects:
 * - Stores the position where the URL starts in start_url.
 */
int find_carriage(stream_cursor *cursor, const char *stream) {
  size_t local_cur = cursor->curr;
  for (; local_cur != BUFFER - 1;
       local_cur++, cursor->offset = cursor->offset == BUFFER - 2
                                         ? cursor->offset
                                         : cursor->offset + 1) {
    if (stream[local_cur] == '\r') {

      if ((cursor->curr_mem == BUFFER - 3 &&
               cursor->memory[BUFFER - 3] == '\r' &&
               cursor->memory[BUFFER - 3] == '\n' && local_cur < BUFFER - 1 &&
               (stream[local_cur + 1] == '\n') ||
           cursor->curr < BUFFER - 4 && stream[cursor->curr] == '\n' &&
               stream[cursor->curr + 1] == '\r' &&
               stream[cursor->curr + 2] == '\n'))
        return 2;
      return 0;
    }
  }
  if (cursor->curr_mem == -1) {
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
