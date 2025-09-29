#include "test.h"

TEST(proc_inst) {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .string = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>",
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
  });

  ASSERT_EQ(SAX_EVENT_PROCESSING_INSTRUCTION, sax_next(parser));
  ASSERT_STR_EQ("xml", sax_tag(parser));
  ASSERT_STR_EQ("1.0", sax_attr(parser, "version"));
  ASSERT_STR_EQ("UTF-8", sax_attr(parser, "encoding"));
  ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));

  sax_free(parser);
}