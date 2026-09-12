#include "../includes/files.h"   // for fperror
#include "../includes/defines.h" // for fperror
#include "logger.h"
#include <inttypes.h>
#include <stdint.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h> // for open() function
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // for fstat() function
#include <sys/types.h>
#include <unistd.h> // for close() function
int open_file(file *f, const char *r_path, int flag) {
  logger_log(LOG_HIGH | LOG_MEDIUM, "%s: r_path: %s\n", __func__, r_path);
  f->fd = open(r_path, flag);
  if (f->fd < 0) {
    fperror;
    return -1;
  }
  struct stat fs;
  if (fstat(f->fd, &fs) == -1) {
    fperror;
    logger_log(LOG_HIGH | LOG_MEDIUM, "%s: closing `f->fd` (%d)\n", __func__,
               f->fd);
    close(f->fd);
    f->fd = -1;
    return -1;
  }
  f->size = (size_t)fs.st_size;
  return 0;
}

int close_file(file *f) {
  logger_log(LOG_HIGH | LOG_MEDIUM, "%s: closing `f->fd` (%d)\n", __func__,
             f->fd);
  if (close(f->fd) < 0) {
    fperror;
    return -1;
  }
  f->fd = -1; // Reset fd to indicate that the file is closed
  return 0;
}
int attempt_create_dir(const char *path, mode_t mode) {
  int res = mkdir(path, mode);
  if (res == -1) {
    if (errno == EEXIST)
      return 1;
    return -1;
  }
  return 0;
}

uint64_t get_biggest_index(const char *path) {
  DIR *dir = opendir(path);
  struct dirent *entry;
  uint64_t max = 0, num = 0;
  /* char *dot, nombre_sin_ext[256]; */

  assert(dir);

  while ((entry = readdir(dir))) {
    // omit "." and ".."
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    size_t i = 0;
    for (; entry->d_name[i] != '\0' && isdigit(entry->d_name[i]); i++)
      ;
    if (entry->d_name[i] != '.')
      continue;
    char *file_name = entry->d_name;
    file_name[i] = '\0';

    num = strtoull(file_name, nullptr, 10);
    if (num > max) {
      max = num;
    } else {

      num = max;
    }
  }

  closedir(dir);
  return num;
}

file open_img_at(const char *id, const char *path) {
  DIR *dir = opendir(path);
  struct dirent *entry;
  assert(dir);
  file f = {0};

  while ((entry = readdir(dir))) {
    size_t i = 0;
    for (; entry->d_name[i] != '\0' && isdigit(entry->d_name[i]); i++)
      ;
    if (i > 0 && strncmp(entry->d_name, id, i) == 0) {
      char b[BUFFER] = {0};
      snprintf(b, sizeof(b), "%s%s", path, entry->d_name);
      open_file(&f, b, O_RDONLY);
      f.name = strdup(entry->d_name);

      break;
    }
  }
  closedir(dir);
  return f; // not found.
}
#warning "I should handle file not found case"
uint64_t *open_files_to_arr(const char *path, file *const out,
                            uint64_t *files_arr, size_t files_arr_size) {

  logger_log(LOG_HIGH | LOG_MEDIUM, "open_files_to_arr: arr_size: %lu\n",
             files_arr_size);
  char curr_filename[21];

  for (size_t i = 0; i < files_arr_size; i++) {

    snprintf(curr_filename, sizeof(curr_filename), "%" PRIu64 ".webp",
             files_arr[i]);
    char b[BUFFER] = {0};
    snprintf(b, sizeof(b), "%s%s", path, curr_filename);
    logger_log(LOG_HIGH | LOG_MEDIUM, "%s\n", b);
    open_file(&out[i], b, O_RDONLY);
  }

  return (uint64_t *)files_arr;
}
