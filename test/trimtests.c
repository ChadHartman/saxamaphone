#include <test.h>

char *sax_trim(char *str);

TEST(trim) {

  (void)arena;

  char buf[1024];

  strcpy(buf, "   Foo Bar   \n");
  ASSERT_STR_EQ("Foo Bar", sax_trim(buf));

  strcpy(buf, "🎵");
  ASSERT_STR_EQ("🎵", sax_trim(buf));

  strcpy(buf, "   😀   \n");
  ASSERT_STR_EQ("😀", sax_trim(buf));

  strcpy(buf, "   🌸");
  ASSERT_STR_EQ("🌸", sax_trim(buf));

  strcpy(buf, "🚀   \n");
  ASSERT_STR_EQ("🚀", sax_trim(buf));
}