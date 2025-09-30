#include "test.h"
#include <saxamaphone.h>

TEST(comment) {
  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .string = "<!-- <hello, world!> -->",
  });
  ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));
}