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
      .xml = xml,
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

  sax_parser_free(parser);
}

static void assert_xml_events(
    arena_t *restrict arena,
    const char *restrict xml,
    const uint8_t *restrict expected) {

  arena_reset(arena);
  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .xml = xml,
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
  });

  for (const uint8_t *restrict ex = expected; *ex != 0; ++ex) {
    ASSERT_EQ(*ex, sax_next(parser));
  }

  sax_parser_free(parser);
}

static const char *xml_parse_attr_val(
    arena_t *restrict arena,
    const char *restrict attr_val) {

  char xml[1024];
  snprintf(xml, sizeof(xml), "<content name=\"%s\"/>", attr_val);

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .xml = xml,
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
  });
  sax_event_t ev = sax_next(parser);

  if (ev == SAX_EVENT_ERROR) {
    printf("%s\n", sax_error(parser));
    return NULL;
  }

  ASSERT_EQ(SAX_EVENT_START_TAG, ev);
  ASSERT_STR_EQ("content", sax_tag(parser));

  const char *restrict attr = arena_strdup(arena, sax_attr(parser, "name"));
  sax_parser_free(parser);
  return attr;
}

TEST(attr_val) {
  ASSERT_STR_EQ("foo", xml_parse_attr_val(arena, "foo"));
  ASSERT_STR_EQ("\tfoo", xml_parse_attr_val(arena, "\tfoo"));
  ASSERT_STR_EQ("foo\n", xml_parse_attr_val(arena, "foo\n"));
  ASSERT_STR_EQ("\t foo \n", xml_parse_attr_val(arena, "\t foo \n"));
  ASSERT_STR_EQ("中", xml_parse_attr_val(arena, "中"));
  ASSERT_STR_EQ("\t中", xml_parse_attr_val(arena, "\t中"));
  ASSERT_STR_EQ("中\n", xml_parse_attr_val(arena, "中\n"));
  ASSERT_STR_EQ("\t 中 \n", xml_parse_attr_val(arena, "\t 中 \n"));
  ASSERT_STR_EQ("😀", xml_parse_attr_val(arena, "&#128512;"));
  ASSERT_STR_EQ("\t😀", xml_parse_attr_val(arena, "\t&#128512;"));
  ASSERT_STR_EQ("😀\n", xml_parse_attr_val(arena, "&#128512;\n"));
  ASSERT_STR_EQ("\t 😀 \n", xml_parse_attr_val(arena, "\t &#128512; \n"));
  ASSERT_XML_ERR(arena, "<alpha beta)=\"gamma\">", "Unexpected character ')' located on line 1 column 12");
}

TEST(start_tag_closed) {

  ASSERT_START_TAG(arena, "<xml version=\"1.0\" encoding=\"UTF-8\"/>", "xml", {"version", "1.0"}, {"encoding", "UTF-8"});
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

  ASSERT_XML_ERR(arena, "<alpha beta=\"gamma\" delta/>", "Unexpected character '/' located on line 1 column 26");
  ASSERT_XML_ERR(arena, "<alpha beta=\"gamma\" delta />", "Unexpected character ' ' located on line 1 column 26");
  ASSERT_XML_ERR(arena, "<alpha beta gamma=\"delta\" />", "Unexpected character ' ' located on line 1 column 12");
  ASSERT_XML_ERR(arena, "</>", "Unexpected character '>' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "< />", "Unexpected character ' ' located on line 1 column 2");

  assert_xml_events(arena, "<alpha //>", (uint8_t[]){SAX_EVENT_START_TAG, SAX_EVENT_ERROR, 0});
}