#include "../includes/common-response.h"
#include "../includes/files.h"
#include "../includes/server-defines.h"
#include "../includes/server-machine.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h> // for open() function
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h> //bzero
#include <sys/sendfile.h>
#include <sys/socket.h> // recv
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h> // for close() function
#include <vips/vips.h>
typedef enum SUPPORTED_FILETYPE {
  FT_UNKNOWN = -1, // err value
  FT_PNG = 0,
  FT_JPEG,
  FT_HEIC
} SUPPORTED_FILETYPE;
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

// This will be sent to the client when getting /api/image/cursor
typedef struct [[gnu::packed]] img_metadata {
  unsigned short idx;
  unsigned long long img_id;
  unsigned long long img_size;
} img_metadata; // 18 bytes (linux x86-64)
struct post_api_image {
  int fd;
  unsigned long long content_length_val; // Valor original, NO modificar
  unsigned long long bytes_received; // ← NUEVO: llevar cuenta de lo recibido
  ssize_t n;
  char *filename;
  SUPPORTED_FILETYPE ft;
  unsigned long long my_idx;
  bool headers_written; // ← NUEVO: para saber si ya escribiste headers
};
struct get_api_image_cursor {
  file *files;
  img_metadata *metadata;
  unsigned long long total_size;
  unsigned short lim;

  unsigned long long *files_ids;
  bool headers_written;
  unsigned short idx_metadata_written;
  off_t metadata_off;
  unsigned short idx_img_written;
  off_t img_off;
};
int free_wrapper(void *ptr) {
  struct post_api_image *p = (struct post_api_image *)ptr;
  if (p == nullptr)
    return 0;

  if (p->filename != nullptr) {
    free(p->filename);
    p->filename = nullptr;
  }

  if (p->fd > 0) {
    close(p->fd);
    p->fd = -1;
  }

  free(p);
  ptr = nullptr;
  return 0;
}
int free_get_api_image_cursor_ctx(void *p) {
  struct get_api_image_cursor *ctx = (struct get_api_image_cursor *)p;
  for (unsigned short i = 0; ctx->files_ids[i] != 0; i++) {

    if (ctx->files[i].fd > 0) {
      close(ctx->files[i].fd);
      ctx->files[i].fd = -1;
    }
  }
  free(ctx->files_ids);
  ctx->files_ids = nullptr;
  free(ctx->files);
  ctx->files = nullptr;
  free(ctx->metadata);
  ctx->metadata = nullptr;
  free(p);

  /* printf("limpiado context de api_image_cursor\n"); */
  p = nullptr;
  return 0;
}

/**
curl -X POST -H "Content-Length: $(wc -c < yo.jpeg)" --data-binary @yo.jpeg
http:/localhost:1600/api/image

*/
endpoint_return get_api_image(server_machine *machine, char *read_buffer);
endpoint_return post_api_image(stream_cursor *cursor, server_machine *machine,
                               char *read_buffer);
endpoint_return get_api_image_cursor(server_machine *machine);
endpoint_return get_api_image_cursor_start(server_machine *machine);
endpoint_return options_api_image_cursor(server_machine *machine);
endpoint_return options_api_image(server_machine *machine);
endpoint_return options_post_api_image(server_machine *machine);
endpoint_return options_api_image_cursor_start(server_machine *machine);
endpoint_return api_jump_table(stream_cursor *cursor, server_machine *machine,
                               char *read_buffer) {
  size_t id = get_route_index(machine);
  switch (id) {
  case 0:
    /* printf("get_api_image:\n"); */
    return get_api_image(machine, read_buffer);
  case 1:
    /* printf("get_api_image_cursor:\n"); */
    return get_api_image_cursor(machine);
  case 2:
    /* printf("post_api_image:\n"); */
    return post_api_image(cursor, machine, read_buffer);
  case 3:
    /* printf("get_api_image_cursor_start:\n"); */
    return get_api_image_cursor_start(machine);
  case 4:
    /* printf("options_api_image_cursor_start:\n"); */
    return options_api_image_cursor_start(machine);
  case 5:
    /* printf("options_api_image_cursor:\n"); */
    return options_api_image_cursor(machine);
  case 6:
    /* printf("options_api_image:\n"); */
    return options_api_image(machine);
  case 7:
    /* printf("options_post_api_image:\n"); */
    return options_post_api_image(machine);
  default:
    unreachable();
  }
}
extern unsigned long long get_idx();
extern unsigned long long get_next_idx();
/*
 * assumes stream is big enough
 * */
