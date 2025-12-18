#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h> // memset

#include "arena.h"

typedef struct ledger_item_t {
  uintptr_t address;
  size_t size;
} ledger_item_t;

typedef struct ledger_t {
  bool sorted;
  ledger_item_t *items;
  size_t count;
  size_t capacity;
} ledger_t;

struct arena_t {
  uint8_t *bytes;
  size_t size;
  size_t offset;

  ledger_t ledger;

  struct arena_t *upstream;
};

static int ledger_item_cmp(const void *a, const void *b) {
  const ledger_item_t *restrict lhs = a;
  const ledger_item_t *restrict rhs = b;
  return lhs->address == rhs->address ? 0 : (lhs->address < rhs->address ? -1 : 1);
}

static void ledger_free(ledger_t *restrict ledger) {

    const ledger_item_t *end = ledger->items + ledger->count;

  for (ledger_item_t *i = ledger->items; i != end; ++i) {
    if (i->size != 0) {
      fprintf(stderr, "Leak found with address %p sized %zu\n", (void *)i->address, i->size);
    }
  }

  free(ledger->items);
}

static void ledger_update(
    ledger_t *restrict ledger,
    void *restrict address,
    size_t size) {

  if (!ledger->sorted) {
    qsort(ledger->items, ledger->count, sizeof(ledger_item_t), ledger_item_cmp);
    ledger->sorted = true;
  }

  ledger_item_t *restrict found = bsearch(
      &(ledger_item_t){.address = (uintptr_t)address},
      ledger->items,
      ledger->count,
      sizeof(ledger_item_t),
      ledger_item_cmp);

  if (found) {
    found->size = size;
    return;
  }

  if (ledger->count == ledger->capacity) {
    ledger->capacity = ledger->capacity == 0 ? 32 : ledger->capacity * 2;
    ledger->items = realloc(ledger->items, ledger->capacity * sizeof(ledger_item_t));
    assert(ledger->items);
  }

  ledger->items[ledger->count++] = (ledger_item_t){
      .address = (uintptr_t)address,
      .size = size,
  };
}

static size_t arena_allocation_size(const void *restrict ptr) {

  if (ptr == NULL) {
    return 0;
  }

  const uint8_t *block = ptr;
  const size_t *block_size = (const void *)(block - sizeof(size_t));
  return *block_size;
}

arena_t *arena_create() {
  arena_t *restrict arena = calloc(1, sizeof(arena_t));
  assert(arena);
  return arena;
}

void *arena_alloc(arena_t *restrict arena, size_t bytes) {

  assert(arena);
  assert(bytes > 0);

  if (arena->offset + bytes + sizeof(size_t) <= arena->size) {
    uint8_t *restrict ptr = arena->bytes + arena->offset;
    memcpy(ptr, &bytes, sizeof(size_t));
    ptr += sizeof(size_t);
    arena->offset += bytes + sizeof(size_t);
    memset(ptr, 0, bytes);
    return ptr;
  }

  if (arena->upstream) {
    return arena_alloc(arena->upstream, bytes);
  }

  const size_t upstream_size = arena->size == 0 ? 1024 : arena->size * 2;
  arena->upstream = arena_create();
  *arena->upstream = (arena_t){
      .bytes = calloc(upstream_size, sizeof(uint8_t)),
      .size = upstream_size,
  };
  assert(arena->upstream->bytes);
  return arena_alloc(arena->upstream, bytes);
}

char *arena_strdup(arena_t *restrict arena, const char *restrict src) {

  if (src == NULL) {
    return NULL;
  }

  const size_t size = strlen(src) + 1;
  char *restrict copy = arena_alloc(arena, size);
  memcpy(copy, src, size);
  return copy;
}

void arena_reset(arena_t *restrict arena) {

  if (!arena) {
    return;
  }

  arena->offset = 0;
  arena_reset(arena->upstream);
}

size_t arena_size(const arena_t *restrict arena) {

  if (arena == NULL) {
    return 0;
  }

  return arena->size + arena_size(arena->upstream);
}

void arena_free(arena_t *restrict arena) {

  if (!arena) {
    return;
  }

  arena_free(arena->upstream);
  ledger_free(&arena->ledger);
  free(arena->bytes);
}

void *arena_custom_alloc(void *ctx, void *ptr, size_t size) {

  arena_t *restrict arena = ctx;

  if (size == 0) {
    ledger_update(&arena->ledger, ptr, size);
    return NULL;
  }

  if (ptr == NULL) {
    ptr = arena_alloc(ctx, size);
    ledger_update(&arena->ledger, ptr, size);
    return ptr;
  }

  const size_t block_size = arena_allocation_size(ptr);
  if (block_size >= size) {
    return ptr;
  }

  ledger_update(&arena->ledger, ptr, 0);
  void *new_ptr = arena_alloc(ctx, size);
  ledger_update(&arena->ledger, new_ptr, size);
  memcpy(new_ptr, ptr, block_size);
  return new_ptr;
}