#ifndef API_IMAGE_H
#define API_IMAGE_H
#include "./defines.h"
#include "./server-machine.h"
void *api_jump_table(stream_cursor *cursor, server_machine *machine,
                     char *read_buffer);
#endif
