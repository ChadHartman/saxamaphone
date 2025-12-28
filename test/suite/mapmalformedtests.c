#include <saxmapper.h>
#include <test.h>

static bool false_decoder(const sax_decode_ctx_t *restrict ctx, void *value) {
  (void)ctx;
  (void)value;
  return false;
}

static void test_mapper_decode_attr_returns_false(arena_t *restrict arena) {

  const sax_field_t foo_schema[] = {{.name = "bar", .type = SAX_TYPE_UINT8}, {0}};
  const sax_field_t doc_schema[] = {{.name = "foo", .schema = foo_schema}, {0}};
  typedef struct foo_t {
    uint8_t bar;
  } foo_t;

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = "<foo bar=\"42\"/>",
  });

  foo_t foo = {0};
  char *err = NULL;
  ASSERT_FALSE(sax_decode(
      parser,
      doc_schema,
      &foo,
      &(sax_map_opts_t){
          .decoders[SAX_TYPE_UINT8] = false_decoder,
      },
      &err));
  ASSERT_STR_EQ("Failed to set \"bar\" with \"42\"; decoder returned false", err);
  arena_custom_alloc(arena, err, 0);

  sax_parser_free(parser);
}

static void test_mapper_decode_content_returns_false(arena_t *restrict arena) {

  typedef struct foo_t {
    uint8_t bar;
  } foo_t;
  const sax_field_t foo_schema[] = {{.name = SAX_CONTENT, .type = SAX_TYPE_UINT8, .offset = offsetof(foo_t, bar)}, {0}};
  const sax_field_t doc_schema[] = {{.name = "foo", .schema = foo_schema}, {0}};

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = "<foo>42</foo>",
  });

  foo_t foo = {0};
  char *err = NULL;
  ASSERT_FALSE(sax_decode(
      parser,
      doc_schema,
      &foo,
      &(sax_map_opts_t){
          .decoders[SAX_TYPE_UINT8] = false_decoder,
      },
      &err));
  ASSERT_STR_EQ("Failed to set \"@content\" with \"42\"; decoder returned false", err);
  arena_custom_alloc(arena, err, 0);

  sax_parser_free(parser);
}

static void test_mapper_no_decoder(arena_t *restrict arena) {
  const sax_field_t foo_schema[] = {{.name = "bar", .type = 63}, {0}};
  const sax_field_t doc_schema[] = {{.name = "foo", .schema = foo_schema}, {0}};
  typedef struct foo_t {
    uint8_t bar;
  } foo_t;

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = "<foo bar=\"42\"/>",
  });

  foo_t foo = {0};
  char *err = NULL;
  ASSERT_FALSE(sax_decode(parser, doc_schema, &foo, NULL, &err));
  ASSERT_STR_EQ("No decoder set for attribute \"bar\" with value \"42\"", err);
  arena_custom_alloc(arena, err, 0);

  sax_parser_free(parser);
}

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

static void test_mapper_decode_malformed_no_error(arena_t *restrict arena) {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = "<foo",
  });

  const sax_field_t schema = {0};
  char buf[128];
  ASSERT_FALSE(sax_decode(parser, &schema, buf, NULL, NULL));

  sax_parser_free(parser);
}

TEST(map_malformed) {
  test_mapper_no_decoder(arena);
  test_mapper_decode_attr_returns_false(arena);
  test_mapper_decode_content_returns_false(arena);
  test_mapper_set_error(arena);
  test_mapper_decode_malformed_no_error(arena);
}