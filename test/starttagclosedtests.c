#include <string.h> // strcmp

#include "test.h"

#define ASSERT_START_TAG(arena, xml, tag, ...) \
  assert_start_tag(                            \
      arena,                                   \
      xml,                                     \
      tag,                                     \
      (st_attr_t[]){                           \
          __VA_ARGS__,                         \
          {0},                                 \
      });

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

static void assert_start_tag(
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

  ASSERT_EQ(SAX_EVENT_START_TAG, sax_next(parser));
  ASSERT_STR_EQ(tag, sax_tag(parser));

  if (attrs[0].name == NULL) {
    ASSERT_NULL(sax_attrs(parser));
  }

  for (const st_attr_t *restrict attr = attrs; attr->name != NULL; ++attr) {
    if (attr->value == NULL) {
      ASSERT(has_attr(parser, attr->name));
    } else {
      ASSERT_STR_EQ(attr->value, sax_attr(parser, attr->name));
    }
  }

  ASSERT_EQ(SAX_EVENT_END_TAG, sax_next(parser));
  ASSERT_STR_EQ(tag, sax_tag(parser));
  ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));

  sax_free(parser);
}

TEST(start_tag_closed) {

  ASSERT_START_TAG(arena, "<xml version=\"1.0\" encoding=\"UTF-8\"/>", "xml", {"version", "1.0"}, {"encoding", "UTF-8"});
  ASSERT_START_TAG(arena, "<alpha beta=\"gamma\" delta/>", "alpha", {"beta", "gamma"}, {"delta", NULL});
  ASSERT_START_TAG(arena, "<alpha beta=\"gamma\" delta />", "alpha", {"beta", "gamma"}, {"delta", NULL});
  ASSERT_START_TAG(arena, "<alpha beta gamma=\"delta\" />", "alpha", {"beta", NULL}, {"gamma", "delta"});
  ASSERT_START_TAG(arena, "<alpha/>", "alpha", {0});
  ASSERT_START_TAG(arena, "<alpha />", "alpha", {0});

  ASSERT_START_TAG(arena, "<alpha beta=\"foo\"/>", "alpha", {"beta", "foo"});
  ASSERT_START_TAG(arena, "<alpha beta=\"\tfoo\"/>", "alpha", {"beta", "\tfoo"});
  ASSERT_START_TAG(arena, "<alpha beta=\"foo\n\"/>", "alpha", {"beta", "foo\n"});
  ASSERT_START_TAG(arena, "<alpha beta=\"\t foo \n\"/>", "alpha", {"beta", "\t foo \n"});
  ASSERT_START_TAG(arena, "<alpha beta=\"中\"/>", "alpha", {"beta", "中"});
  ASSERT_START_TAG(arena, "<alpha beta=\"\t中\"/>", "alpha", {"beta", "\t中"});
  ASSERT_START_TAG(arena, "<alpha beta=\"中\n\"/>", "alpha", {"beta", "中\n"});
  ASSERT_START_TAG(arena, "<alpha beta=\"\t 中 \n\"/>", "alpha", {"beta", "\t 中 \n"});
  ASSERT_START_TAG(arena, "<alpha beta=\"&#128512;\"/>", "alpha", {"beta", "😀"});
  ASSERT_START_TAG(arena, "<alpha beta=\"\t&#128512;\"/>", "alpha", {"beta", "\t😀"});
  ASSERT_START_TAG(arena, "<alpha beta=\"&#128512;\n\"/>", "alpha", {"beta", "😀\n"});
  ASSERT_START_TAG(arena, "<alpha beta=\"\t &#128512; \n\"/>", "alpha", {"beta", "\t 😀 \n"});

  ASSERT_XML_ERR(arena, "</>", "Unexpected character '>' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "< />", "Unexpected character ' ' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "< //>", "Unexpected character ' ' located on line 1 column 3");
}