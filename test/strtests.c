#include "test.h"

bool sax_str_eq(const char *restrict lhs, const char *restrict rhs);
bool sax_startswith(const char *restrict subject, const char *restrict prefix);

TEST(str_eq) {

  (void)arena;

  ASSERT_FALSE(sax_str_eq(NULL, "alpha"));
  ASSERT_FALSE(sax_str_eq("alpha", NULL));
  ASSERT(sax_str_eq(NULL, NULL));
}

TEST(startswith) {

  (void)arena;

  ASSERT_FALSE(sax_startswith(NULL, "alpha"));
  ASSERT_FALSE(sax_startswith("alpha", NULL));
}