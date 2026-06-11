#ifndef FILES_H
#define FILES_H

#include <stddef.h>
#include <sys/types.h>
typedef struct F {
  int fd;
  size_t size;
} file;
int open_file(file *f, const char *r_path, int flag);
int close_file(file *f);
unsigned long long get_biggest_index(const char *path);
file open_img_at(const char *id, const char *path);
unsigned long long *open_files_to_arr(const char *path, file *const out,
                                      unsigned short *const n_files,
                                      unsigned long long start_file_id);
int attempt_create_dir(const char *path, mode_t mode);
#endif // FILES_H
