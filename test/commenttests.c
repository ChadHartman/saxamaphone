#include "test.h"
#include <saxamaphone.h>

TEST(comment) {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = "<!-- <hello, world!> -->",
  });
  ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));

  ASSERT_XML_ERR(arena, "<!?", "Unexpected character '?' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "<!/", "Unexpected character '/' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "<!!", "Unexpected character '!' located on line 1 column 3");
}