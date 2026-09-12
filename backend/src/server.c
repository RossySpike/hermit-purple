#define _GNU_SOURCE
#include "../includes/server.h"
#include "../includes/api-image.h"
#include "../includes/common-response.h"
#include "../includes/defines.h" // for fperror,  bzero macros.
#include "../includes/http-sanitize.h" // for __http_sanitize_key_value_t struct, validator functions.
#include "../includes/server-defines.h" // for  BUFFER, PORT, BACKLOG, macros; http_methods_t enum.
#include "../includes/server-routes.h"
#include "logger.h"
#include "server-machine.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h> // for open() function
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h> // for exit() function.
#include <strings.h>
#include <sys/socket.h> // for socket() function.
#include <sys/stat.h>   // for fstat() function
#include <sys/time.h>
#include <sys/types.h> // for types.
#include <unistd.h>
#define is_header_at_i_parsed(i) (completed_headers & ((size_t)1 << (i))) != 0
#define header_at_i_is_parsed(i) completed_headers |= ((size_t)1 << (i))
// NOTHING, RETURN, BREAK, CONTINUE
#define handle_action(action)                                                  \
  if ((action).type == CONTINUE) {                                             \
    continue;                                                                  \
  }                                                                            \
  if ((action).type == BREAK) {                                                \
    break;                                                                     \
  }                                                                            \
  if ((action).type == RETURN) {                                               \
    return (action).return_status;                                             \
  }

static ssize_t seek_separator(const char *params);
int init_server_addr(server_addr *saddr) {
  saddr->len = sizeof(saddr->addr);
  memset(saddr, 0, saddr->len);
  saddr->addr.sin_family = AF_INET;
  saddr->addr.sin_port = htons(PORT);
  saddr->addr.sin_addr.s_addr = htonl(INADDR_ANY);
  return 0;
}

int init_sock_server(server *s) {
  int optval = 1;
  s->server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (s->server_fd < 0) {
    fperror;
    return -1;
  }
  if (setsockopt(s->server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval,
                 sizeof(optval))) {
    logger_log(LOG_HIGH | LOG_MEDIUM, "%s:%d: closing `s->server_fd` (%d)\n",
               __func__, __LINE__, s->server_fd);
    close(s->server_fd);
    fperror;
    return -1;
  }
  if (bind(s->server_fd, (struct sockaddr *)&s->addr.addr, s->addr.len) < 0) {
    fperror;
    logger_log(LOG_HIGH | LOG_MEDIUM, "%s:%d: closing `s->server_fd` (%d)\n",
               __func__, __LINE__, s->server_fd);
    close(s->server_fd);
    return -1;
  }
  // epoll
  // int flags = fcntl(s->server_fd, F_GETFL, 0);

  /* if (!flags) { */
  /*   return -1; */
  /* } */
  /* if (fcntl(s->server_fd, F_SETFL, flags | O_NONBLOCK) == -1) */
  /*   return -1; */

  if (listen(s->server_fd, BACKLOG) < 0) {
    fperror;
    logger_log(LOG_HIGH | LOG_MEDIUM, "%s:%d: closing `s->server_fd` (%d)\n",
               __func__, __LINE__, s->server_fd);
    close(s->server_fd);
    return -1;
  }
  return 0;
}
// 3
int init_server(server *s) {
  if (init_server_addr(&s->addr) < 0) {
    return -1;
  }
  if (init_sock_server(s) < 0) {
    return -1;
  }
  return 0;
}
int init_client(server *s) {
  s->client_fd = accept4(s->server_fd, (struct sockaddr *)&s->addr.addr,
                         &s->addr.len, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (s->client_fd == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 1;
    }
    return -1;
  }
  /* int flags = fcntl(s->client_fd, F_GETFL, 0); */
  /**/
  /* if (!flags) { */
  /*   return -1; */
  /* } */
  /* if (fcntl(s->client_fd, F_SETFL, flags | O_NONBLOCK) == -1) { */
  /*   return -1; */
  /* } */

  return 0;
}
int destroy_server(server *s) {
  if (s->client_fd >= 0) {
    logger_log(LOG_HIGH | LOG_MEDIUM, "%s:%d: closing `s->server_fd` (%d)\n",
               __func__, __LINE__, s->server_fd);
    close(s->client_fd);
    s->client_fd = -1;
  }
  if (s->server_fd >= 0) {
    logger_log(LOG_HIGH | LOG_MEDIUM, "%s:%d: closing `s->server_fd` (%d)\n",
               __func__, __LINE__, s->server_fd);
    close(s->server_fd);
    s->client_fd = -1;
  }
  return 0;
}

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
static enum http_methods_t get_method(char *r, size_t *start_url);

