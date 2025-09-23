#include "test.h"
#include <saxamaphone.h>
#include <stdlib.h>

typedef struct allocator_item_t {
  uintptr_t address;
  size_t size;
  bool live;
} allocator_item_t;

typedef struct allocator_t {
  allocator_item_t *items;
  size_t item_count;
} allocator_t;

static int allocator_item_cmp(const void *a, const void *b) {
  const allocator_item_t *lhs = a;
  const allocator_item_t *rhs = b;

  if (lhs->address < rhs->address) {
    return -1;
  }

  return lhs->address > rhs->address ? 1 : 0;
}

static void allocator_set(
    allocator_t *restrict alloc,
    void *restrict ptr,
    size_t size,
    bool live) {

  if (ptr == NULL) {
    return;
  }

  allocator_item_t item = {
      .address = (uintptr_t)ptr,
      .live = live,
      .size = size,
  };

  qsort(
      alloc->items,
      alloc->item_count,
      sizeof(allocator_item_t),
      allocator_item_cmp);

  allocator_item_t *restrict found = bsearch(
      &item,
      alloc->items,
      alloc->item_count,
      sizeof(allocator_item_t),
      allocator_item_cmp);

  if (found == NULL) {
    alloc->items = realloc(
        alloc->items,
        sizeof(allocator_item_t) * ++alloc->item_count);
    alloc->items[alloc->item_count - 1] = item;
  } else {
    // Only write size if non-zero
    item.size = item.size == 0 ? found->size : item.size;
    *found = item;
  }
}

static void *alloc(void *ctx, void *ptr, size_t size) {

  if (size == 0) {
    allocator_set(ctx, ptr, 0, false);
    free(ptr);
    return NULL;
  }

  if (ptr == NULL) {
    ptr = malloc(size);
    allocator_set(ctx, ptr, size, true);
    return ptr;
  }

  allocator_set(ctx, ptr, 0, false);
  ptr = realloc(ptr, size);
  allocator_set(ctx, ptr, size, true);
  return ptr;
}

static void allocator_report(const allocator_t *restrict alloc) {

  for (size_t i = 0; i < alloc->item_count; ++i) {
    const allocator_item_t *restrict item = &alloc->items[i];
    if (item->live) {
      FAIL("Found leak %p sized %zu", (void *)item->address, item->size);
    }
  }
}

int main() {

  allocator_t allocator = {0};
  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = alloc,
      .alloc_ctx = &allocator,
      .path = "../test/files/programming-languages.xml",
  });
  for (sax_event_t ev = sax_next(parser);
       ev != SAX_EVENT_END_DOCUMENT && ev != SAX_EVENT_ERROR;
       ev = sax_next(parser)) {
  }
  sax_free(parser);
  allocator_report(&allocator);

  return EXIT_SUCCESS;
}