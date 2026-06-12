#include "../includes/list.h"
#include <assert.h>
#include <stdlib.h>

void list_new(list_t *new_list) {
  new_list->capacity = 1;
  new_list->len = 0;
  new_list->array = malloc(sizeof(void *));
}

void list_grow(list_t *l) {
  l->capacity *= 2;
  l->array = realloc(l->array, l->capacity * sizeof(void *));
  assert(l->array != nullptr);
}

void list_append(list_t *l, void *element) {
  if (l->len == l->capacity) {
    list_grow(l);
  }
  l->array[l->len++] = element;
}
int list_insert(list_t *l, void *element, size_t index) {
  if (0 == l->len || !l->array || index >= l->len) {
    return -1;
  }
  list_append(l, element);
  void *temp = l->array[l->len - 1];
  // Right shift
  for (size_t i = index; i < l->len - 1; i++) {
    l->array[i + 1] = l->array[i];
  }
  l->array[index] = temp;
  return 0;
}

void list_default_callback(void *element) {
  if (!element)
    return;
  free(element);
  element = nullptr;
}

int list_pop(list_t *l, void (*callback)(void *)) {
  if (0 == l->len || !l->array) {
    return -1;
  }
  l->len--;
  if (callback)
    callback(l->array[l->len]);
  l->array[l->len] = nullptr;
  return 0;
}

void list_free_contents(list_t *l, void (*callback)(void *)) {
  if (!l)
    return;
  for (size_t i = 0; i < l->len; i++) {
    if (callback)
      callback(l->array[i]);
    l->array[i] = nullptr;
  }
}
void list_free(list_t *l, void (*callback)(void *)) {
  for (size_t i = 0; i < l->len; i++) {
    if (callback)
      callback(l->array[i]);
    l->array[i] = nullptr;
  }
  if (l->array != nullptr)
    free(l->array);
  l->array = nullptr;
  l->capacity = 0;
  l->len = 0;
}

int list_remove(list_t *l, size_t index, void (*callback)(void *)) {
  if (0 == l->len || !l->array || index >= l->len) {
    return -1;
  }
  void *element = l->array[index];

  for (size_t i = index; i < l->len - 1; i++) {
    l->array[i] = l->array[i + 1];
  }

  if (callback)
    callback(element);

  l->len--;
  return 0;
}
