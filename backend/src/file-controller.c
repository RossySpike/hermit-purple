#include "../includes/file-controller.h"
#include "../includes/server-defines.h"
#include "defines.h"
#include "files.h"
#include "logger.h"
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static file_controller *controller = {0};

#define this (*controller)
#define this_at(idx) ((uint64_t)(uintptr_t)this.images.array[idx])
int sort_asc(const void *a, const void *b) {
  const uint64_t x = (uint64_t)(uintptr_t)*(void *const *)a;

  const uint64_t y = (uint64_t)(uintptr_t)*(void *const *)b;

  if (x < y)
    return -1;
  if (x > y)
    return 1;
  return 0;
}

uint64_t file_controller_get_length(void) { return this.images.len; }
void file_controller_init() {

  controller = malloc(sizeof(file_controller));
  list_new(&this.images);

  DIR *dir = opendir(IMG_THUMBNAIL_DIR);
  struct dirent *entry;
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

    /* uint64_t *ptr = malloc(sizeof(uint64_t)); */
    /* *ptr = strtoull(file_name, nullptr, 10); */

    file_controller_record_file(strtoull(file_name, nullptr, 10));
#if LOG_LEVEL == LOG_HIGH
    logger_log("appended: %" PRIu64 "\n", this_at(this.images.len - 1));
#endif
  }

  closedir(dir);

#if LOG_LEVEL == LOG_HIGH
  logger_log("len: %lu, cap: %lu\n", this.images.len, this.images.capacity);
#endif
  qsort(this.images.array, this.images.len, sizeof(void *), sort_asc);
  this.ready = true;
#if LOG_LEVEL == LOG_HIGH
  logger_log("files: [");
  for (size_t i = 0; i < this.images.len; i++) {

    logger_log(" %" PRIu64 " ", this_at(i));
  }
  logger_log("] \n");
#endif
}

void file_controller_record_file(uint64_t id) {
  assert(controller);
  list_append(&this.images, (void *)(uintptr_t)id);
}

bool binary_search(uint64_t id, size_t *found_at) {
  assert(controller);
  void *needle = (void *)(uintptr_t)id;

  void **res = bsearch(&needle, this.images.array, this.images.len,
                       sizeof(void *), sort_asc);

#if LOG_LEVEL == LOG_HIGH
  logger_log("id: %llu, found_at: %lu\n", (unsigned long long)id, *found_at);
#endif
  if (res != nullptr) {
    if (found_at) {
      // Pointer aritmethics
      *found_at = (size_t)(res - (void **)this.images.array);
    }
    return true;
  }

  return false;
}

/*
 * On error:
 * - batch.size == 0 if the it cannot find the id;
 * - batch.batch == nullptr if calloc fails
 */
bool file_controller_find(uint64_t needle, size_t *found_at) {
  assert(controller);

  if (!binary_search(needle, found_at)) {
    return false;
  }
  return true;
}
batch_t file_controller_get_batch(uint64_t start, size_t size) {

  assert(controller);
  batch_t batch = {.size = size < this.images.len ? size : this.images.len,
                   .batch = nullptr};

#if LOG_LEVEL == LOG_HIGH
  logger_log("%s: size(%lu) < this.images.len (%lu) ? size (%lu) : "
             "this.images.len - size (%lu)\n",
             __func__, size, this.images.len, size, this.images.len);
  logger_log("%s: batch: .size: %lu\n", __func__, batch.size);
#endif
  uint64_t *arr = calloc(batch.size, sizeof(uint64_t));
  if (!arr)
    return batch;

  batch.batch = arr;

  for (size_t batch_idx = 0; batch_idx < batch.size; batch_idx++) {
    size_t i = start - 1 - batch_idx;
#if LOG_LEVEL == LOG_HIGH
    logger_log("idx: %lu, batch_idx: %lu, at: %llu\n", i, batch_idx,
               (unsigned long long)this_at(i));
#endif
    arr[batch_idx] = this_at(i);
  }

#if LOG_LEVEL == LOG_HIGH
  logger_log("batch: [");
  for (size_t i = 0; i < batch.size; i++) {
    logger_log(" %" PRIu64 " ", arr[i]);
  }
  logger_log("]\n");
#endif

  return batch;
}
uint64_t file_controller_get_current() { return this_at(this.images.len - 1); }
uint64_t file_controller_get_next_index() {
  return this.images.len == 0 ? 1 : file_controller_get_current() + 1;
}
file file_controller_open_image_by_idx(const char *const idx,
                                       const char *const path) {

  size_t position = 0;
#warning "check for overflow"
  uint64_t idx_as_num = strtoull(idx, nullptr, 10);

  bool idx_is_registered = file_controller_find(idx_as_num, &position);
  return !idx_is_registered ? (file){0} : open_img_at(idx, path);
}
