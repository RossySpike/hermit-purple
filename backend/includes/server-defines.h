#ifndef SERVER_DEFINES_H

#define SERVER_DEFINES_H
#include "defines.h"
#include "http-sanitize.h"
#include <regex.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>     // for close() function


#define PORT 1600
#define BACKLOG 1

#define HTTP_VERSION "HTTP/1.1"

#define IMG_ORIGINAL_DIR "/tmp/original/"

enum http_methods_t { HTTP_GET, HTTP_DELETE, HTTP_POST, HTTP_PUT };





#endif // !#ifndef SERVER_DEFINES_H
