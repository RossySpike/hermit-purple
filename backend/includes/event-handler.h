#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H
#include <stddef.h>
#include <sys/epoll.h>
typedef struct event_handler_t event_handler_t;
typedef struct {

  size_t len;
  struct epoll_event *events;
} events_t;
event_handler_t *event_handler_init(size_t max_events);
bool event_handler_add_event(const event_handler_t *handler, int target_fd,
                             struct epoll_event event);
struct epoll_event event_handler_get_event(event_handler_t *handler);
bool event_handler_is_valid_epoll_event(struct epoll_event event);

bool event_handler_remove_event(event_handler_t *handler, int target_fd);
bool event_handler_mod(const event_handler_t *handler, int target_fd,
                       struct epoll_event *event);
void event_handler_destroy(event_handler_t **handler);

events_t event_handler_get_events(event_handler_t *handler);
int event_handler_get_num_events(event_handler_t *handler);
bool event_handler_del(const event_handler_t *handler, int target_fd);
#endif // EVENT_HANDLER_H
