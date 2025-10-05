#include "test.h"
#include <saxamaphone.h>

TEST(comment) {

  {
    sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
        .alloc = arena_custom_alloc,
        .alloc_ctx = arena,
        .xml = "<!-- <hello, world!> -->",
    });
    ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));
    sax_parser_free(parser);
  }

  // ASSERT_XML_ERR(arena, "<!?", "Unexpected character '?' located on line 1 column 3");
    {                                                             \
    arena_reset(arena);                                         \
    sax_parser_t *restrict parser = sax_parser(&(sax_config_t){ \
        .xml = "<!?",                                          \
        .alloc = arena_custom_alloc,                            \
        .alloc_ctx = arena,                                     \
    });                                                         \
    ASSERT_EQ(SAX_EVENT_ERROR, sax_next(parser));               \
    if ("Unexpected character '?' located on line 1 column 3" != NULL) {                                     \
      ASSERT_STR_EQ("Unexpected character '?' located on line 1 column 3", sax_error(parser));               \
    }                                                           \
    sax_parser_free(parser);                                    \
  }
  ASSERT_XML_ERR(arena, "<!/", "Unexpected character '/' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "<!!", "Unexpected character '!' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "<!-foo", "Unexpected character 'f' located on line 1 column 4");
}