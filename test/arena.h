#ifndef ARENA_H
#define ARENA_H

#include <stdint.h> // uint8_t
#include <stdlib.h> // size_t

typedef struct arena_t arena_t;

arena_t *arena_create();

void *arena_alloc(arena_t *restrict arena, size_t bytes);

void *arena_copy(arena_t *restrict arena, const void *restrict src, size_t bytes);

void *arena_strdup(arena_t *restrict arena, const char *restrict src);

void arena_reset(arena_t *restrict arena);

void arena_free(arena_t *restrict arena);

void *arena_custom_alloc(void *ctx, void *ptr, size_t size);

#endif