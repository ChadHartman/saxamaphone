#include "test.h"
#include <saxamaphone.h>

TEST(cdata) {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = "<content>"
             "  Sample content: "
             "  <![CDATA[<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
             "    <body>Hello, world!</body>]]> (xml)"
             "</content>",
  });

  ASSERT_EQ(SAX_EVENT_START_TAG, sax_next(parser));
  ASSERT_STR_EQ("content", sax_tag(parser));

  ASSERT_EQ(SAX_EVENT_CONTENT, sax_next(parser));
  ASSERT_STR_EQ("Sample content:", sax_content(parser));

  ASSERT_EQ(SAX_EVENT_CONTENT, sax_next(parser));
  ASSERT_STR_EQ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                "    <body>Hello, world!</body>",
                sax_content(parser));

  ASSERT_EQ(SAX_EVENT_CONTENT, sax_next(parser));
  ASSERT_STR_EQ("(xml)", sax_content(parser));

  ASSERT_EQ(SAX_EVENT_END_TAG, sax_next(parser));
  ASSERT_STR_EQ("content", sax_tag(parser));

  ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));
}

TEST(cdata_malformed) {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml =
          "<content>"
          "  Sample content: "
          "  <![CDATA["
          "    <?xml version=\"1.0\" encoding=\"UTF-8\"?>"
          "    <body>Hello, world!</body>"
          "  ] ]> (xml)"
          "</content>",
  });

  ASSERT_EQ(SAX_EVENT_START_TAG, sax_next(parser));
  ASSERT_STR_EQ("content", sax_tag(parser));

  ASSERT_EQ(SAX_EVENT_CONTENT, sax_next(parser));
  ASSERT_STR_EQ("Sample content:", sax_content(parser));

  ASSERT_EQ(SAX_EVENT_ERROR, sax_next(parser));
  ASSERT_STR_EQ("Unexpected termination at line 1 column 133", sax_error(parser));
}