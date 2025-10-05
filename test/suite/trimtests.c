#include <test.h>

char *sax_strcpy(char *restrict dest, size_t dest_size, const char *restrict src);
char *sax_trim(char *str);

TEST(trim) {

  (void)arena;

  char buf[1024];

  sax_strcpy(buf, 1024, "   Foo Bar   \n");
  ASSERT_STR_EQ("Foo Bar", sax_trim(buf));

  sax_strcpy(buf, 1024, "🎵");
  ASSERT_STR_EQ("🎵", sax_trim(buf));

  sax_strcpy(buf, 1024, "   😀   \n");
  ASSERT_STR_EQ("😀", sax_trim(buf));

  sax_strcpy(buf, 1024, "   🌸");
  ASSERT_STR_EQ("🌸", sax_trim(buf));

  sax_strcpy(buf, 1024, "🚀   \n");
  ASSERT_STR_EQ("🚀", sax_trim(buf));
}