#ifndef ARENA_H
#define ARENA_H

#include <stdint.h> // uint8_t
#include <stdlib.h> // size_t

/// @brief Arena allocator instance
typedef struct arena_t arena_t;

/// @brief Create a new arena instance
/// @return new non-NULL instance
arena_t *arena_create();

/// @brief Allocate a block of memory using the arena
/// @param arena instance
/// @param bytes size of the contiguous allocation
/// @return non-NULL allocation
void *arena_alloc(arena_t *restrict arena, size_t bytes);

/// @brief Copy the provided string to a new allocation
/// @param arena instance
/// @param src string to copy
/// @return copied string or NULL if src was NULL
char *arena_strdup(arena_t *restrict arena, const char *restrict src);

/// @brief Invalidate all previously allocated pointers
/// @param arena instance
void arena_reset(arena_t *restrict arena);

/// @brief Free all arena allocations and the arena itelf
/// @param arena instance
void arena_free(arena_t *restrict arena);

/// @brief Retrieve the total number of bytes (regardless of allocations this
///   arena manages)
/// @param arena instance
/// @return number of bytes managed
size_t arena_size(const arena_t *restrict arena);

/// @brief Custom allocator utility
/// @param ctx arena instance
/// @param ptr to free/realloc
/// @param size size in bytes to malloc/realloc
/// @return malloc/realloc'd block or NULL if size is zero
void *arena_custom_alloc(void *ctx, void *ptr, size_t size);

#endif