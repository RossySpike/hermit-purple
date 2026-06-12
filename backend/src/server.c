#include "../includes/server.h"
#include "../includes/api-image.h"
#include "../includes/common-response.h"
#include "../includes/defines.h" // for fperror,  bzero macros.
#include "../includes/http-sanitize.h" // for __http_sanitize_key_value_t struct, validator functions.
#include "../includes/server-defines.h" // for  BUFFER, PORT, BACKLOG, macros; http_methods_t enum.
#include "../includes/server-routes.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h> // for open() function
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h> // for exit() function.
#include <strings.h>
#include <sys/socket.h>
#include <sys/socket.h> // for socket() function.
#include <sys/stat.h>   // for fstat() function
#include <sys/types.h>  // for types.
#include <unistd.h>
ssize_t seek_separator(const char *params) {
  for (size_t i = 0; params[i] != '\0'; i++) {
    if (params[i] == '&') {
      return i;
    }
  }
  return -1;
}

int init_server_addr(server_addr *saddr) {
  saddr->len = sizeof(saddr->addr);
  bzero(saddr, saddr->len);
  saddr->addr.sin_family = AF_INET;
  saddr->addr.sin_port = htons(PORT);
  saddr->addr.sin_addr.s_addr = htonl(INADDR_ANY);
  return 0;
}

