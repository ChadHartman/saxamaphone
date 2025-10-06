#include "test.h"
#include <saxamaphone.h>

static const char *xml_parse_content(
    arena_t *restrict arena,
    const char *restrict content,
    bool untrimmed) {

  char xml[1024];
  snprintf(xml, sizeof(xml), "<content>%s</content>", content);

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = xml,
      .untrimmed_content = untrimmed,
  });

  // <content>
  ASSERT_EQ(SAX_EVENT_START_TAG, sax_next(parser));
  // ...
  ASSERT_EQ(SAX_EVENT_CONTENT, sax_next(parser));
  return sax_content(parser);
}

TEST(content) {
  ASSERT_STR_EQ("Foo Bar", xml_parse_content(arena, "   Foo Bar   \n", false));
  ASSERT_STR_EQ("   Foo Bar   \n", xml_parse_content(arena, "   Foo Bar   \n", true));
  ASSERT_STR_EQ("😀", xml_parse_content(arena, "&#x1f600;", false));
  ASSERT_STR_EQ("🌸", xml_parse_content(arena, "    \t  &#x1f338;", false));
  ASSERT_STR_EQ("🎵", xml_parse_content(arena, "&#x1f3b5;    \t  \n", false));
  ASSERT_STR_EQ("🚀", xml_parse_content(arena, "\t   \r\n   &#x1f680;  \t  \n", false));
  ASSERT_XML_SEQ(arena, "<foo>bar", SAX_EVENT_START_TAG, SAX_EVENT_ERROR);
}