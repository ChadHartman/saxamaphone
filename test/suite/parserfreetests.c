#include <test.h>

TEST(parser_free) {

  (void)arena;

  // Confirm noop
  sax_parser_free(NULL);
}