#define THIS_ROUTE (get_routes()[get_route_index(local_machine)])

/**
 * Finds the index of the server_routes_t with a matching uri_regex.
 *
 * Expects: url to be a null-terminated string.
 *
 * @param url The URL string to check.
 * @return The index of the matching server_routes_t, or -1 if not found.
 */
static ssize_t check_url(const char *url);

// TODO: Check

#warning "bro I cant be this lazy lolllll"
#define read_buffer local_machine->server_ctx->read_buffer
#define cursor (*local_machine->server_ctx->cursor)
#define completed_headers local_machine->server_ctx->completed_headers
#define should_read local_machine->server_ctx->should_read

typedef enum { NOTHING, RETURN, BREAK, CONTINUE } action_type_t;
typedef struct {
  endpoint_return return_status;
  action_type_t type;
} action_t;

static action_t process_request_line(server_machine *local_machine,
                                     char *send_buffer);
static action_t process_headers(server_machine *local_machine,
                                char *send_buffer);

static action_t process_work(server_machine *local_machine, char *send_buffer);
endpoint_return server_job(void *args) {
  server_machine *local_machine = (server_machine *)args;

  if (get_state(local_machine) == WAITING) {
    set_state(local_machine, PROCESSING_REQUEST_LINE);
  }
  char send_buffer[BUFFER] = {0};
  local_machine->server_ctx->n = 0; // number of bytes read
  /* bool stop_headers = false; */

  /* cursor.empty_mem = true;  // move to initializator */
  /* size_t completed_headers = 0; // I dont remember why I used this */
  /* bool should_read = true; */
  while (1) {

    if (should_read) {

      /* should_read = true; */
      /* struct timeval tv; */
      /* tv.tv_sec = 30; // 30 segundos timeout */
      /* tv.tv_usec = 0; */
      /* setsockopt(get_client_fd(local_machine), SOL_SOCKET, SO_RCVTIMEO, &tv,
       */
      /*            sizeof(tv)); */

      local_machine->server_ctx->n =
          read(get_client_fd(local_machine), read_buffer, BUFFER - 1);
      if (local_machine->server_ctx->n == 0) { // Client close conn
        return SOMETHING_WENT_WRONG;
      }
      if (local_machine->server_ctx->n == -1) {

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          return NEED_READ_MORE_DATA;
        }
        return SOMETHING_WENT_WRONG;
      }
    }
    switch (get_state(local_machine)) {

    // Frist case:
    case PROCESSING_REQUEST_LINE: {
      logger_log(LOG_HIGH | LOG_MEDIUM, read_buffer);
      action_t result = process_request_line(local_machine, send_buffer);
      handle_action(result);
      [[fallthrough]];
    }

    case PROCESSING_HEADERS: {

      action_t result = process_headers(local_machine, send_buffer);
      handle_action(result);

      [[fallthrough]];
    }
    case WORKING: {
      action_t result = process_work(local_machine, send_buffer);
      handle_action(result);
    }
    default:
      unreachable();
    }
    if (get_state(local_machine) == ENDING) {

      break;
    }
    if (get_state(local_machine) != WORKING) {

      memset(read_buffer, 0, (size_t)BUFFER);
    } else {
      break;
    }
    memset(send_buffer, 0, (size_t)BUFFER);
  }

  return FINISHED;
}
static ssize_t seek_separator(const char *params) {
  for (size_t i = 0; params[i] != '\0'; i++) {
    if (params[i] == '&') {
      return i;
    }
  }
  return -1;
}
static enum http_methods_t get_method(char *r, size_t *start_url) {
  char *first_space = strchr(r, ' ');
  if (first_space == nullptr) {
    return -1; // No space found, invalid request
  }
  first_space++;
  int len = first_space - r;

