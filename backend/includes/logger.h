#ifndef LOGGER_H
#define LOGGER_H
#include "server-defines.h"
bool logger_init();
bool logger_destroy();
void logger_log(log_level_t level, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

#endif // LOGGER_H
