// You either love me or hate me
#include "logger.h"
#include "server-defines.h"
#include <string.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

typedef FILE *logger_t;
#define UNSET nullptr

static logger_t logger = UNSET;
#define this logger
#define is_set (this != UNSET)
#define log_errno_to_stderr(message)                                           \
  perror(message);                                                             \
  perror(strerror(errno));                                                     \
  perror("\n")

bool logger_init() {

#ifndef LOG_FILE
  const char *filename = "./hermit-purple.log";
#else
  const char *filename = LOG_FILE;

#endif
  logger = fopen(filename, "a");

  if (!logger) {
    log_errno_to_stderr("[LOG] FAILED TO CREATE LOG\n");
    return false;
  }
  // turns the stream into  line buffered
  // (_IOLBF) so when it hits \n it writes
  if (0 != setvbuf(logger, nullptr, _IOLBF, 0)) {
    log_errno_to_stderr("UNABLE TO TURN STREAM INTO LINE BUFFERED");
    return false;
  }
  switch (LOG_LEVEL) {
  case LOG_MEDIUM:
  case LOG_HIGH: {

    logger_log("[LOG] file created at: %s\n", filename);
    break;
  }
  default:
    break;
  }

  return true;
}

void logger_log(const char *fmt, ...) {
  assert(is_set);

  va_list args;
  va_start(args, fmt);
  if (vfprintf(logger, fmt, args) < 0) {
    log_errno_to_stderr("[LOG] UNABLE TO PRINT\n");
  }
  va_end(args);
}

bool logger_destroy() {
  assert(is_set);

  if (fclose(this) != 0) {
    log_errno_to_stderr("[LOG] FAILED TO CREATE LOG\n");
    return false;
  }

  return true;
}
