#include "test.h"

bool sax_str_eq(const char *restrict lhs, const char *restrict rhs);

TEST(str_eq) {

  (void)arena;

  ASSERT_FALSE(sax_str_eq(NULL, "alpha"));
  ASSERT_FALSE(sax_str_eq("alpha", NULL));
  ASSERT(sax_str_eq(NULL, NULL));
}
