#include <test.h>

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

TEST(parser) {

  // Confirm noop
  sax_parser_free(NULL);
  ASSERT_NULL(sax_parser(NULL));
  test_parser_missing_file(arena);
}
