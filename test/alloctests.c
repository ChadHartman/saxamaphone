#include "test.h"

typedef struct sax_arena_t sax_arena_t;

void *sax_default_alloc(void *ctx, void *ptr, size_t size);
void *sax_arena_alloc(sax_arena_t *restrict arena, size_t size);
sax_arena_t *sax_parser_arena(sax_parser_t *restrict parser);

TEST(default_alloc) {

  (void)arena;

  size_t *size = sax_default_alloc(NULL, NULL, sizeof(size_t));
  ASSERT_NON_NULL(size);

  *size = 42;
  size = sax_default_alloc(NULL, size, sizeof(size_t) * 2);
  ASSERT_EQ(42, *size);

  sax_default_alloc(NULL, size, 0);
}

TEST(arena_alloc) {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = "<foo/>",
      .arena_size = 8,
  });

  sax_arena_t *restrict sax_arena = sax_parser_arena(parser);

  ASSERT_NULL(sax_arena_alloc(NULL, 42));
  ASSERT_NULL(sax_arena_alloc(sax_arena, 0));
  // Expands beyond initial allocation
  ASSERT_NON_NULL(sax_arena_alloc(sax_arena, 16));

  sax_free(parser);
}