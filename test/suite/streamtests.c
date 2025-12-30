#include <saxstream.h>
#include <test.h>

static void test_stream_nullalloc(void) {
  ASSERT_NULL(sax_stream(&(sax_stream_config_t){
      .alloc = test_null_alloc,
  }));
}

static void test_stream_str(arena_t *restrict arena) {

  sax_stream_t *restrict stream = sax_stream(&(sax_stream_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
  });

  ASSERT_FALSE(sax_stream_append(NULL, NULL));
  ASSERT_FALSE(sax_stream_append(stream, NULL));

  const bool res = sax_stream_append(stream, "<hello>%s</hello>", "world!");
  ASSERT(res);
  ASSERT_NULL(sax_stream_str(NULL));
  ASSERT_STR_EQ("<hello>world!</hello>", sax_stream_str(stream));

  sax_stream_free(NULL);
  sax_stream_free(stream);
}

static void test_stream_realloc(arena_t *restrict arena) {

  sax_stream_t *restrict stream = sax_stream(&(sax_stream_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .buf_size = 4,
  });

  const bool res = sax_stream_append(stream, "<hello>%s</hello>", "world!");
  ASSERT(res);
  ASSERT_STR_EQ("<hello>world!</hello>", sax_stream_str(stream));

  sax_stream_free(stream);
}

static void test_stream_stdout(arena_t *restrict arena) {

  sax_stream_t *restrict stream = sax_stream(&(sax_stream_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .file = stdout,
  });

  const bool res = sax_stream_append(stream, "<hello>%s</hello>\n", "world!");
  ASSERT(res);

  sax_stream_free(stream);
}

static void test_stream_str_nullconf(void) {

  sax_stream_t *restrict stream = sax_stream(NULL);

  const bool res = sax_stream_append(stream, "<hello>%s</hello>", "world!");
  ASSERT(res);
  ASSERT_STR_EQ("<hello>world!</hello>", sax_stream_str(stream));

  sax_stream_free(stream);
}

static void test_stream_str_zeroconf(void) {

  sax_stream_t *restrict stream = sax_stream(&(sax_stream_config_t){0});

  const bool res = sax_stream_append(stream, "<hello>%s</hello>", "world!");
  ASSERT(res);
  ASSERT_STR_EQ("<hello>world!</hello>", sax_stream_str(stream));

  sax_stream_free(stream);
}

TEST(stream) {
  test_stream_nullalloc();
  test_stream_realloc(arena);
  test_stream_stdout(arena);
  test_stream_str(arena);
  test_stream_str_nullconf();
  test_stream_str_zeroconf();
}