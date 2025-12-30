#include <stdlib.h> // malloc, realloc, free

#include "saxalloc.h"

void *sax_system_alloc(void *ctx, void *ptr, size_t size) {

  (void)ctx;

  if (size == 0) {
    free(ptr);
    return NULL;
  }

  if (ptr == NULL) {
    return malloc(size);
  }

  return realloc(ptr, size);
}