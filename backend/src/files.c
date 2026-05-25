#include "../includes/defines.h" // for fperror
#include <dirent.h>
#include <fcntl.h> // for open() function
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> // for fstat() function
#include <unistd.h>   // for close() function
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

unsigned long long get_biggest_index(const char *path) {
  DIR *dir = opendir(path); // ← Usar el path recibido
  struct dirent *entry;
  unsigned long long max = 0, num;
  char *dot, nombre_sin_ext[256];

  if (!dir) {
    perror("opendir failed");
    return 1; // O crear el directorio si no existe
  }

  while ((entry = readdir(dir))) {
    // Ignorar "." y ".."
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    // Copiar el nombre con límite de seguridad
    strncpy(nombre_sin_ext, entry->d_name, sizeof(nombre_sin_ext) - 1);
    nombre_sin_ext[sizeof(nombre_sin_ext) - 1] = '\0';

    // Buscar el último punto (extensión)
    dot = strrchr(nombre_sin_ext, '.');
    if (dot)
      *dot = '\0'; // Cortar la extensión

    // Si el nombre está vacío después de quitar extensión, continuar
    if (nombre_sin_ext[0] == '\0')
      continue;

    // Convertir solo la parte numérica
    char *endptr;
    num = strtoull(nombre_sin_ext, &endptr, 10);

    // Verificar que TODO el nombre (sin extensión) sea número
    if (*endptr == '\0') {
      if (num > max)
        max = num;
      printf("%s -> %llu\n", entry->d_name, num);
    }
  }

  closedir(dir);
  return max;
}
