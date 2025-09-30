#include "test.h"

bool sax_str_eq(const char *restrict lhs, const char *restrict rhs);
bool sax_startswith(const char *restrict subject, const char *restrict prefix);
bool sax_endswith(const char *restrict subject, const char *restrict suffix);

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

TEST(endswith) {

  (void)arena;

  ASSERT(sax_endswith(NULL, NULL));
  ASSERT_FALSE(sax_endswith(NULL, "alpha"));
  ASSERT(sax_endswith("alpha", NULL));
  ASSERT(sax_endswith("alpha", ""));
}