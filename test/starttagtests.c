#include <string.h> // strcmp

#include "test.h"

#define ASSERT_START_TAG(arena, xml, tag, ...)                                         \
  if (!assert_start_tag(                                                               \
          arena,                                                                       \
          xml,                                                                         \
          tag,                                                                         \
          (st_attr_t[]){                                                               \
              __VA_ARGS__,                                                             \
              {0},                                                                     \
          })) {                                                                        \
    FAIL("ASSERT_START_TAG(arena, \"" #xml "\", \"" #tag "\", \"" #__VA_ARGS__ "\")"); \
  }

typedef struct st_attr_t {
  const char *name;
  const char *value;
} st_attr_t;

static bool has_attr(const sax_parser_t *restrict parser, const char *restrict name) {

  for (const sax_attr_t *restrict attr = sax_attrs(parser); attr != NULL; attr = attr->next) {
    if (strcmp(name, attr->name) == 0) {
      return true;
    }
  }

  return false;
}

static bool assert_start_tag(
    arena_t *restrict arena,
    const char *restrict xml,
    const char *restrict tag,
    const st_attr_t *restrict attrs) {

  arena_reset(arena);
  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .string = xml,
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
  });

  sax_event_t ev = sax_next(parser);
  if (SAX_EVENT_START_TAG != ev) {
    TEST_LOG("%d != %d; error: \"%s\"", SAX_EVENT_START_TAG, ev, sax_error(parser));
    return false;
  }

  if (strcmp(tag, sax_tag(parser)) != 0) {
    TEST_LOG("\"%s\" != \"%s\"", tag, sax_tag(parser));
    return false;
  }

  if ((attrs[0].name == NULL) && (sax_attrs(parser) != NULL)) {
    TEST_LOG("Unexpected attr \"%s\"", sax_attrs(parser)->name);
    return false;
  }

  for (const st_attr_t *restrict attr = attrs; attr->name != NULL; ++attr) {
    if (attr->value == NULL) {
      if (!has_attr(parser, attr->name)) {
        TEST_LOG("Missing attribute \"%s\"", attr->name);
        return false;
      }
    } else if (strcmp(attr->value, sax_attr(parser, attr->name)) != 0) {
      TEST_LOG("\"%s\" != \"%s\"", attr->value, sax_attr(parser, attr->name));
      return false;
    }
  }

  sax_free(parser);
  return SAX_EVENT_END_DOCUMENT == sax_next(parser);
}

TEST(start_tag) {

  ASSERT_START_TAG(arena, "<xml version=\"1.0\" encoding=\"UTF-8\">", "xml", {"version", "1.0"}, {"encoding", "UTF-8"});
  ASSERT_START_TAG(arena, "<alpha beta=\"gamma\" delta>", "alpha", {"beta", "gamma"}, {"delta", NULL});
  ASSERT_START_TAG(arena, "<alpha beta=\"gamma\" delta >", "alpha", {"beta", "gamma"}, {"delta", NULL});
  ASSERT_START_TAG(arena, "<alpha beta gamma=\"delta\" >", "alpha", {"beta", NULL}, {"gamma", "delta"});
  ASSERT_START_TAG(arena, "<alpha>", "alpha", {0});
  ASSERT_START_TAG(arena, "<alpha >", "alpha", {0});

  ASSERT_START_TAG(arena, "<alpha beta=\"foo\">", "alpha", {"beta", "foo"});
  ASSERT_START_TAG(arena, "<alpha beta=\"\tfoo\">", "alpha", {"beta", "\tfoo"});
  ASSERT_START_TAG(arena, "<alpha beta=\"foo\n\">", "alpha", {"beta", "foo\n"});
  ASSERT_START_TAG(arena, "<alpha beta=\"\t foo \n\">", "alpha", {"beta", "\t foo \n"});
  ASSERT_START_TAG(arena, "<alpha beta=\"中\">", "alpha", {"beta", "中"});
  ASSERT_START_TAG(arena, "<alpha beta=\"\t中\">", "alpha", {"beta", "\t中"});
  ASSERT_START_TAG(arena, "<alpha beta=\"中\n\">", "alpha", {"beta", "中\n"});
  ASSERT_START_TAG(arena, "<alpha beta=\"\t 中 \n\">", "alpha", {"beta", "\t 中 \n"});
  ASSERT_START_TAG(arena, "<alpha beta=\"&#128512;\">", "alpha", {"beta", "😀"});
  ASSERT_START_TAG(arena, "<alpha beta=\"\t&#128512;\">", "alpha", {"beta", "\t😀"});
  ASSERT_START_TAG(arena, "<alpha beta=\"&#128512;\n\">", "alpha", {"beta", "😀\n"});
  ASSERT_START_TAG(arena, "<alpha beta=\"\t &#128512; \n\">", "alpha", {"beta", "\t 😀 \n"});

  ASSERT_XML_ERR(arena, "<>", "Unexpected character '>' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "< >", "Unexpected character ' ' located on line 1 column 2");
}