#include "../includes/common-response.h"
#include "../includes/files.h"
#include "../includes/server-defines.h"
#include "../includes/server-machine.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h> // for open() function
#include <stddef.h>
#include <stdio.h>
#include <strings.h> //bzero
#include <sys/sendfile.h>
#include <sys/socket.h> // recv
#include <sys/time.h>
#include <unistd.h> // for close() function

size_t veces_ft = 0;
/**
curl -X POST -H "Content-Length: $(wc -c < yo.jpeg)" --data-binary @yo.jpeg
http:/localhost:1600/api/image

*/
void *get_api_image(server_machine *machine, char *read_buffer);
void *post_api_image(stream_cursor *cursor, server_machine *machine,
                     char *read_buffer);
void *api_jump_table(stream_cursor *cursor, server_machine *machine,
                     char *read_buffer) {
  size_t id = get_route_index(machine);
  switch (id) {
  case 0:
    return get_api_image(machine, read_buffer);
  case 1:
    return nullptr;
  case 2:
    return post_api_image(cursor, machine, read_buffer);
  case 3:

    return nullptr;
  default:
    unreachable();
  }
}
extern unsigned long long get_idx();
typedef enum SUPPORTED_FILETYPE {
  FT_UNKNOWN = -1, // Valor explicito para error
  FT_PNG = 0,
  FT_JPEG,
  FT_HEIC
} SUPPORTED_FILETYPE;
SUPPORTED_FILETYPE validate_file_type(char *stream, size_t start) {
  printf("DEBUG: %s\n", __func__);

  SUPPORTED_FILETYPE ft = FT_UNKNOWN;
  unsigned char *buffer = (unsigned char *)&stream[start];

  // Verificar que tengamos al menos 12 bytes disponibles
  // (Necesitas pasar el tamaño total o asumir que stream es suficientemente
  // grande)

  // PNG: 89 50 4E 47 0D 0A 1A 0A
  if (buffer[0] == 0x89 && buffer[1] == 0x50 && buffer[2] == 0x4E &&
      buffer[3] == 0x47 && buffer[4] == 0x0D && buffer[5] == 0x0A &&
      buffer[6] == 0x1A && buffer[7] == 0x0A) {
    return FT_PNG;
  }

  // JPEG: FF D8 FF
  if (buffer[0] == 0xFF && buffer[1] == 0xD8 && buffer[2] == 0xFF) {
    // Puedes verificar el marcador específico (E0, E1, etc.) si quieres
    return FT_JPEG;
  }

  // HEIC/HEIF: busca "ftyp" y luego "heic", "heix", "mif1", "msf1"
  if (buffer[4] == 0x66 && buffer[5] == 0x74 && buffer[6] == 0x79 &&
      buffer[7] == 0x70) {
    // Verificar el brand (major brand)
    if ((buffer[8] == 0x68 && buffer[9] == 0x65 && buffer[10] == 0x69 &&
         buffer[11] == 0x63) || // heic
        (buffer[8] == 0x68 && buffer[9] == 0x65 && buffer[10] == 0x69 &&
         buffer[11] == 0x78) || // heix
        (buffer[8] == 0x6D && buffer[9] == 0x69 && buffer[10] == 0x66 &&
         buffer[11] == 0x31) || // mif1
        (buffer[8] == 0x6D && buffer[9] == 0x73 && buffer[10] == 0x66 &&
         buffer[11] == 0x31)) { // msf1
      return FT_HEIC;
    }
  }

  return ft;
}
const char *file_extension(SUPPORTED_FILETYPE ft) {
  assert(ft != FT_UNKNOWN);
  switch (ft) {

  case FT_PNG:
    return "png";
  case FT_JPEG:
    return "jpeg";
  case FT_HEIC:
    return "heic";
  default:
    return "unknown";
  }
}

void *get_api_image(server_machine *machine, char *read_buffer) {
  const char *param = get_param(machine, "variant");
  assert(param != nullptr);
  const char *img_id = get_param(machine, "id");
  printf("DEBUG: variant param = %s\n", param);
  printf("DEBUG: id param = %s\n", img_id);
  assert(img_id != nullptr);
  file f = {0};
  if (strcasecmp(param, "original") == 0) {
    printf("DEBUG: fetching original image with id %s\n", img_id);
    f = open_img_at(img_id, IMG_ORIGINAL_DIR);
  } else {
    bad_request(get_client_fd(machine), BUFFER, nullptr, "Unsupported variant");
    set_state(machine, ENDING);
    return nullptr;
  }
  char buffer[BUFFER] = {0};
  snprintf(buffer, BUFFER, "HTTP/1.1 200 OK\r\n%s: %s\r\n%s: %lu\r\n\r\n",
           "Server", HOST_NAME, "Content-Length", f.size);
  write(get_client_fd(machine), buffer, strlen(buffer));
  assert(sendfile(get_client_fd(machine), f.fd, nullptr, f.size) == f.size);
  if (f.fd > 0) {
    close(f.fd);
  }

  return nullptr;
}

