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

// This will be sent to the client when getting /api/image/cursor
typedef struct [[gnu::packed]] img_metadata {
  unsigned short idx;
  unsigned long long img_id;
  unsigned long long img_size;
} img_metadata; // 18 bytes (linux x86-64)
struct post_api_image {
  int fd;
  unsigned long long content_length_val;
  ssize_t n;
  char *filename;
  unsigned long long my_idx;
};
struct get_api_image_cursor {
  file *files;
  img_metadata *metadata;
  unsigned long long total_size;
  unsigned short lim;

  unsigned long long *files_ids;
  bool headers_written;
  unsigned short idx_metadata_written;
  unsigned short idx_img_written;
};
int free_wrapper(void *ptr) {
  struct post_api_image *p = (struct post_api_image *)ptr;
  free(p->filename);
  free(p);
  ptr = nullptr;
  return 0;
}
int free_get_api_image_cursor_ctx(void *p) {
  struct get_api_image_cursor *ctx = (struct get_api_image_cursor *)p;
  free(ctx->files_ids);
  ctx->files_ids = nullptr;
  free(ctx->files);
  ctx->files = nullptr;
  free(ctx->metadata);
  ctx->metadata = nullptr;
  free(p);
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
    return get_api_image(machine, read_buffer);
  case 1:
    return get_api_image_cursor(machine);
  case 2:
    return post_api_image(cursor, machine, read_buffer);
  case 3:
    return get_api_image_cursor_start(machine);
  case 4:
    return options_api_image_cursor_start(machine);
  case 5:
    return options_api_image_cursor(machine);
  case 6:
    return options_api_image(machine);
  case 7:
    return options_post_api_image(machine);
  default:
    unreachable();
  }
}
extern unsigned long long get_idx();
extern unsigned long long get_next_idx();
typedef enum SUPPORTED_FILETYPE {
  FT_UNKNOWN = -1, // err value
  FT_PNG = 0,
  FT_JPEG,
  FT_HEIC
} SUPPORTED_FILETYPE;
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

endpoint_return get_api_image(server_machine *machine, char *read_buffer) {
  const char *param = get_param(machine, "variant");
  assert(param != nullptr);
  const char *img_id = get_param(machine, "id");
  assert(img_id != nullptr);
  file f = {0};
  if (strcasecmp(param, "original") == 0) {
    f = open_img_at(img_id, IMG_ORIGINAL_DIR);
  } else if (strcasecmp(param, "thumbnail") == 0) {

    f = open_img_at(img_id, IMG_THUMBNAIL_DIR);
  } else if (strcasecmp(param, "compressed") == 0) {

    f = open_img_at(img_id, IMG_CACHE_DIR);
  } else {
    bad_request(get_client_fd(machine), BUFFER, nullptr, "Unsupported variant");
    set_state(machine, ENDING);
    return FINISHED;
  }
  char buffer[BUFFER] = {0};
  snprintf(buffer, BUFFER, "HTTP/1.1 200 OK\r\n%s: %s\r\n%s: %lu\r\n%s\r\n\r\n",
           "Server", HOST_NAME, "Content-Length", f.size, cors_headers);
  assert(write(get_client_fd(machine), buffer, strlen(buffer)));
  ssize_t sent = sendfile(get_client_fd(machine), f.fd, nullptr, f.size);
  while (f.size - sent > 0) {
    ssize_t count = f.size - sent;
    ssize_t s = sendfile(get_client_fd(machine), f.fd, &sent, count);
    if (s < 0) {
      break;
    }
  }
  if (f.fd > 0) {
    close(f.fd);
  }

  return FINISHED;
}

