#include <test.h>

TEST(attr) {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = "<foo/>",
  });

  ASSERT_NULL(sax_attr(NULL, NULL));
  ASSERT_NULL(sax_attr(parser, NULL));
  ASSERT_NULL(sax_attr(parser, "bar"));

  sax_parser_free(parser);
}