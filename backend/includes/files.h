#ifndef FILES_H
#define FILES_H
#include "../src/files.c"
int open_file(file *f, const char *r_path, int flag);
int close_file(file *f);
unsigned long long get_biggest_index(const char *path);
#endif // FILES_H
