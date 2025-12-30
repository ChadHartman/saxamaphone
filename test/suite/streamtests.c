#include <saxstream.h>
#include <test.h>

static void test_stream_str(arena_t *restrict arena) {

  sax_stream_t *restrict stream = sax_stream(&(sax_stream_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
  });

  const bool res = sax_stream_append(stream, "<hello>%s</hello>", "world!");
  ASSERT(res);
  ASSERT_STR_EQ("<hello>world!</hello>", sax_stream_str(stream));

  sax_stream_free(stream);
}

TEST(stream) {
  test_stream_str(arena);
}