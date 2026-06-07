#include "../includes/common-response.h"
#include "../includes/defines.h" // for fperror,  bzero macros.
#include "../includes/http-sanitize.h" // for __http_sanitize_key_value_t struct, validator functions.
#include "../includes/server-defines.h" // for  BUFFER, PORT, BACKLOG, macros; http_methods_t enum.
#include "../src/api-image.c"
#include "../src/server-routes.c"
#include <asm-generic/socket.h>
#include <fcntl.h>      // for open() function
#include <netinet/in.h> // for sockaddr_in structure.
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h> // for exit() function.
#include <strings.h>
#include <sys/socket.h> // for socket() function.
#include <sys/stat.h>   // for fstat() function
#include <sys/types.h>  // for types.

ssize_t seek_separator(const char *params) {
  for (size_t i = 0; params[i] != '\0'; i++) {
    if (params[i] == '&' || params[i] == ' ') {
      return i;
    }
  }
  return -1;
}
typedef struct SERVER_ADDR {
  struct sockaddr_in addr;
  socklen_t len;
} server_addr;

typedef struct SERVER {
  int server_fd;
  int client_fd;
  server_addr addr;
} server;

int init_server_addr(server_addr *saddr) {
  printf("DEBUG: %s\n", __func__);
  saddr->len = sizeof(saddr->addr);
  bzero(saddr, saddr->len);
  saddr->addr.sin_family = AF_INET;
  saddr->addr.sin_port = htons(PORT);
  saddr->addr.sin_addr.s_addr = htonl(INADDR_ANY);
  return 0;
}

int init_sock_server(server *s) {
  printf("DEBUG: %s\n", __func__);
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
  if (listen(s->server_fd, BACKLOG) < 0) {
    fperror;
    close(s->server_fd);
    return -1;
  }

  return 0;
}
// 3
int init_server(server *s) {
  printf("DEBUG: %s\n", __func__);
  printf("Initialazing server state...\n");
  if (init_server_addr(&s->addr) < 0) {
    return -1;
  }
  if (init_sock_server(s) < 0) {
    return -1;
  }
  printf("Server initialized successfully.\n");
  return 0;
}
int init_client(server *s) {
  printf("DEBUG: %s\n", __func__);
  printf("Waiting for client connection on PORT:%d...\n", PORT);
  s->client_fd =
      accept(s->server_fd, (struct sockaddr *)&s->addr.addr, &s->addr.len);
  if (s->client_fd < 0) {
    fperror;
    close(s->server_fd);
    return -1;
  }
  printf("Client connected successfully.\n");
  return 0;
}
int destroy_server(server *s) {
  printf("DEBUG: %s\n", __func__);
  if (s->client_fd >= 0) {
    close(s->client_fd);
    s->client_fd = -1;
  }
  if (s->server_fd >= 0) {
    close(s->server_fd);
    s->client_fd = -1;
  }
  printf("Server destroyed successfully.\n");
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
  printf("DEBUG: %s\n", __func__);
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
  } else {
    return -1; // Unknown method
  }
}

#define THIS_ROUTE (routes[get_route_index(local_machine)])

/**
 * Finds the index of the server_routes_t with a matching uri_regex.
 *
 * Expects: url to be a null-terminated string.
 *
 * @param url The URL string to check.
 * @return The index of the matching server_routes_t, or -1 if not found.
 */
ssize_t check_url(const char *url) {
  printf("DEBUG: %s\n", __func__);
  const server_routes_t *local_routes = routes;

  for (size_t i = 0; local_routes[i].uri_regex != nullptr; i++) {
    printf("Patron: %s \n", local_routes[i].uri_regex);
    int res = check_regex(url, local_routes[i].uri_regex);
    if (res == 1) {
      printf("hallada con patron %s\n", local_routes[i].uri_regex);
      return i; // TODO: change
    } else if (res == -1) {
      perror("FALLO EL PATRON");
      exit(1);
    }
  }
  printf("no hallada\n");

  return -1;
}

// TODO: Check