// Must have processed headers
endpoint_return post_api_image(stream_cursor *cursor, server_machine *machine,
                               char *read_buffer) {
  assert(BUFFER > 12);
  server_machine *local_machine = machine;
  endpoint_return ret_val = FINISHED;
  int fd = -1;
  VipsImage *in = nullptr;
  if (local_machine->server_ctx->endpoint_ctx == nullptr) {

    ssize_t n = 0;
    char filename[BUFFER] = {0};

    const char *content_length = get_header(machine, "content-length:");
    assert(content_length != nullptr);

    size_t colon_pos = 0;
    while (content_length[colon_pos] != ':' &&
           content_length[colon_pos] != '\0') {
      colon_pos++;
    }

    if (content_length[colon_pos] != ':') {
      bad_request(get_client_fd(local_machine), BUFFER, nullptr, nullptr);
      goto cleanup;
    }

    size_t value_start = colon_pos + 1;
    while (content_length[value_start] == ' ' ||
           content_length[value_start] == '\t') {
      value_start++;
    }

    unsigned long long content_length_val =
        strtoull(content_length + value_start, nullptr, 10);

    SUPPORTED_FILETYPE ft = FT_UNKNOWN;

    size_t body_offset = cursor->offset;
    size_t bytes_en_buffer = BUFFER - 1 - body_offset;

    ft = validate_file_type(read_buffer, body_offset);
    if (ft == FT_UNKNOWN) {
      bad_request(get_client_fd(local_machine), BUFFER, nullptr,
                  "unsuported file type");
      goto cleanup;
    }

    unsigned long long my_idx = get_next_idx();
    int filename_res = snprintf(filename, BUFFER, "%s%llu.%s", IMG_ORIGINAL_DIR,
                                my_idx, file_extension(ft));

    if (filename_res < 0 || (unsigned long long)filename_res >= BUFFER) {
      internal_server_error(get_client_fd(local_machine), BUFFER, nullptr,
                            "254");
      goto cleanup;
    }
    fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
      perror("[debug] error en open() de archivo original");
      internal_server_error(get_client_fd(local_machine), BUFFER, nullptr,
                            "261");
      goto cleanup;
    }
    struct post_api_image *ctx = malloc(sizeof(struct post_api_image));
    machine->server_ctx->endpoint_ctx = (void *)ctx;
    machine->server_ctx->free_endpoint_ctx = free_wrapper;
    ssize_t initial_w = write(fd, read_buffer + body_offset, bytes_en_buffer);
    if (initial_w < 0) {
      perror("[DEBUG] Error escribiendo buffer inicial en disco");
      goto cleanup;
    }
    content_length_val -= bytes_en_buffer;
    ((struct post_api_image *)machine->server_ctx->endpoint_ctx)
        ->content_length_val = content_length_val;
    printf("content_length_val: %llu\n", content_length_val);
    ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->n = n;
    ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->filename =
        strdup(filename);
    ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->my_idx =
        my_idx;
    ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->fd = fd;
    bzero(read_buffer, BUFFER);
    /* struct timeval tv; */
    /* tv.tv_sec = 5; */
    /* tv.tv_usec = 1000; */
    /* setsockopt(get_client_fd(machine), SOL_SOCKET, SO_RCVTIMEO, (const char
     * *)&tv, */
    /*            sizeof(tv)); */
  }
  char recv_buffer[BUFFER] = {0};

  while (((struct post_api_image *)machine->server_ctx->endpoint_ctx)
             ->content_length_val > 0) {
    ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->n =
        recv(get_client_fd(machine), recv_buffer, BUFFER - 1, 0);

    if (((struct post_api_image *)machine->server_ctx->endpoint_ctx)->n >
        (ssize_t)((struct post_api_image *)machine->server_ctx->endpoint_ctx)
            ->content_length_val) {
      ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->n =
          ((struct post_api_image *)machine->server_ctx->endpoint_ctx)
              ->content_length_val;
    }
    if (((struct post_api_image *)machine->server_ctx->endpoint_ctx)->n > 0) {
      ssize_t loop_w = write(
          ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->fd,
          recv_buffer,
          ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->n);
      if (loop_w < 0) {
        break;
      }
      ((struct post_api_image *)machine->server_ctx->endpoint_ctx)
          ->content_length_val -=
          ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->n;
      bzero(recv_buffer, BUFFER);
    } else if (((struct post_api_image *)machine->server_ctx->endpoint_ctx)
                   ->n == 0) {
      internal_server_error(get_client_fd(local_machine), BUFFER, nullptr,
                            "317");
      break;
    }
    if (((struct post_api_image *)machine->server_ctx->endpoint_ctx)->n == -1) {

      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return NEED_READ_MORE_DATA;
      else
        return SOMETHING_WENT_WRONG;
    }
  }

  if (((struct post_api_image *)machine->server_ctx->endpoint_ctx)
          ->content_length_val != 0) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "331");
    goto cleanup;
  }

  if (lseek(((struct post_api_image *)machine->server_ctx->endpoint_ctx)->fd, 0,
            SEEK_SET) == (off_t)-1) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "338");
    goto cleanup;
  }

  // IMG TRANSFORMATION
  VipsSource *source = nullptr;
  source = vips_source_new_from_descriptor(
      ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->fd);
  if (!source) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "348");
    goto cleanup;
  }

  in = vips_image_new_from_source(source, "", NULL);
  g_object_unref(source);
  source = nullptr;

  if (!in) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "358");
    goto cleanup;
  }

  void *buffer_salida = NULL;
  size_t tam_salida = 0;
  int resultado_save = -1;

  resultado_save = vips_webpsave_buffer(in, &buffer_salida, &tam_salida, NULL);

  int filename_res = snprintf(
      ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->filename,
      BUFFER, "%s%llu.%s", IMG_THUMBNAIL_DIR,
      ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->my_idx,
      "webp");
  if (filename_res < 0 || filename_res >= BUFFER) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "375");
    goto cleanup;
  }

  if (resultado_save != 0) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "381");
    goto cleanup;
  }

  int fd_salida = open(
      ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->filename,
      O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd_salida < 0) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "390");
    g_free(buffer_salida);
    goto cleanup;
  }

  if (write(fd_salida, buffer_salida, tam_salida) < 0) {
    close(fd_salida);
    g_free(buffer_salida);
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "399");
    goto cleanup;
  }

  close(fd_salida);
  g_free(buffer_salida);

  // --- JPEG ---
  buffer_salida = nullptr;
  tam_salida = 0;
  resultado_save = -1;

  resultado_save = vips_jpegsave_buffer(in, &buffer_salida, &tam_salida, NULL);

  filename_res = snprintf(
      ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->filename,
      BUFFER, "%s%llu.%s", IMG_CACHE_DIR,
      ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->my_idx,
      "jpeg");

  if (filename_res < 0 || filename_res >= BUFFER) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "421");
    goto cleanup;
  }

  if (resultado_save != 0) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "427");
    goto cleanup;
  }

  fd_salida = open(
      ((struct post_api_image *)machine->server_ctx->endpoint_ctx)->filename,
      O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd_salida < 0) {
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "436");
    g_free(buffer_salida);
    goto cleanup;
  }

  if (write(fd_salida, buffer_salida, tam_salida) < 0) {
    close(fd_salida);
    g_free(buffer_salida);
    internal_server_error(get_client_fd(local_machine), BUFFER, nullptr, "445");
    goto cleanup;
  }

  close(fd_salida);
  g_free(buffer_salida);

  g_object_unref(in);
  in = nullptr;

  if (fd >= 0) {
    close(fd);
    fd = -1;
  }

  created(get_client_fd(local_machine), BUFFER, nullptr, nullptr);