// Must have processed headers
void *post_api_image(stream_cursor *cursor, server_machine *machine,
                     char *read_buffer) {
  assert(BUFFER > 12);
  server_machine *local_machine = machine;
  size_t n = 0;
  char send_buffer[BUFFER] = {0};
  char filename[BUFFER] = {0};
  int fd = -1;

  const char *content_length = get_header(machine, "Content-Length:");
  assert(content_length != nullptr);

  // Buscar ':' en lugar de espacio
  size_t colon_pos = 0;
  while (content_length[colon_pos] != ':' &&
         content_length[colon_pos] != '\0') {
    colon_pos++;
  }

  if (content_length[colon_pos] != ':') {
    printf("DEBUG: No se encontró ':' en Content-Length header\n");
    // No se encontraron dos puntos
    /* snprintf(send_buffer, BUFFER, "HTTP/1.1 400 Bad Request\r\n\r\n"); */
    /* write(get_client_fd(local_machine), send_buffer, strlen(send_buffer)); */

    bad_request(get_client_fd(local_machine), BUFFER, nullptr, nullptr);
    goto cleanup;
  }

  // Avanzar después de ':' y los espacios
  size_t value_start = colon_pos + 1;
  while (content_length[value_start] == ' ' ||
         content_length[value_start] == '\t') {
    value_start++;
  }

  // Convertir el valor numérico
  unsigned long long content_length_val =
      strtoull(content_length + value_start, nullptr, 10);

  printf("DEBUG: Content-Length value = %llu\n", content_length_val);
  SUPPORTED_FILETYPE ft = FT_UNKNOWN;

  // Ya tenemos el body en read_buffer + cursor->curr
  size_t body_offset = cursor->offset; // 95
  size_t bytes_en_buffer =
      BUFFER - 1 - body_offset; // 191 - 95 = 96 bytes del body

  // Validar tipo
  ft = validate_file_type(read_buffer, body_offset);
  printf("DEBUG: file type detected: %d\n", ft);

  printf("DEBUG ft: %d | veces: %lu\n", ft, veces_ft++);
  if (ft == FT_UNKNOWN) {
    /* snprintf(send_buffer, BUFFER, */
    /*          "HTTP/1.1 400 Bad Request\r\n\r\nUnsupported file type"); */
    /* write(get_client_fd(local_machine), send_buffer, strlen(send_buffer)); */
    bad_request(get_client_fd(local_machine), BUFFER, nullptr,
                "Unsuported file type");
    goto cleanup;
  }

  unsigned long long my_idx = get_idx();
  int filename_res = snprintf(filename, BUFFER, "%s%llu.%s", IMG_ORIGINAL_DIR,
                              my_idx, file_extension(ft));
  if (filename_res > 0) {

    assert((unsigned long long)filename_res < BUFFER);
  } else {
    printf("DEBUG: %s remember to handle all values of `snprintf` bc it failed "
           "with: %d\n",
           __func__, filename_res);
  }
  fd = open(filename, O_CREAT | O_WRONLY | O_APPEND, 0644);
  if (fd < 0) {
    /* snprintf(send_buffer, BUFFER, "HTTP/1.1 500 Internal Server
     * Error\r\n\r\n"); */
    /* write(get_client_fd(local_machine), send_buffer, strlen(send_buffer)); */
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr,
                          nullptr);
    goto cleanup;
  }

  // Escribir los bytes del body que ya tenemos
  assert(write(fd, read_buffer + body_offset, bytes_en_buffer));
  content_length_val -= bytes_en_buffer;

  // Leer el resto del body
  bzero(read_buffer, BUFFER);
  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 1000;
  setsockopt(get_client_fd(machine), SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv,
             sizeof(tv));
  while (content_length_val != 0) {
    n = recv(get_client_fd(machine), read_buffer, BUFFER - 1, MSG_DONTWAIT);

    printf("DEBUG: Content-Length value = %llu\n", content_length_val);
    printf("DEBUG: n = %lu\n", n);
    if (n > 0) {

      assert(write(fd, read_buffer, n));
      content_length_val -= n;
      bzero(read_buffer, BUFFER);
    } else if (n == 0) {
      /* snprintf(send_buffer, BUFFER, */
      /*          "HTTP/1.1 500 Internal Server Error\r\n\r\n"); */

      internal_server_error(get_client_fd(local_machine), BUFFER, nullptr,
                            nullptr);
      break;
    } else {
      perror("read");
      break;
    }
  } // Will exit when theres an error, I expect either EAGAIN or EWOULDBLOCK
    // that would mean EOF

  if (content_length_val == 0)
    /* snprintf(send_buffer, BUFFER, "HTTP/1.1 201 Created\r\n\r\n"); */
    created(get_client_fd(local_machine), BUFFER, nullptr, nullptr);
  ssize_t write_res =
      write(get_client_fd(local_machine), send_buffer, strlen(send_buffer));
  if (write_res < 0) {
    fperror;
    assert(0);
  }
cleanup:
  if (fd >= 0)
    close(fd);
  set_state(local_machine, ENDING);
  return nullptr;
}