void *server_job(void *args) {
  printf("DEBUG: %s\n", __func__);
  server_machine *local_machine = (server_machine *)args;

  char send_buffer[BUFFER] = {0};
  char read_buffer[BUFFER] = {0};
  set_state(local_machine, PROCESSING_REQUEST_LINE);
  ssize_t n = 0; // number of bytes read
  /* bool stop_headers = false; */
  stream_cursor cursor = {
      0}; // has an index to the current character of the stream and a buffer to
          // store old values from the stream that need to be used

  cursor.empty_mem = true;
  size_t completed_headers = 0; // I dont remember why I used this
  printf("llegue\n");
  bool should_read = true;
  while (!should_read || ((n = read(get_client_fd(local_machine), read_buffer,
                                    BUFFER - 1)) > 0)) {
    printf("=========\n%s=========\nnBytes: %li\n=========\n", read_buffer, n);

    should_read = true;
    switch (get_state(local_machine)) {

    // Frist case:
    case PROCESSING_REQUEST_LINE: {
      size_t i = 1;
      get_method(read_buffer, &i);
      ssize_t route_index = check_url(read_buffer);

      char *url = read_buffer + i;

      if (route_index == -1) {
        /* snprintf(send_buffer, BUFFER, "HTTP/1.1 404 Not Found\r\n\r\n"); */
        /* write(get_client_fd(local_machine), send_buffer, */
        /*       strlen(send_buffer)); // TODO: add error handling */

        not_found(get_client_fd(local_machine), BUFFER, nullptr, nullptr);
        set_state(local_machine, ENDING);

        should_read = false;
        break;
      }

      int result = find_carriage(&cursor, read_buffer);
      // NOTE: previously `result < 0` and it worked
      if (result == -1 ||
          result == 1) { // In order to proceed we need result should be 0 or 2
                         // because the first line of the request shouldn't be
                         // bigger than BUFFER
        /* snprintf(send_buffer, BUFFER, "HTTP/1.1 400 Bad Request\r\n\r\n"); */
        /* write(get_client_fd(local_machine), send_buffer, */
        /*       strlen(send_buffer)); // TODO: add error handling */

        bad_request(get_client_fd(local_machine), BUFFER, nullptr, nullptr);
        set_state(local_machine, ENDING);
        should_read = false;
        break;
      }
      size_t j = 0;
      for (; url[j] != '\0' && url[j] != '?' && url[j] != ' '; j++) {
      }
      if (url[j] == '?') {

        char *param = url + j + 1;
        param++;
        ssize_t idx = 0;
        for (; (idx = seek_separator(param)) != -1;) {
          char *p = param;
          p[idx] = '\0';
          add_params(local_machine, p);
          param += idx + 1;
        }
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
        /* snprintf(send_buffer, BUFFER, */
        /*          "HTTP/1.1 400 Bad Request\r\n\r\nHeader too large"); */
        /* write(get_client_fd(local_machine), send_buffer, */
        /*       strlen(send_buffer)); // TODO: add error handling */
        bad_request(get_client_fd(local_machine), BUFFER, nullptr,
                    "Header too large");
        set_state(local_machine, ENDING);
        should_read = false;
        break;
      }
      if (result == 2) {
        set_state(local_machine, ENDING); //
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

            printf("[+] Header=%s\n", cursor.memory);
            size_t i = 0;
            for (auto header = THIS_ROUTE.headers; header->key != nullptr;
                 i++, header++) {
              printf("DEBUG: completed_headers: %lu\n", completed_headers);
              printf("DEBUG: completed_headers & ((size_t)1 << i -> %lu & "
                     "((size_t)1 << %lu = %lu \n",
                     completed_headers, i,
                     completed_headers & ((size_t)1 << i));
              if ((completed_headers & ((size_t)1 << i)) !=
                  0) { // As we iterate over the `server_routes_t` headers if we
                       // find the same header the request in invalid

                continue;
                printf("DEBUG: found duplicated header\n");
                printf("[+] DEBUG Header %s failed\n", cursor.memory);
                /* snprintf(send_buffer, BUFFER, */
                /*          "HTTP/1.1 400 Bad Request\r\n\r\nBad headers"); */
                /* write(get_client_fd(local_machine), send_buffer, */
                /*       strlen(send_buffer)); // TODO: add error handling */
                bad_request(get_client_fd(local_machine), BUFFER, nullptr,
                            "Bad headers");
                set_state(local_machine, ENDING);
                should_read = false;
                break;
              } // TODO: refactor

              bool good_header = true;
              printf("DEBUG: Processing header definition: %s\n", header->key);

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
                  printf("DEBUG: good_header[j:%lu]=%d\n", j, good_header);
                }
              } else {
                printf("DEBUG: validators is nullptr!\n");
              }

              if (good_header) {
                completed_headers |= ((size_t)1 << i);
                printf("DEBUG: SETTING BIT AS MARKED\n");
                if (should_add) {
                  printf("DEBUG: ABOUT TO ADD HEADER: '%s'\n", cursor.memory);

                  char *temp = malloc(strlen(cursor.memory) + 1);
                  strcpy(temp, cursor.memory);
                  add_header(local_machine, temp);
                  printf("DEBUG: HEADER ADDED. Headers count: %zu\n",
                         local_machine->headers.len);
                }
              } else {

                printf("[+] DEBUG Header: %s failed\n", cursor.memory);
              }
              printf("DEBUG: completed_headers: %lu\n", completed_headers);
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
        for (auto header = THIS_ROUTE.headers; header->key != nullptr;
             k++, header++) {
          printf("DEBUG: validating all headers required were given\n");
          if ((completed_headers & ((size_t)1 << k)) != 1 && header->required) {
            /* snprintf(send_buffer, BUFFER, */
            /*          "HTTP/1.1 400 Bad Request\r\n\r\nBad headers"); */
            /* write(get_client_fd(local_machine), send_buffer, */
            /*       strlen(send_buffer)); // TODO: add error handling */
            bad_request(get_client_fd(local_machine), BUFFER, nullptr,
                        "Bad headers");
            set_state(local_machine, ENDING);
            should_read = false;
            break;
          }
        }
        if (get_state(local_machine) == ENDING)
          break;
        if (n < BUFFER - 1 && (size_t)n == cursor.curr &&
            get_http_method(get_route_index(local_machine)) <
                HTTP_POST) { // GET || DELETE with body

          /* snprintf(send_buffer, BUFFER, "HTTP/1.1 400 Bad Request\r\n\r\n");
           */
          /* write(get_client_fd(local_machine), send_buffer, */
          /*       strlen(send_buffer)); // TODO: add error handling */

          bad_request(get_client_fd(local_machine), BUFFER, nullptr, nullptr);
          set_state(local_machine, ENDING);
          should_read = false;
          break;
        }
        bzero(cursor.memory, BUFFER);
        cursor.curr_mem = 0;
        cursor.empty_mem = true;
        cursor.offset = cursor.curr;
        printf("headers terminados\n");
        printf("DEBUG: cursor.offset = %zu\n", cursor.offset);
        printf("DEBUG: cursor.curr = %zu\n", cursor.curr);
        printf("DEBUG: cursor.curr_mem = %zu\n", cursor.curr_mem);
        printf("DEBUG: n = %li\n", n);
        printf("DEBUG: primeros bytes del body: ");
        for (int i = 0; i < 16; i++) {
          printf("%02X ", (unsigned char)read_buffer[cursor.offset + i]);
        }
        printf("\n");

        set_state(local_machine, WORKING);
      } else {
        should_read = true;
        break; // keep processing headers
      }
      [[fallthrough]];
    }
    case WORKING: {

      api_jump_table(&cursor, local_machine, read_buffer);
      set_state(local_machine, ENDING);
      [[fallthrough]];
    }
    case ENDING: {
      close(get_client_fd(local_machine));
      set_state(local_machine, WAITING);
      break;
    }
    default:
      unreachable();
    }
    if (get_state(local_machine) != WORKING) {

      bzero(read_buffer, BUFFER);
    }
    bzero(send_buffer, BUFFER);
  }

  return nullptr;
}
