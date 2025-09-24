#include <assert.h>
#include <string.h> // memset

#include "arena.h"

struct arena_t {
  uint8_t *bytes;
  size_t size;
  size_t offset;

  struct arena_t *upstream;
};

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

void *arena_copy(arena_t *restrict arena, const void *restrict src) {

  assert(arena);

  const size_t block_size = arena_allocation_size(src);
  if (block_size == 0) {
    return NULL;
  }

  void *restrict copy = arena_alloc(arena, block_size);
  memcpy(copy, src, block_size);
  return copy;
}

void arena_reset(arena_t *restrict arena) {

  if (!arena) {
    return;
  }

  arena->offset = 0;
  arena_reset(arena->upstream);
}

void arena_free(arena_t *restrict arena) {

  if (!arena) {
    return;
  }

  arena_free(arena->upstream);
  free(arena->bytes);
}

void *arena_custom_alloc(void *ctx, void *ptr, size_t size) {

  if (size == 0) {
    return NULL;
  }

  if (ptr == NULL) {
    return arena_alloc(ctx, size);
  }

  const size_t block_size = arena_allocation_size(ptr);
  if (block_size >= size) {
    return ptr;
  }

  void *new_ptr = arena_alloc(ctx, size);
  memcpy(new_ptr, ptr, block_size);
  return new_ptr;
}