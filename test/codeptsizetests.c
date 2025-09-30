#include "test.h"

uint_fast8_t sax_code_pt_size(uint8_t byte);

TEST(code_pt_size) {

  (void)arena;

  ASSERT_EQ(2, sax_code_pt_size(0xC2));
  ASSERT_EQ(2, sax_code_pt_size(0xC3));
  ASSERT_EQ(2, sax_code_pt_size(0xDF));

  ASSERT_EQ(3, sax_code_pt_size(0xE0));
  ASSERT_EQ(3, sax_code_pt_size(0xEF));

  ASSERT_EQ(4, sax_code_pt_size(0xF0));
  ASSERT_EQ(4, sax_code_pt_size(0xF4));

  ASSERT_EQ(0, sax_code_pt_size(0x80));
  ASSERT_EQ(0, sax_code_pt_size(0xFF));
}