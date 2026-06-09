#include "../includes/defines.h" // for fperror

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
typedef struct F {
  int fd;
  size_t size;
} file;
int open_file(file *f, const char *r_path, int flag) {
  f->fd = open(r_path, flag);
  if (f->fd < 0) {
    fperror;
    return -1;
  }
  struct stat fs;
  if (fstat(f->fd, &fs) == -1) {
    fperror;
    close(f->fd);
    f->fd = -1;
    return -1;
  }
  f->size = (size_t)fs.st_size;
  return 0;
}

int close_file(file *f) {
  if (close(f->fd) < 0) {
    fperror;
    return -1;
  }
  f->fd = -1; // Reset fd to indicate that the file is closed
  return 0;
}
int attempt_create_dir(const char *path, mode_t mode) {
  printf("DEBUG: %s\n", __func__);
  int res = mkdir(path, mode);
  if (res == -1) {
    printf("DEBUG: errno:`%d`. strerror:`%s`\n", errno, strerror(errno));
    if (errno == EEXIST)
      return 1;
    return -1;
  }
  return 0;
}

unsigned long long get_biggest_index(const char *path) {
  printf("DEBUG: %s\n", __func__);
  DIR *dir = opendir(path); // ← Usar el path recibido
  struct dirent *entry;
  unsigned long long max = 0, num = 0;
  /* char *dot, nombre_sin_ext[256]; */

  assert(dir);

  while ((entry = readdir(dir))) {
    printf("DEBUG: readdir:`%s`\n", entry->d_name);
    // Ignorar "." y ".."
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
    printf("DEBUG: num:`%llu`\n", num);
    printf("DEBUG: max:`%llu`\n", max);
    if (num > max) {
      max = num;
    } else {

      num = max;
    }
  }

  printf("DEBUG: final num:`%llu`\n", num);
  closedir(dir);
  return num;
}

file open_img_at(const char *id, const char *path) {
  printf("DEBUG: %s\n", __func__);
  DIR *dir = opendir(path); // ← Usar el path recibido
  struct dirent *entry;
  unsigned long long max = 0, num = 0;
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

      break;
    }
  }
  closedir(dir);
  return f; // not found.
}
unsigned long long *open_files_to_arr(const char *path, file *const out,
                                      unsigned short *const n_files,
                                      unsigned long long start_file_id) {
  printf("DEBUG: open_files_to_arr\n");

  if (*n_files > start_file_id) {
    return nullptr; // No hay archivos para abrir el start_file_id es menor que
                    // n_files
  }
  unsigned short arr_idx = 0;
  unsigned long long *f_names_to_ull =
      malloc(sizeof(unsigned long long) * (*n_files + 1));

  bool should_exit = false;
  while (!should_exit && arr_idx < *n_files) {
    DIR *dir = opendir(path); // ← Usar el path recibido
    struct dirent *entry;
    assert(dir);
    const char curr_filename[21];
    (const char *)snprintf(curr_filename, sizeof(curr_filename), "%llu",
                           start_file_id - (arr_idx + 1));
    printf("DEBUG: Looking for file with name %s\n", curr_filename);
    printf("DEBUG: arr_idx = %hu\n", arr_idx);
    printf("DEBUG: start_file_id = %llu\n", start_file_id);
    printf("DEBUG: n_files = %hu\n", *n_files);
    printf("start_file_id - (arr_idx + 1): %llu\n",
           start_file_id - (arr_idx + 1));

    /* && entry != nullptr && arr_idx != *n_files */
    while ((entry = readdir(dir))) {
      size_t i = 0;
      for (; (entry->d_name[i] != '\0' && isdigit(entry->d_name[i])); i++)
        ;

      if (i > 0 && strncmp(entry->d_name, curr_filename, i) == 0) {
        printf("DEBUG: Found file with name %s\n", entry->d_name);
        char b[BUFFER] = {0};
        snprintf(b, sizeof(b), "%s%s", path, entry->d_name);
        open_file(&out[arr_idx++], b, O_RDONLY);
        f_names_to_ull[arr_idx] = arr_idx + start_file_id;
        break;
      } else {
        printf("DEBUG: File with name %s not found\n", curr_filename);
      }
    }
    should_exit = entry == nullptr;
    closedir(dir);
  }
  *n_files = arr_idx;
  f_names_to_ull[arr_idx] = 0; // Mark the end of the array with a 0
  return arr_idx == 0 ? nullptr : f_names_to_ull;
}
