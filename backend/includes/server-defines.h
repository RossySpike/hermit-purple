#ifndef SERVER_DEFINES_H
#define SERVER_DEFINES_H
#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h> // for close() function

// TODO: add options method to endpoints in order to do that, ask for the
// current method
#define HOST_NAME "hermit"

#define PORT 1600

typedef enum {
  LOG_DISABLED = 0,
  LOG_BASIC = (1 << 0),
  LOG_MEDIUM = (1 << 1),
  LOG_HIGH = (1 << 2)
} log_level;
#define log_level_t size_t

#define LOG_LEVEL LOG_BASIC
// #define LOG_FILE

#define BACKLOG 1

#define HTTP_VERSION "HTTP/1.1"

#define IMG_ORIGINAL_DIR "/tmp/original/"
#define IMG_THUMBNAIL_DIR "/tmp/thumbnail/"
#define IMG_CACHE_DIR "/tmp/cache/"
#define MAX_BYTES_SIZE 134217700 // 1GiB

enum http_methods_t {
  HTTP_GET,
  HTTP_DELETE,
  HTTP_POST,
  HTTP_PUT,
  HTTP_OPTIONS
};
typedef enum {
  FINISHED,
  NEED_READ_MORE_DATA,
  NEED_WRITE_MORE_DATA,
  SOMETHING_WENT_WRONG
} endpoint_return;
typedef enum CONTENT_TYPE { TEXT_PLAIN, CURSOR_BINARY_FORMAT } CONTENT_TYPE;
extern const char *const content_type_value;
#endif // !#ifndef SERVER_DEFINES_H