cleanup:
  if (in) {
    g_object_unref(in);
  }
  if (fd >= 0) {
    close(fd);
  }
  return ret_val;
}
endpoint_return get_api_image_cursor(server_machine *machine) {

  if (machine->server_ctx->endpoint_ctx == nullptr) {
    const char *current = get_param(machine, "current");
    assert(current != nullptr);
    unsigned long long curr = strtoul(current, nullptr, 10);
    if (curr == 0) {
      bad_request(get_client_fd(machine), BUFFER, nullptr,
                  "Invalid current parameter");
      return FINISHED;
    }
    const char *limit = get_param(machine, "limit");
    assert(limit != nullptr);
    const unsigned short lim = strtoul(limit, nullptr, 10);
    if (limit == 0 || curr < (unsigned long long)(lim + 1)) {
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
    unsigned long long *files_ids = open_files_to_arr(
        IMG_THUMBNAIL_DIR, files, (unsigned short *const)&lim, curr);
    assert(files_ids != nullptr);
    img_metadata *metadata = malloc(sizeof(img_metadata) * lim);
    unsigned long long total_size = 0;
    for (unsigned short i = 0; i < lim && files_ids[i] != 0; i++) {
      total_size += files[i].size;
    }
    total_size += sizeof(img_metadata) * lim;
    struct get_api_image_cursor *ctx =
        calloc(1, sizeof(struct get_api_image_cursor));
    ctx->files = files;
    ctx->metadata = metadata;
    ctx->total_size = total_size;
    ctx->lim = lim;
    ctx->files_ids = files_ids;

    machine->server_ctx->endpoint_ctx = (void *)ctx;
    machine->server_ctx->free_endpoint_ctx = free_get_api_image_cursor_ctx;
  }

  struct get_api_image_cursor *ctx =
      (struct get_api_image_cursor *)machine->server_ctx->endpoint_ctx;
  if (!ctx->headers_written) {

    char buffer[BUFFER] = {0};
    snprintf(buffer, BUFFER,
             "HTTP/1.1 200 OK\r\n%s: %s\r\n%s: %llu\r\n%s\r\n\r\n", "Server",
             HOST_NAME, "Content-Length", ctx->total_size, cors_headers);
    ssize_t s = write(get_client_fd(machine), buffer, strlen(buffer));
    if (s < 0) {

      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return NEED_WRITE_MORE_DATA;
      else
        return SOMETHING_WENT_WRONG;
    }
    ctx->headers_written = true;
  }
  for (unsigned short i = ctx->idx_metadata_written > ctx->idx_img_written
                              ? ctx->idx_img_written
                              : ctx->idx_metadata_written;
       i < ctx->lim && ctx->files_ids[i] != 0; i++) {

    ctx->metadata[i] = (img_metadata){
        .idx = i, .img_id = ctx->files_ids[i], .img_size = ctx->files[i].size};
    if (ctx->idx_metadata_written == ctx->idx_img_written) {

      ssize_t s = write(get_client_fd(machine), &ctx->metadata[i],
                        sizeof(img_metadata));

      if (s < 0) {

        if (errno == EAGAIN || errno == EWOULDBLOCK)
          return NEED_WRITE_MORE_DATA;
        else
          return SOMETHING_WENT_WRONG;
      }
      ctx->idx_metadata_written++;
    }
    if (ctx->idx_metadata_written > ctx->idx_img_written) {

      ssize_t s = sendfile(get_client_fd(machine), ctx->files[i].fd, nullptr,
                           ctx->files[i].size);
      if (s < 0) {

        if (errno == EAGAIN || errno == EWOULDBLOCK)
          return NEED_WRITE_MORE_DATA;
        else
          return SOMETHING_WENT_WRONG;
      }
    }
    if (ctx->files[i].fd > 0) {
      close(ctx->files[i].fd);
    }
  }

  /* free(files_ids); */
  /* files_ids = nullptr; */
  /* free(files); */
  /* files = nullptr; */
  /* free(metadata); */
  /* metadata = nullptr; */
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
