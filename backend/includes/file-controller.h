#ifndef FILE_CONTROLLER_H
#define FILE_CONTROLLER_H
#include "./list.h"
#include "files.h"
#include <stddef.h>
#include <stdint.h>
typedef struct {
  list_t images;
  bool ready;
} file_controller;
typedef struct {
  uint64_t *batch;
  size_t size;
} batch_t;

uint64_t file_controller_get_length(void);
uint64_t file_controller_get_current();
uint64_t file_controller_get_next_index();
bool file_controller_init();
void file_controller_record_file(uint64_t id);
batch_t file_controller_get_batch(uint64_t start_id, size_t size);
/*
 * if the return value is file.fd == 0 and ERRNO = EDOM || EAGAIN
 * the it failed
 * */
file file_controller_open_image_by_idx(const char *idx, const char *path);
#endif // FILE_CONTROLLER_H
