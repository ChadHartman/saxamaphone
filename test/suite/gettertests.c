#include <test.h>

TEST(getter) {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = "<foo/>",
  });

  ASSERT_NULL(sax_error(NULL));
  ASSERT_NULL(sax_error(parser));
  ASSERT_NULL(sax_tag(NULL));
  ASSERT_NULL(sax_content(NULL));
  ASSERT_NULL(sax_attrs(NULL));

  sax_parser_free(parser);
}