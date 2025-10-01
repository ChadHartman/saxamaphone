#include <test.h>

typedef struct capped_allocator_t {
  arena_t *restrict arena;
  size_t current;
  size_t max;
} capped_allocator_t;

static void *capped_alloc(void *ctx, void *ptr, size_t size) {

  (void)ptr;

  capped_allocator_t *restrict allocator = ctx;
  if (allocator->max >= allocator->current + size) {
    allocator->current += size;
    return arena_alloc(allocator->arena, size);
  }

  return NULL;
}

static void test_parser_missing_file(arena_t *restrict arena) {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .path = "missing.xml",
  });

  ASSERT_NON_NULL(parser);
  ASSERT_STR_EQ("Failed to open \"missing.xml\"", sax_error(parser));
  ASSERT_EQ(SAX_EVENT_ERROR, sax_next(parser));

  sax_parser_free(parser);
}

static void test_parser_no_src(arena_t *restrict arena) {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
  });

  ASSERT_NON_NULL(parser);
  ASSERT_STR_EQ("No XML source provided", sax_error(parser));
  ASSERT_EQ(SAX_EVENT_ERROR, sax_next(parser));

  sax_parser_free(parser);
}

static void test_parser_null_alloc_parser() {

  capped_allocator_t allocator = {0};
  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = capped_alloc,
      .alloc_ctx = &allocator,
  });

  ASSERT_NULL(parser);
}

static void test_parser_null_alloc_arena(arena_t *restrict arena) {

  capped_allocator_t allocator = {
      .arena = arena,
      .max = 128,
  };

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = capped_alloc,
      .alloc_ctx = &allocator,
  });

  ASSERT_NON_NULL(parser);
  ASSERT_STR_EQ("Out of memory", sax_error(parser));

  sax_parser_free(parser);
}

static void test_parser_null_alloc_file_buf(arena_t *restrict arena) {

  capped_allocator_t allocator = {
      .arena = arena,
      .max = 256,
  };

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = capped_alloc,
      .alloc_ctx = &allocator,
      .arena_size = 128,
      .path = "../test/files/xml-well-formed.xml",
  });

  ASSERT_NON_NULL(parser);
  ASSERT_STR_EQ("Out of memory", sax_error(parser));

  sax_parser_free(parser);
}

TEST(parser) {

  // Confirm noop
  sax_parser_free(NULL);
  ASSERT_NULL(sax_parser(NULL));
  test_parser_missing_file(arena);
  test_parser_no_src(arena);
  test_parser_null_alloc_parser();
  test_parser_null_alloc_arena(arena);
  test_parser_null_alloc_file_buf(arena);
}
