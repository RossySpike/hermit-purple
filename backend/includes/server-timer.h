#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>
typedef int timer;
timer server_timer_create();
bool server_timer_start(timer timer, uint64_t seconds, uint64_t nanoseconds);
int server_timer_get_fd(timer timer);
bool server_timer_stop(timer timer);
bool server_timer_close(timer timer);
bool server_timer_has_expired(timer timer);

#endif // TIMER_H
