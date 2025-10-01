#include "test.h"

size_t sax_file_buf_size(size_t capacity);

TEST(file_buf_size) {

  (void)arena;
  ASSERT_EQ(32, sax_file_buf_size(0));
  ASSERT_EQ(32, sax_file_buf_size(31));
  ASSERT_EQ(256, sax_file_buf_size(512));
  ASSERT_EQ(1024, sax_file_buf_size(4095));
}