#ifndef DEFINES_H
#define DEFINES_H
#include <errno.h>
#include <stdio.h>
#include <string.h>
#define fperror                                                                \
  fprintf(stderr, "[-] %s. errno: %d, %s\n", __func__, errno, strerror(errno))
#ifndef bzero
#define bzero(x, len_x)                                                        \
  { memset((x), 0, (len_x)); }
#endif // bzero

#endif // DEFINES_H
