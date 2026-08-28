#ifndef FILE_CONTROLLER_H
#define FILE_CONTROLLER_H
#include "./list.h"
#include "files.h"
#include <stdint.h>
typedef struct {
  bool ready;
  list_t images;
} file_controller;
typedef struct {
  size_t size;
  uint64_t *batch;
} batch_t;

uint64_t file_controller_get_length(void);
uint64_t file_controller_get_current();
uint64_t file_controller_get_next_index();
void file_controller_init();
void file_controller_record_file(uint64_t id);
batch_t file_controller_get_batch(uint64_t start_id, size_t size);
file file_controller_open_image_by_idx(const char *const idx,
                                       const char *const path);
#endif // FILE_CONTROLLER_H