int init_sock_server(server *s) {
  int optval = 1;
  s->server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (s->server_fd < 0) {
    fperror;
    return -1;
  }
  if (setsockopt(s->server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval,
                 sizeof(optval))) {
    close(s->server_fd);
    fperror;
    return -1;
  }
  if (bind(s->server_fd, (struct sockaddr *)&s->addr.addr, s->addr.len) < 0) {
    fperror;
    close(s->server_fd);
    return -1;
  }
  // epoll
  int flags = fcntl(s->server_fd, F_GETFL, 0);

  if (!flags) {
    return -1;
  }
  if (fcntl(s->server_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    return -1;

  if (listen(s->server_fd, BACKLOG) < 0) {
    fperror;
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
  s->client_fd =
      accept(s->server_fd, (struct sockaddr *)&s->addr.addr, &s->addr.len);
  if (s->client_fd == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 1;
    } else {
      return -1;
    }
  }
  int flags = fcntl(s->client_fd, F_GETFL, 0);

  if (!flags) {
    return -1;
  }
  if (fcntl(s->client_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    return -1;

  return 0;
}
int destroy_server(server *s) {
  if (s->client_fd >= 0) {
    close(s->client_fd);
    s->client_fd = -1;
  }
  if (s->server_fd >= 0) {
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
enum http_methods_t get_method(char *r, size_t *start_url) {
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

#define THIS_ROUTE (get_routes()[get_route_index(local_machine)])

/**
 * Finds the index of the server_routes_t with a matching uri_regex.
 *
 * Expects: url to be a null-terminated string.
 *
 * @param url The URL string to check.
 * @return The index of the matching server_routes_t, or -1 if not found.
 */
ssize_t check_url(const char *url) {
  const server_routes_t *local_routes = get_routes();

  for (size_t i = 0; local_routes[i].uri_regex != nullptr; i++) {
    printf("usr:\n%s\n======", url);
    printf("patron %s  \n", local_routes[i].uri_regex);
    int res = check_regex_with_regex(url, &local_routes[i].compiled_uri_regex);
    if (res == 1) {
      printf("hallada con patron %s ,local_routes[i]:%lu \n",
             local_routes[i].uri_regex, i);
      return i; // TODO: change
    } else if (res == -1) {
      perror("FALLO EL PATRON");
      exit(1);
    }
  }

  return -1;
}

// TODO: Check

#warning "bro I cant be this lazy lolllll"
#define read_buffer local_machine->server_ctx->read_buffer
#define cursor (*local_machine->server_ctx->cursor)
#define completed_headers local_machine->server_ctx->completed_headers
#define should_read local_machine->server_ctx->should_read

endpoint_return server_job(void *args) {
  server_machine *local_machine = (server_machine *)args;

  if (get_state(local_machine) == WAITING)
    set_state(local_machine, PROCESSING_REQUEST_LINE);
  char send_buffer[BUFFER] = {0};
  ssize_t n = 0; // number of bytes read
  /* bool stop_headers = false; */

  /* cursor.empty_mem = true;  // move to initializator */
  /* size_t completed_headers = 0; // I dont remember why I used this */
  /* bool should_read = true; */
  printf("should_read: %d\n", should_read);
  while (1) {

    if (should_read) {

      should_read = true;
      n = read(get_client_fd(local_machine), read_buffer, BUFFER - 1);
      if (n == -1) {

        if (errno == EAGAIN || errno == EWOULDBLOCK)
          return NEED_READ_MORE_DATA;
        else
          return SOMETHING_WENT_WRONG;
      }
    }
    switch (get_state(local_machine)) {

    // Frist case:
    case PROCESSING_REQUEST_LINE: {
      size_t i = 1;
      get_method(read_buffer, &i);
      ssize_t route_index = check_url(read_buffer);

      char *url = read_buffer + i;

      if (route_index == -1) {

        not_found(get_client_fd(local_machine), BUFFER, nullptr, nullptr);
        set_state(local_machine, ENDING);
        printf("lo mando a ending %d\n", 222);

        should_read = false;
        break;
      }

      int result = find_carriage(&cursor, read_buffer);
      // NOTE: previously `result < 0` and it worked
      if (result == -1 ||
          result == 1) { // In order to proceed we need result should be 0 or 2
                         // because the first line of the request shouldn't be
                         // bigger than BUFFER

        bad_request(get_client_fd(local_machine), BUFFER, nullptr, nullptr);
        set_state(local_machine, ENDING);
        printf("lo mando a ending %d\n", 237);
        should_read = false;
        break;
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
          ++cursor
                .offset; // find_carriage modifies the offset, and because its
                         // no longer needed to process that offset the current
                         // index is assined the next character from the offset
      set_route_index(
          local_machine,
          (size_t)route_index); // stored the `route_index` into `local_machine`

      // NOTE: previously `result < 1` and it worked
      if (result == 0) {

        set_state(local_machine, PROCESSING_HEADERS);
        // NOTE: previously `result < 2` and it worked
      } else if (result == 2) {

        set_state(local_machine, WORKING);
        should_read = false;
        break;
      }
      [[fallthrough]];
    }

    case PROCESSING_HEADERS: {
      // read_buffer[BUFFER - 1] Will always be \0
      // If he got here he had to be located at the first \r the one after HTTP
      // version
      // in http every line **should** end with crlf so before
      // incrementing the indexes Its required to check for \0

      if (cursor.curr == BUFFER - 1)
        continue;
      int result = find_carriage(&cursor, read_buffer);
      if (result == -1) { // header too large
        bad_request(get_client_fd(local_machine), BUFFER, nullptr,
                    "Header too large");
        set_state(local_machine, ENDING);
        printf("lo mando a ending %d\n", 305);
        should_read = false;
        break;
      }
      if (result == 2) {
        set_state(local_machine, ENDING); //
        printf("lo mando a ending %d\n", 311);
        should_read = false;
        break;
      }
      if (result == 1) {
        cursor.curr = 0;
        cursor.offset = 0;
        continue;
      }
      size_t i = 0;

      char last_char = '\0'; // it will stored the las seen crlf char
      size_t skips = 0; // will track the double CRLF, so lets say it founds
                        // "b\r\nH" would be skip=0->skips++->skips++->skips=0
      for (size_t *local_mem_index =
               cursor.empty_mem
                   ? &i
                   : &cursor.curr_mem; // `local_mem_index` will be used as the
                                       // `cursor.memory` index, if its empty
                                       // `local_mem_index` will use `i` else
                                       // will use `cursor.curr_mem`
           cursor.curr <
               (size_t)n && // n will never be 0 bc of while loop condition
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

            size_t i = 0;
            for (auto header = THIS_ROUTE.headers;
                 header && header->key != nullptr; i++, header++) {
              if ((completed_headers & ((size_t)1 << i)) !=
                  0) { // As we iterate over the `server_routes_t` headers if we
                       // find the same header the request in invalid

                continue;
                bad_request(get_client_fd(local_machine), BUFFER, nullptr,
                            "Bad headers");
                set_state(local_machine, ENDING);
                printf("lo mando a ending %d\n", 358);
                should_read = false;
                break;
              } // TODO: refactor

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
                completed_headers |= ((size_t)1 << i);
                if (should_add) {

                  char *temp = malloc(strlen(cursor.memory) + 1);
                  strcpy(temp, cursor.memory);
                  add_header(local_machine, temp);
                }
              } else {
              }
            }
            if (get_state(local_machine) == ENDING)
              break;
          }

          last_char = '\r';     // every header ends with crlf.
          skips++;              // every header ends with crlf.
          *local_mem_index = 0; // reset the local memory index to 0 to start
                                // storing the next header

          bzero(cursor.memory, BUFFER);
          continue;

        } else if (read_buffer[cursor.curr] == '\n') {
          last_char = '\n';
          skips++;
          continue;
        } else {
          // It didnt found \r or \n so we arent in the end of the header
          skips = 0;
          last_char = '\0';
        }
        cursor.memory[(*local_mem_index)++] =
            read_buffer[cursor.curr]; // this is to store the current header
                                      // into `cursor.memory` while incrementing
                                      // its index
      }
      if (cursor.curr == (size_t)n) { // end of the stream

        cursor.curr = 0;
        cursor.offset = 0;
      }
      if (last_char == '\n' &&
          skips == 4) { // double CRLF found, end of headers

        size_t k = 0;
        for (auto header = THIS_ROUTE.headers; header && header->key != nullptr;
             k++, header++) {
          if ((completed_headers & ((size_t)1 << k)) != 1 && header->required) {
            bad_request(get_client_fd(local_machine), BUFFER, nullptr,
                        "Bad headers");
            set_state(local_machine, ENDING);
            printf("lo mando a ending %d\n", 455);
            should_read = false;
            break;
          }
        }
        if (get_state(local_machine) == ENDING)
          break;
        if (n < BUFFER - 1 && (size_t)n == cursor.curr &&
            get_http_method(get_route_index(local_machine)) < HTTP_POST) {
          bad_request(get_client_fd(local_machine), BUFFER, nullptr, nullptr);
          set_state(local_machine, ENDING);

          printf("lo mando a ending %d\n", 467);
          should_read = false;
          break;
        }
        bzero(cursor.memory, BUFFER);
        set_state(local_machine, WORKING);
        if (cursor.curr == 0 &&
            (get_http_method(get_route_index(local_machine)) == HTTP_POST ||
             get_http_method(get_route_index(local_machine)) == HTTP_PUT)) {
          break;
        } else {

          cursor.curr_mem = 0;
          cursor.empty_mem = true;
          cursor.offset = cursor.curr;
        }

      } else {
        should_read = true;
        break; // keep processing headers
      }
      [[fallthrough]];
    }
    case WORKING: {

      assert(get_state(local_machine) == WORKING);
      endpoint_return ret = api_jump_table(&cursor, local_machine, read_buffer);
      switch (ret) {
      case SOMETHING_WENT_WRONG:
      case FINISHED:

        printf("lo mando a ending %d\n", 499);

        break;

      case NEED_READ_MORE_DATA:
      case NEED_WRITE_MORE_DATA:
        return ret;
      default:
        unreachable();
      }
      set_state(local_machine, WAITING);
      printf("lo mando a ending %d\n", 510);
      should_read = false;
      printf("ret: %d\n", ret);
      return ret;
    }
    /* case ENDING: { */
    /*   close(get_client_fd(local_machine)); */
    /*   set_state(local_machine, WAITING); */
    /*   printf("case ENDING\n"); */
    /*   return FINISHED; */
    /*   break; */
    /* } */
    default:
      unreachable();
    }
    if (get_state(local_machine) != WORKING) {

      bzero(read_buffer, BUFFER);
    }
    bzero(send_buffer, BUFFER);
  }

  printf("llego al final\n");
  return FINISHED;
}
