#ifndef LOGGER_H
#define LOGGER_H
bool logger_init();
bool logger_destroy();
void logger_log(const char *fmt, ...);

#endif // LOGGER_H
