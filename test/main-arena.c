#include "arena.h"
#include "test.h"

int main() {

  arena_t *restrict arena = arena_create();

  size_t *alpha = arena_alloc(arena, sizeof(size_t));
  *alpha = 42;
  ASSERT_EQ(42, *alpha);

  size_t *beta = arena_alloc(arena, sizeof(size_t));
  *beta = 52;
  ASSERT_EQ(42, *alpha);
  ASSERT_EQ(52, *beta);

  arena_free(arena);
  return EXIT_SUCCESS;
}