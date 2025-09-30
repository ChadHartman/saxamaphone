#include "test.h"

void *sax_default_alloc(void *ctx, void *ptr, size_t size);

TEST(alloc) {

  (void)arena;

  size_t *size = sax_default_alloc(NULL, NULL, sizeof(size_t));
  ASSERT_NON_NULL(size);

  *size = 42;
  size = sax_default_alloc(NULL, size, sizeof(size_t) * 2);
  ASSERT_EQ(42, *size);

  sax_default_alloc(NULL, size, 0);
}