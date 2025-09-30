#include "test.h"

#define ASSERT_END_TAG(arena, xml, tag)                        \
  if (!assert_end_tag(arena, xml, tag)) {                      \
    FAIL("ASSERT_END_TAG(arena, \"" #xml "\", \"" #tag "\")"); \
  }

static bool assert_end_tag(
    arena_t *restrict arena,
    const char *restrict xml,
    const char *restrict tag) {

  arena_reset(arena);
  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .string = xml,
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
  });

  sax_event_t ev = sax_next(parser);
  if (SAX_EVENT_END_TAG != ev) {
    TEST_LOG("%d != %d; error: \"%s\"", SAX_EVENT_END_TAG, ev, sax_error(parser));
    return false;
  }

  if (strcmp(tag, sax_tag(parser)) != 0) {
    TEST_LOG("\"%s\" != \"%s\"", tag, sax_tag(parser));
    return false;
  }

  const bool pass = SAX_EVENT_END_DOCUMENT == sax_next(parser);
  sax_free(parser);
  return pass;
}

TEST(end_tag) {
  ASSERT_END_TAG(arena, "</alpha>", "alpha");
  ASSERT_END_TAG(arena, "</alpha\n>", "alpha");
}