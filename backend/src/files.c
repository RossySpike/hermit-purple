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
