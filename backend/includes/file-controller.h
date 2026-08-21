#ifndef FILE_CONTROLLER_H
#define FILE_CONTROLLER_H
#include "./list.h"
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
void file_controller_init();
void file_controller_record_file(uint64_t id);
batch_t file_controller_get_batch(uint64_t start_id, size_t size);
#endif // FILE_CONTROLLER_H