  if (start_url != nullptr)
    *start_url = len; // Set `start_url` to the position after the method
  char method[len];
  method[len - 1] = '\0'; // ensure null-terminated
  for (int i = 0; i < len - 1; i++) {
    method[i] = r[i]; // copying only the method from `r` into `method`
  }
  if (strcmp(method, "GET") == 0) {
    return HTTP_GET; // GET method
  } else if (strcmp(method, "POST") == 0) {
    return HTTP_POST; // POST method
  } else if (strcmp(method, "PUT") == 0) {
    return HTTP_PUT; // PUT method
  } else if (strcmp(method, "DELETE") == 0) {
    return HTTP_DELETE; // DELETE method
  } else if (strcmp(method, "OPTIONS") == 0) {
    return HTTP_OPTIONS; // OPTIONS method
  } else {
    return -1; // Unknown method
  }
}
static ssize_t check_url(const char *url) {
  const server_routes_t *local_routes = get_routes();

  for (size_t i = 0; local_routes[i].uri_regex != nullptr; i++) {
    int res = check_regex_with_regex(url, &local_routes[i].compiled_uri_regex);
    if (res == 1) {
      return i; // TODO: change
    } else if (res == -1) {
      exit(1);
    }
  }

  return -1;
}
static action_t process_request_line(server_machine *local_machine,
                                     char *send_buffer) {
  action_t ret_val = {0};
  size_t i = 1;
  get_method(read_buffer, &i);
  ssize_t route_index = check_url(read_buffer);

  char *url = read_buffer + i;

  if (route_index == -1) {

    not_found(get_client_fd(local_machine), (uint64_t)BUFFER, nullptr, nullptr);
    set_state(local_machine, ENDING);

    should_read = false;
    ret_val.type = RETURN;
    ret_val.return_status = SOMETHING_WENT_WRONG;
    return ret_val;
  }

  carriage_status result = find_carriage(&cursor, read_buffer);
  // NOTE: previously `result < 0` and it worked
  if (result == CARRIAGE_NOT_FOUND_MEM_NOT_EMPTY ||
      result == CARRIAGE_NOT_FOUND_MEM_PUSHED) { // In order to proceed we need
                                                 // result should be 0 or 2
    // because the first line of the request shouldn't be
    // bigger than BUFFER

    bad_request(get_client_fd(local_machine), BUFFER, nullptr, nullptr);
    set_state(local_machine, ENDING);
    should_read = false;
    ret_val.type = BREAK;
    return ret_val;
  }
  size_t j = 0;
  for (; url[j] != '\0' && url[j] != '?' && url[j] != ' '; j++) {
  }
  if (url[j] == '?') {

    char *param = url + j + 1;
    ssize_t idx = 0;
    for (; (idx = seek_separator(param)) != -1;) {
      char *p = param;
      p[idx] = '\0';
      const char *to_add = strdup(p);
      add_params(local_machine, to_add);
      param += idx + 1;
    }
    for (size_t i = 0; param[i] != '\0'; i++) {
      if (param[i] == ' ') {
        idx = i;
        break;
      }
    }
    char *p = param;
    p[idx] = '\0';
    const char *to_add = strdup(p); // '\0'
    add_params(local_machine, to_add);
  }

  // in order to have it when processing the request
  cursor.curr =
      ++cursor.offset; // find_carriage modifies the offset, and because its
                       // no longer needed to process that offset the current
                       // index is assined the next character from the offset
  set_route_index(
      local_machine,
      (size_t)route_index); // stored the `route_index` into `local_machine`

  // NOTE: previously `result < 1` and it worked
  if (CARRIAGE_FOUND == 0) {

    set_state(local_machine, PROCESSING_HEADERS);
    // NOTE: previously `result < 2` and it worked
  } else if (result == CARRIAGE_DOUBLE) {

    set_state(local_machine, WORKING);
    should_read = false;
    ret_val.type = BREAK;
    return ret_val;
  }
}
static action_t process_headers(server_machine *local_machine,
                                char *send_buffer) {
  action_t ret_val = {0};
  // read_buffer[BUFFER - 1] Will always be \0
  // If he got here he had to be located at the first \r the one after HTTP
  // version
  // in http every line **should** end with crlf so before
  // incrementing the indexes Its required to check for \0

  if (cursor.curr == BUFFER - 1) {
    ret_val.type = CONTINUE;
    return ret_val;
  }
  carriage_status result = find_carriage(&cursor, read_buffer);
  if (result == CARRIAGE_NOT_FOUND_MEM_NOT_EMPTY) { // header too large
    bad_request(get_client_fd(local_machine), BUFFER, nullptr,
                "Header too large");
    set_state(local_machine, ENDING);
    should_read = false;
    ret_val.type = BREAK;
    return ret_val;
  }
  if (result == CARRIAGE_DOUBLE) {
    set_state(local_machine, ENDING); //
    should_read = false;
    ret_val.type = BREAK;
    return ret_val;
  }
  if (result == CARRIAGE_NOT_FOUND_MEM_PUSHED) {
    cursor.curr = 0;
    cursor.offset = 0;
    ret_val.type = CONTINUE;
    return ret_val;
  }
  size_t i = 0;

  char last_char = '\0'; // it will stored the las seen crlf char
  size_t skips = 0;      // will track the double CRLF, so lets say it founds
                         // "b\r\nH" would be skip=0->skips++->skips++->skips=0
  for (size_t *local_mem_index =
           cursor.empty_mem
               ? &i
               : &cursor.curr_mem; // `local_mem_index` will be used as the
                                   // `cursor.memory` index, if its empty
                                   // `local_mem_index` will use `i` else
                                   // will use `cursor.curr_mem`
       cursor.curr < (size_t)local_machine->server_ctx
                         ->n && // n will never be 0 bc of while loop condition
       (last_char != '\n' ||
        skips != 4); // cursor.curr ist the last char of the string and
                     // havent found double CRLF
       cursor.curr++) {
    if (read_buffer[cursor.curr] ==
        '\r') { // As the for loop goes `cursor.curr` increments when whe
                // finally hit a \r it means that we have found the end of a
                // header
      // TODO: process header;
      if (cursor.memory[0] !=
          '\0') { // We are in the end of a header(previous if) and the
                  // cursor memory isnt empty so it has a header

        size_t headerIdx = 0;
        for (auto header = THIS_ROUTE.headers; header && header->key != nullptr;
             headerIdx++, header++) {
          /* if ((completed_headers & ((size_t)1 << i)) != 0) { */
          /* if (is_header_at_i_parsed(i)) { */
          /*   // As we iterate over the */
          /*   // `server_routes_t` headers if we */
          /*   // find the same header the request in invalid */
          /**/
          /*   bad_request(get_client_fd(local_machine), BUFFER, nullptr, */
          /*               "Bad headers"); */
          /*   set_state(local_machine, ENDING); */
          /*   should_read = false; */
          /*   break; */
          /* }  */
          // TODO: refactor

          bool good_header = true;

          bool should_add = true; // ifts not required then we dont add it
          if (header->validators != nullptr) {
            // evaluates every validator, ( header[i] )->validators[j]
            //
            for (size_t j = 0;
                 header->validators[j] != nullptr && good_header != false;
                 good_header = header->validators[j](
                     *header, // header must be dereferenced
                     cursor.memory),
                        j++) {
              if (!good_header && !header->required) {
                // this will matter only in the first
                // iteration of the loop, as its
                // required to add a header_key to be
                // the first validator, so if the
                // current header doesnt match the the
                // header key its set as good_header,
                // so if the header ends up being later
                // in the data stream then it will
                // obviusly match
                // `validator_header_key` and wont fall
                // inside the if statement and will be
                // properly validated, but if the
                // header is missing and its not
                // required then it will be set as
                // good_header and shouldnt be added to
                // the headers list of the
                // local_machine, so `should_add` is
                // set to false to avoid adding it
                // later
                good_header = true;
                should_add = false;
                break;
              }
            }
          } else {
          }

          if (good_header) {
            /* header_at_i_is_parsed(i); */
            if ((completed_headers & ((size_t)1 << headerIdx)) != 0) {

              bad_request(get_client_fd(local_machine), (uint64_t)BUFFER,
                          nullptr, "Bad headers");
              set_state(local_machine, ENDING);
              should_read = false;
              break;
            }
            completed_headers |= ((size_t)1 << headerIdx);
            if (should_add) {

              char *temp = calloc(1, strlen(cursor.memory) + 1);
              strcpy(temp, cursor.memory);
              add_header(local_machine, temp);
            }
          } else {
          }
        }
        if (get_state(local_machine) == ENDING) {
          break;
        }
      }

      last_char = '\r';     // every header ends with crlf.
      skips++;              // every header ends with crlf.
      *local_mem_index = 0; // reset the local memory index to 0 to start
                            // storing the next header

      memset(cursor.memory, 0, (size_t)BUFFER);
      continue;
    }
    if (read_buffer[cursor.curr] == '\n') {
      last_char = '\n';
      skips++;
      continue;
    }
    // It didnt found \r or \n so we arent in the end of the header
    skips = 0;
    last_char = '\0';
    cursor.memory[(*local_mem_index)++] =
        read_buffer[cursor.curr]; // this is to store the current header
                                  // into `cursor.memory` while incrementing
                                  // its index
  }
  if (cursor.curr ==
      (size_t)local_machine->server_ctx->n) { // end of the stream

    cursor.curr = 0;
    cursor.offset = 0;
  }
  if (last_char == '\n' && skips == 4) { // double CRLF found, end of headers

    size_t k = 0;
    for (auto header = THIS_ROUTE.headers; header && header->key != nullptr;
         k++, header++) {
      if ((completed_headers & ((size_t)1 << k)) != 1 && header->required) {
        /* if (!(is_header_at_i_parsed(k)) && header->required) { */
        bad_request(get_client_fd(local_machine), (uint64_t)BUFFER, nullptr,
                    "Bad headers");
        set_state(local_machine, ENDING);
        should_read = false;
        break;
      }
    }
    if (get_state(local_machine) == ENDING) {
      ret_val.type = BREAK;
      return ret_val;
    }
    if (local_machine->server_ctx->n < BUFFER - 1 &&
        (size_t)local_machine->server_ctx->n == cursor.curr &&
        get_http_method(get_route_index(local_machine)) < HTTP_POST) {
      bad_request(get_client_fd(local_machine), BUFFER, nullptr, nullptr);
      set_state(local_machine, ENDING);

      should_read = false;
      ret_val.type = BREAK;
      return ret_val;
    }
    memset(cursor.memory, 0, (size_t)BUFFER);
    set_state(local_machine, WORKING);
    if (cursor.curr == 0 &&
        (get_http_method(get_route_index(local_machine)) == HTTP_POST ||
         get_http_method(get_route_index(local_machine)) == HTTP_PUT)) {
      bad_request(get_client_fd(local_machine), (uint64_t)BUFFER, nullptr,
                  "PUT and POST methods require a body");
      ret_val.type = BREAK;
      return ret_val;
    }

    cursor.curr_mem = 0;
    cursor.empty_mem = true;
    cursor.offset = cursor.curr;

  } else {
    should_read = true;

    ret_val.type = BREAK;
    return ret_val;
    // keep processing headers
  }
}
static action_t process_work(server_machine *local_machine, char *send_buffer) {
  action_t ret_val = {0};
  should_read = false;

  assert(get_state(local_machine) == WORKING);
  endpoint_return ret = api_jump_table(&cursor, local_machine, read_buffer);
  ret_val.return_status = ret;
  switch (ret) {
  case SOMETHING_WENT_WRONG:
  case FINISHED:

    ret_val.type = BREAK;
    return ret_val;

  case NEED_READ_MORE_DATA:
  case NEED_WRITE_MORE_DATA:
    ret_val.type = RETURN;

    return ret_val;
  default:
    unreachable();
  }
  set_state(local_machine, WAITING);
  ret_val.type = RETURN;

  return ret_val;
}
