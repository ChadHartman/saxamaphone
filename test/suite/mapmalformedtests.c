#include <saxmapper.h>
#include <test.h>

static void test_mapper_set_error(arena_t *restrict arena) {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = "<foo",
  });

  const sax_field_t schema = {0};
  char *err = NULL;
  char buf[128];
  ASSERT_FALSE(sax_decode(parser, &schema, buf, NULL, &err));
  ASSERT_STR_EQ("Unexpected termination at line 1 column 5", err);
  arena_custom_alloc(arena, err, 0);

  sax_parser_free(parser);
}

TEST(map_malformed) {
  test_mapper_set_error(arena);
}