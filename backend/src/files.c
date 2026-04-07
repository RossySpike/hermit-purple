#include "../includes/defines.h" // for fperror
#include <fcntl.h>               // for open() function
#include <sys/stat.h>            // for fstat() function
#include <unistd.h>              // for close() function
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
