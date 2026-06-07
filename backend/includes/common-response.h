#ifndef COMMON_RESPONSE_H
#define COMMON_RESPONSE_H
#include "./server-defines.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int bad_request(int client_fd, unsigned long long buffer, const char *headers,
                const char *body) {
  if (client_fd <= STDERR_FILENO)
    return -1;
  unsigned long long content_length = 0;
  if (body != nullptr)
    content_length = strlen(body);
  else
    content_length = strlen("Bad Request");

  char *b = (char *)calloc(buffer, sizeof(char));
  assert(b);

  int msg_res = snprintf(
      b, buffer, "HTTP/1.1 400 Bad Request\r\n%s: %s\r\n%s: %llu%s\r\n\r\n%s",
      "Server", HOST_NAME, "Content-Length", content_length,
      headers == nullptr ? "" : headers, body != nullptr ? body : "Bad Request");
  if (msg_res > 0) {

    assert((unsigned long long)msg_res < buffer);
    assert(write(client_fd, b, buffer));
  } else {
    printf("DEBUG: %s remember to handle all values of `snprintf` bc it failed "
           "with: %d\n",
           __func__, msg_res);
  }
  free(b);
  return 0;
}

int internal_server_error(int client_fd, unsigned long long buffer,
                          const char *headers, const char *body) {
  if (client_fd <= STDERR_FILENO)
    return -1;
  unsigned long long content_length = 0;
  if (body != nullptr)
    content_length = strlen(body);
  else
    content_length = strlen("Something went wrong");

  char *b = (char *)calloc(buffer, sizeof(char));
  assert(b);

  int msg_res = snprintf(b, buffer,
                         "HTTP/1.1 500 Internal Server Error\r\n%s: %s\r\n%s: "
                         "%llu%s\r\n\r\n%s",
                         "Server", HOST_NAME, "Content-Length", content_length,
                         headers == nullptr ? "" : headers,
                         body != nullptr ? body : "Something went wrong");
  if (msg_res > 0) {

    assert((unsigned long long)msg_res < buffer);
    assert(write(client_fd, b, buffer));
  } else {
    printf("DEBUG: %s remember to handle all values of `snprintf` bc it failed "
           "with: %d\n",
           __func__, msg_res);
  }
  free(b);
  return 0;
}
int not_found(int client_fd, unsigned long long buffer, const char *headers,
              const char *body) {
  if (client_fd <= STDERR_FILENO)
    return -1;
  unsigned long long content_length = 0;
  if (body != nullptr)
    content_length = strlen(body);
  else
    content_length = strlen("Not Found");

  char *b = (char *)calloc(buffer, sizeof(char));
  assert(b);

  int msg_res = snprintf(
      b, buffer, "HTTP/1.1 404 Not Found\r\n%s: %s\r\n%s: %llu%s\r\n\r\n%s",
      "Server", HOST_NAME, "Content-Length", content_length,
      headers == nullptr ? "" : headers, body != nullptr ? body : "Not Found");
  if (msg_res > 0) {

    assert((unsigned long long)msg_res < buffer);
    assert(write(client_fd, b, buffer));
  } else {
    printf("DEBUG: %s remember to handle all values of `snprintf` bc it failed "
           "with: %d\n",
           __func__, msg_res);
  }
  free(b);
  return 0;
}
int created(int client_fd, unsigned long long buffer, const char *headers,
            const char *body) {
  if (client_fd <= STDERR_FILENO)
    return -1;
  unsigned long long content_length = 0;
  if (body != nullptr)
    content_length = strlen(body);
  else
    content_length = strlen("Created");

  char *b = (char *)calloc(buffer, sizeof(char));
  assert(b);

  int msg_res = snprintf(
      b, buffer, "HTTP/1.1 201 Created\r\n%s: %s\r\n%s: %llu%s\r\n\r\n%s",
      "Server", HOST_NAME, "Content-Length", content_length,
      headers == nullptr ? "" : headers, body != nullptr ? body : "Created");
  if (msg_res > 0) {

    assert((unsigned long long)msg_res < buffer);
    assert(write(client_fd, b, buffer));
  } else {
    printf("DEBUG: %s remember to handle all values of `snprintf` bc it failed "
           "with: %d\n",
           __func__, msg_res);
  }
  // printf("%s\n", b);
  free(b);
  return 0;
}

#endif // COMMON_RESPONSE_H
