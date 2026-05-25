#include <assert.h>
#include <stdlib.h>

typedef struct DYNAMIC_LIST_STRUCT {
  size_t capacity;
  size_t len;
  void **array;
} list_t;

void list_new(list_t *new_list) {
  new_list->capacity = 1;
  new_list->len = 0;
  new_list->array = malloc(sizeof(void *));
}

void list_grow(list_t *l) {
  l->capacity *= 2;
  l->array = realloc(l->array, l->capacity * sizeof(void *));
  assert(l->array != NULL);
}

void list_append(list_t *l, void *element) {
  if (l->len == l->capacity) {
    list_grow(l);
  }
  l->array[l->len++] = element;
}
int list_insert(list_t *l, void *element, int index) {
  if (0 == l->len || !l->array || index < -1 || index >= l->len) {
    return -1;
  }
  list_append(l, element);
  void *temp = l->array[l->len - 1];
  // Right shift
  for (int i = index; i < l->len - 1; i++) {
    l->array[i + 1] = l->array[i];
  }
  l->array[index] = temp;
  return 0;
}

void list_default_callback(void *element) {
  free(element);
  element = NULL;
}

int list_pop(list_t *l, void (*callback)(void *)) {
  if (0 == l->len || !l->array) {
    return -1;
  }
  l->len--;
  if (callback)
    callback(l->array[l->len]);
  l->array[l->len] = NULL;
  return 0;
}

void list_free(list_t *l, void (*callback)(void *)) {
  for (size_t i = 0; i < l->len; i++) {
    if (callback)
      callback(l->array[i]);
    l->array[i] = NULL;
  }
  free(l->array);
  l->array = NULL;
}

int list_remove(list_t *l, int index, void (*callback)(void *)) {
  if (0 == l->len || !l->array || index < -1 || index >= l->len) {
    return -1;
  }
  void *element = l->array[index];

  for (int i = index; i < l->len - 1; i++) {
    l->array[i] = l->array[i + 1];
  }

  if (callback)
    callback(element);

  l->len--;
  return 0;
}