SUPPORTED_FILETYPE validate_file_type(char *stream, size_t start) {

  SUPPORTED_FILETYPE ft = FT_UNKNOWN;
  unsigned char *buffer = (unsigned char *)&stream[start];

  // PNG: 89 50 4E 47 0D 0A 1A 0A
  if (buffer[0] == 0x89 && buffer[1] == 0x50 && buffer[2] == 0x4E &&
      buffer[3] == 0x47 && buffer[4] == 0x0D && buffer[5] == 0x0A &&
      buffer[6] == 0x1A && buffer[7] == 0x0A) {
    return FT_PNG;
  }

  // JPEG: FF D8 FF
  if (buffer[0] == 0xFF && buffer[1] == 0xD8 && buffer[2] == 0xFF) {
    return FT_JPEG;
  }

  // HEIC/HEIF: search "ftyp" then "heic", "heix", "mif1", "msf1"
  if (buffer[4] == 0x66 && buffer[5] == 0x74 && buffer[6] == 0x79 &&
      buffer[7] == 0x70) {
    // major brand
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

struct get_api_image {
  file f;
  off_t offset;
};
int free_get_api_image_ctx(void *void_ctx) {
  struct get_api_image *ctx = (struct get_api_image *)void_ctx;
  int close_res = 0;
  if (ctx->f.fd > 0) {
    close_res = close(ctx->f.fd);
  }
  if (ctx->f.name != nullptr) {

    free(ctx->f.name);
    ctx->f.name = nullptr;
  }
  free(ctx);
  void_ctx = nullptr;
  return close_res;
}
endpoint_return get_api_image(server_machine *machine, char *read_buffer) {
  char buffer[BUFFER] = {0};
  if (machine->server_ctx->endpoint_ctx == nullptr) {
    const char *param = get_param(machine, "variant");
    assert(param != nullptr);
    const char *img_id = get_param(machine, "id");
    assert(img_id != nullptr);
    file f = {0};

    struct get_api_image *ctx = malloc(sizeof(struct get_api_image));
    machine->server_ctx->free_endpoint_ctx = free_get_api_image_ctx;
    machine->server_ctx->endpoint_ctx = ctx;
    // offset asigned later...

    if (strcasecmp(param, "original") == 0) {
      f = open_img_at(img_id, IMG_ORIGINAL_DIR);

    } else if (strcasecmp(param, "thumbnail") == 0) {

      f = open_img_at(img_id, IMG_THUMBNAIL_DIR);
    } else if (strcasecmp(param, "compressed") == 0) {

      f = open_img_at(img_id, IMG_CACHE_DIR);
    } else {
      bad_request(get_client_fd(machine), BUFFER, nullptr,
                  "Unsupported variant");
      return FINISHED;
    }
    ctx->f = f;

    size_t last_dot = 0;
    for (size_t i = 0; f.name[i] != '\0'; i++) {
      if (f.name[i] == '.')
        last_dot = i;
    }
    snprintf(buffer, BUFFER,
             "HTTP/1.1 200 OK\r\n%s: %s\r\n%s%s\r\n%s: %lu\r\n%s\r\n\r\n",
             "Server", HOST_NAME, "Content-Type: image/", f.name + last_dot + 1,
             "Content-Length", f.size, cors_headers);
    assert(write(get_client_fd(machine), buffer, strlen(buffer)));
    ssize_t sent = sendfile(get_client_fd(machine), f.fd, nullptr, f.size);

    ctx->offset = sent;

    free(ctx->f.name);
    ctx->f.name = nullptr;

    if (sent == 0) {
      return SOMETHING_WENT_WRONG;
    }
    if (sent == (ssize_t)f.size) {
      return FINISHED;
    }
    return NEED_WRITE_MORE_DATA;
  }
  struct get_api_image *ctx = machine->server_ctx->endpoint_ctx;
  while (ctx->f.size - ctx->offset > 0) {
    ssize_t count = ctx->f.size - ctx->offset;
    ssize_t s =
        sendfile(get_client_fd(machine), ctx->f.fd, &(ctx->offset), count);
    if (s == 0) {
      return SOMETHING_WENT_WRONG; // socket closed
    }
    if (s < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return NEED_WRITE_MORE_DATA;

      return SOMETHING_WENT_WRONG;
      break;
    }
  }

  return FINISHED;
}
endpoint_return post_api_image(stream_cursor *cursor, server_machine *machine,
                               char *read_buffer) {
  server_machine *local_machine = machine;
  struct post_api_image *ctx =
      (struct post_api_image *)machine->server_ctx->endpoint_ctx;

  if (ctx == nullptr) {
    char filename[BUFFER] = {0};

    const char *content_length = get_header(machine, "content-length:");
    if (content_length == nullptr) {
      bad_request(get_client_fd(local_machine), BUFFER, nullptr,
                  "Missing Content-Length header");
      return FINISHED;
    }

    size_t colon_pos = 0;
    while (content_length[colon_pos] != ':' &&
           content_length[colon_pos] != '\0') {
      colon_pos++;
    }

    if (content_length[colon_pos] != ':') {
      bad_request(get_client_fd(local_machine), BUFFER, nullptr,
                  "Invalid Content-Length header");
      return FINISHED;
    }

    size_t value_start = colon_pos + 1;
    while (content_length[value_start] == ' ' ||
           content_length[value_start] == '\t') {
      value_start++;
    }

    unsigned long long content_length_val =
        strtoull(content_length + value_start, nullptr, 10);
    if (content_length_val == 0) {
      bad_request(get_client_fd(local_machine), BUFFER, nullptr,
                  "Content-Length cannot be 0");
      return FINISHED;
    }

    size_t body_offset = cursor->offset;
    size_t bytes_en_buffer = machine->server_ctx->n - body_offset;

    SUPPORTED_FILETYPE ft = validate_file_type(read_buffer, body_offset);
    if (ft == FT_UNKNOWN) {
      bad_request(get_client_fd(local_machine), BUFFER, nullptr,
                  "Unsupported file type");
      return FINISHED;
    }

    unsigned long long my_idx = get_next_idx();
    int filename_res = snprintf(filename, BUFFER, "%s%llu.%s", IMG_ORIGINAL_DIR,
                                my_idx, file_extension(ft));

    if (filename_res < 0 || (unsigned long long)filename_res >= BUFFER) {
      internal_server_error(get_client_fd(local_machine), BUFFER, nullptr,
                            "254");
      return FINISHED;
    }

    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
      internal_server_error(get_client_fd(local_machine), BUFFER, nullptr,
                            "261");
      fd = -1;
      return FINISHED;
    }

    ssize_t initial_w = write(fd, read_buffer + body_offset, bytes_en_buffer);
    if (initial_w < 0) {
      close(fd);
      fd = -1;
      return SOMETHING_WENT_WRONG;
    }

    ctx = malloc(sizeof(struct post_api_image));
    if (ctx == nullptr) {
      internal_server_error(get_client_fd(local_machine), BUFFER, nullptr,
                            "Out of memory");
      close(fd);
      fd = -1;
      return SOMETHING_WENT_WRONG;
    }

    ctx->fd = fd; // will get closed
    ctx->content_length_val = content_length_val;
    ctx->bytes_received = bytes_en_buffer;
    ctx->filename = strdup(filename); // should be freed
    ctx->my_idx = my_idx;
    ctx->headers_written = false;
    ctx->ft = ft;

    machine->server_ctx->endpoint_ctx = (void *)ctx;
    machine->server_ctx->free_endpoint_ctx = free_wrapper;

    bzero(machine->server_ctx->read_buffer, BUFFER);
    bzero(read_buffer, BUFFER);

    if (ctx->bytes_received < ctx->content_length_val) {
      return NEED_READ_MORE_DATA;
    }
  }

  // --- entrypoint for events ---
  char recv_buffer[BUFFER] = {0};

  while (ctx->bytes_received < ctx->content_length_val) {
    size_t remaining = ctx->content_length_val - ctx->bytes_received;
    size_t to_read = (remaining < BUFFER) ? remaining : BUFFER; // NOTE: changed

    ctx->n = recv(get_client_fd(machine), recv_buffer, to_read, 0);
    if (ctx->n > 0) {
      ssize_t loop_w = write(ctx->fd, recv_buffer, ctx->n);
      if (loop_w < 0) {
        return SOMETHING_WENT_WRONG;
      }
      ctx->bytes_received += ctx->n;
      bzero(recv_buffer, BUFFER);

      if (ctx->bytes_received >= ctx->content_length_val) {
        break;
      }
    } else if (ctx->n == 0) {
      if (ctx->bytes_received >= ctx->content_length_val) {
        break;
      }
      internal_server_error(get_client_fd(local_machine), BUFFER, nullptr,
                            "Connection closed before complete data");
      return SOMETHING_WENT_WRONG;
    } else { // ctx->n == -1
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        if (ctx->bytes_received >= ctx->content_length_val) {
          break;
        }
        return NEED_READ_MORE_DATA;
      }
      return SOMETHING_WENT_WRONG;
    }
  }

  if (ctx->bytes_received != ctx->content_length_val) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "331");
    return SOMETHING_WENT_WRONG;
  }

  if (lseek(ctx->fd, 0, SEEK_SET) == (off_t)-1) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "338");
    return SOMETHING_WENT_WRONG;
  }

  VipsImage *in = nullptr;
  VipsSource *source = vips_source_new_from_descriptor(ctx->fd);
  if (!source) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "348");
    return SOMETHING_WENT_WRONG;
  }
  if (fsync(ctx->fd) == -1) {
    // I may need to do something here.
  }

  if (ctx->ft == FT_HEIC) {

    int result = vips_heifload_source(source, &in, "unlimited", TRUE, NULL);
  } else {

    in = vips_image_new_from_source(source, "", NULL);
  }
  g_object_unref(source);
  source = nullptr;

  if (!in) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "358");
    return SOMETHING_WENT_WRONG;
  }

  void *buffer_salida = NULL;
  size_t tam_salida = 0;
  int resultado_save =
      vips_webpsave_buffer(in, &buffer_salida, &tam_salida, NULL);

  char thumbnail_path[BUFFER] = {0};
  int filename_res = snprintf(thumbnail_path, BUFFER, "%s%llu.%s",
                              IMG_THUMBNAIL_DIR, ctx->my_idx, "webp");
  if (filename_res < 0 || filename_res >= BUFFER) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "375");
    g_free(buffer_salida);
    return SOMETHING_WENT_WRONG;
  }

  if (resultado_save != 0) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "381");
    g_free(buffer_salida);
    return SOMETHING_WENT_WRONG;
  }

  int fd_salida = open(thumbnail_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd_salida < 0) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "390");
    g_free(buffer_salida);
    return SOMETHING_WENT_WRONG;
  }

  if (write(fd_salida, buffer_salida, tam_salida) < 0) {
    close(fd_salida);
    g_free(buffer_salida);
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "399");
    return SOMETHING_WENT_WRONG;
  }

  close(fd_salida);
  g_free(buffer_salida);

  buffer_salida = NULL;
  tam_salida = 0;
  resultado_save = vips_jpegsave_buffer(in, &buffer_salida, &tam_salida, NULL);

  char cache_path[BUFFER] = {0};
  filename_res = snprintf(cache_path, BUFFER, "%s%llu.%s", IMG_CACHE_DIR,
                          ctx->my_idx, "jpeg");

  if (filename_res < 0 || filename_res >= BUFFER) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "421");
    g_free(buffer_salida);
    return SOMETHING_WENT_WRONG;
  }

  if (resultado_save != 0) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "427");
    g_free(buffer_salida);
    return SOMETHING_WENT_WRONG;
  }

  fd_salida = open(cache_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd_salida < 0) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "436");
    g_free(buffer_salida);
    return SOMETHING_WENT_WRONG;
  }

  if (write(fd_salida, buffer_salida, tam_salida) < 0) {
    close(fd_salida);
    g_free(buffer_salida);
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "445");
    return SOMETHING_WENT_WRONG;
  }

  close(fd_salida);
  g_free(buffer_salida);

  g_object_unref(in);
  in = nullptr;

  created(get_client_fd(local_machine), BUFFER, nullptr, nullptr);

  return FINISHED;
}
endpoint_return get_api_image_cursor(server_machine *machine) {
  if (machine->server_ctx->endpoint_ctx == nullptr) {
    const char *current = get_param(machine, "current");
    assert(current != nullptr);
    unsigned long long curr = strtoull(current, nullptr, 10);
    if (curr == 0) {
      bad_request(get_client_fd(machine), BUFFER, nullptr,
                  "Invalid current parameter");
      return FINISHED;
    }
    const char *limit = get_param(machine, "limit");
    assert(limit != nullptr);
    unsigned long long lim_tmp = strtoull(limit, nullptr, 10);
    unsigned short lim = (unsigned short)lim_tmp;

    if (lim == 0 || curr < (unsigned long long)(lim + 1)) {
      bad_request(get_client_fd(machine), BUFFER, nullptr,
                  "Invalid limit parameter");
      return FINISHED;
    }
    if (curr > (get_idx() + 1)) {
      bad_request(get_client_fd(machine), BUFFER, nullptr,
                  "Current parameter exceeds available images");
      return FINISHED;
    }

    file *files = malloc(sizeof(file) * lim);
    if (!files) {
      internal_server_error(get_client_fd(machine), BUFFER, nullptr,
                            "Memory allocation failed");
      return FINISHED;
    }

    unsigned short actual_lim = lim;
    unsigned long long *files_ids =
        open_files_to_arr(IMG_THUMBNAIL_DIR, files, &actual_lim, curr);

    if (files_ids == nullptr || actual_lim == 0) {
      free(files);
      char buffer[BUFFER] = {0};
      snprintf(buffer, BUFFER,
               "HTTP/1.1 200 OK\r\n"
               "Server: %s\r\n"
               "Content-Type: application/octet-stream\r\n"
               "Content-Length: 0\r\n"
               "%s\r\n\r\n",
               HOST_NAME, cors_headers);
      write(get_client_fd(machine), buffer, strlen(buffer));
      return FINISHED;
    }

    img_metadata *metadata = malloc(sizeof(img_metadata) * actual_lim);
    if (!metadata) {
      free(files_ids);
      free(files);
      internal_server_error(get_client_fd(machine), BUFFER, nullptr,
                            "Memory allocation failed");
      return FINISHED;
    }

    unsigned long long total_size = 0;
    for (unsigned short i = 0; i < actual_lim; i++) {
      total_size += files[i].size;
    }
    total_size += sizeof(img_metadata) * actual_lim;

    struct get_api_image_cursor *ctx =
        calloc(1, sizeof(struct get_api_image_cursor));
    ctx->files = files;
    ctx->metadata = metadata;
    ctx->total_size = total_size;
    ctx->lim = actual_lim;
    ctx->files_ids = files_ids;
    ctx->headers_written = false;
    ctx->idx_metadata_written = 0;
    ctx->idx_img_written = 0;
    ctx->metadata_off = 0;
    ctx->img_off = 0;

    machine->server_ctx->endpoint_ctx = (void *)ctx;
    machine->server_ctx->free_endpoint_ctx = free_get_api_image_cursor_ctx;
  }

  struct get_api_image_cursor *ctx =
      (struct get_api_image_cursor *)machine->server_ctx->endpoint_ctx;

  if (!ctx->headers_written) {
    char buffer[BUFFER] = {0};
    size_t header_len = snprintf(buffer, BUFFER,
                                 "HTTP/1.1 200 OK\r\n"
                                 "Server: %s\r\n"
                                 "Content-Type: application/octet-stream\r\n"
                                 "Content-Length: %llu\r\n"
                                 "Connection: close\r\n"
                                 "%s\r\n\r\n",
                                 HOST_NAME, ctx->total_size, cors_headers);

    size_t sent = 0;
    while (sent < header_len) {
      ssize_t s =
          write(get_client_fd(machine), buffer + sent, header_len - sent);
      if (s < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
          return NEED_WRITE_MORE_DATA;
        return SOMETHING_WENT_WRONG;
      }
      if (s == 0) {
        return SOMETHING_WENT_WRONG;
      }
      sent += s;
    }
    ctx->headers_written = true;
  }

  for (unsigned short i = ctx->idx_img_written; i < ctx->lim; i++) {

    if (ctx->idx_metadata_written == i) {
      ctx->metadata[i] = (img_metadata){.idx = i,
                                        .img_id = ctx->files_ids[i],
                                        .img_size = ctx->files[i].size};

      size_t total = sizeof(img_metadata);
      char *ptr = (char *)&ctx->metadata[i];

      while (ctx->metadata_off < (off_t)total) {
        ssize_t s = write(get_client_fd(machine), ptr + ctx->metadata_off,
                          total - ctx->metadata_off);
        if (s < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            return NEED_WRITE_MORE_DATA;
          return SOMETHING_WENT_WRONG;
        }
        if (s == 0) {
          return SOMETHING_WENT_WRONG;
        }
        ctx->metadata_off += s;
      }
      ctx->idx_metadata_written++;
      ctx->metadata_off = 0; // Reset for next metadata
    }

    // Send file data
    while (ctx->img_off < (off_t)ctx->files[i].size) {
      size_t remaining = ctx->files[i].size - ctx->img_off;
      ssize_t s = sendfile(get_client_fd(machine), ctx->files[i].fd,
                           &ctx->img_off, remaining);

      if (s < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
          return NEED_WRITE_MORE_DATA;
        return SOMETHING_WENT_WRONG;
      }
      if (s == 0) {
        break; // EOF
      }
    }

    // next file
    ctx->idx_img_written++;
    ctx->img_off = 0; // Reset offset for next file
  }

  return FINISHED;
}
endpoint_return options_api_image_cursor(server_machine *machine) {
  char buffer[BUFFER] = {0};
  snprintf(buffer, BUFFER, "HTTP/1.1 204 No Content\r\n%s: %s\r\n%s\r\n",
           "Server", HOST_NAME, cors_headers);
  assert(write(get_client_fd(machine), buffer, strlen(buffer)));
  return FINISHED;
}
endpoint_return options_api_image(server_machine *machine) {
  char buffer[BUFFER] = {0};
  snprintf(buffer, BUFFER, "HTTP/1.1 204 No Content\r\n%s: %s\r\n%s\r\n",
           "Server", HOST_NAME, cors_headers);
  assert(write(get_client_fd(machine), buffer, strlen(buffer)));
  return FINISHED;
}
endpoint_return options_post_api_image(server_machine *machine) {
  char buffer[BUFFER] = {0};
  const char *const _cors_headers =
      "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: "
      "POST, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type, "
      "Accept\r\nAccess-Control-Expose-Headers: Content-Length, "
      "Content-Type";

  snprintf(buffer, BUFFER, "HTTP/1.1 204 No Content\r\n%s: %s\r\n%s\r\n",
           "Server", HOST_NAME, _cors_headers);
  assert(write(get_client_fd(machine), buffer, strlen(buffer)));
  return FINISHED;
}

endpoint_return get_api_image_cursor_start(server_machine *machine) {

  char buffer[BUFFER] = {0};
  snprintf(buffer, BUFFER, "HTTP/1.1 200 OK\r\n%s: %s\r\n%s\r\n\r\n%llu",
           "Server", HOST_NAME, cors_headers, get_idx() + 1);
  assert(write(get_client_fd(machine), buffer, strlen(buffer)));
  return FINISHED;
}
endpoint_return options_api_image_cursor_start(server_machine *machine) {

  char buffer[BUFFER] = {0};
  snprintf(buffer, BUFFER, "HTTP/1.1 204 No Content\r\n%s: %s\r\n%s\r\n",
           "Server", HOST_NAME, cors_headers);
  assert(write(get_client_fd(machine), buffer, strlen(buffer)));
  return FINISHED;
}
