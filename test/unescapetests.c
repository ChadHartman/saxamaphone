#include "test.h"

const char *sax_unescape(const char *restrict src, char *restrict buf);

TEST(unescape) {

  (void)arena;

  char buf[5];
  ASSERT_STR_EQ("foo", sax_unescape("foo", buf));
  ASSERT_STR_EQ("<", sax_unescape("&lt;", buf));
  ASSERT_STR_EQ(">", sax_unescape("&gt;", buf));
  ASSERT_STR_EQ("&", sax_unescape("&amp;", buf));
  ASSERT_STR_EQ("'", sax_unescape("&apos;", buf));
  ASSERT_STR_EQ("\"", sax_unescape("&quot;", buf));

  ASSERT_NULL(sax_unescape(NULL, buf));
}

TEST(unescape10) {

  (void)arena;

  char buf[5];
  ASSERT_STR_EQ("&#0;", sax_unescape("&#0;", buf));
  ASSERT_STR_EQ("A", sax_unescape("&#65;", buf));
  ASSERT_STR_EQ("a", sax_unescape("&#97;", buf));
  ASSERT_STR_EQ("$", sax_unescape("&#36;", buf));
  ASSERT_STR_EQ("?", sax_unescape("&#63;", buf));
  ASSERT_STR_EQ("¡", sax_unescape("&#161;", buf));
  ASSERT_STR_EQ("µ", sax_unescape("&#181;", buf));
  ASSERT_STR_EQ("é", sax_unescape("&#233;", buf));
  ASSERT_STR_EQ("А", sax_unescape("&#1040;", buf));
  ASSERT_STR_EQ("€", sax_unescape("&#8364;", buf));
  ASSERT_STR_EQ("™", sax_unescape("&#8482;", buf));
  ASSERT_STR_EQ("中", sax_unescape("&#20013;", buf));
  ASSERT_STR_EQ("心", sax_unescape("&#24515;", buf));
  ASSERT_STR_EQ("😀", sax_unescape("&#128512;", buf));
  ASSERT_STR_EQ("🌸", sax_unescape("&#127800;", buf));
  ASSERT_STR_EQ("🎵", sax_unescape("&#127925;", buf));
  ASSERT_STR_EQ("🚀", sax_unescape("&#128640;", buf));

  ASSERT_NULL(sax_unescape("&#65;", NULL));
}

TEST(unescape16) {

  (void)arena;

  char buf[5];
  ASSERT_STR_EQ("&#x0;", sax_unescape("&#x0;", buf));
  ASSERT_STR_EQ("A", sax_unescape("&#x41;", buf));
  ASSERT_STR_EQ("a", sax_unescape("&#x61;", buf));
  ASSERT_STR_EQ("$", sax_unescape("&#x24;", buf));
  ASSERT_STR_EQ("?", sax_unescape("&#x3f;", buf));
  ASSERT_STR_EQ("¡", sax_unescape("&#xa1;", buf));
  ASSERT_STR_EQ("µ", sax_unescape("&#xb5;", buf));
  ASSERT_STR_EQ("é", sax_unescape("&#xe9;", buf));
  ASSERT_STR_EQ("А", sax_unescape("&#x410;", buf));
  ASSERT_STR_EQ("€", sax_unescape("&#x20ac;", buf));
  ASSERT_STR_EQ("™", sax_unescape("&#x2122;", buf));
  ASSERT_STR_EQ("中", sax_unescape("&#x4e2d;", buf));
  ASSERT_STR_EQ("心", sax_unescape("&#x5fc3;", buf));
  ASSERT_STR_EQ("😀", sax_unescape("&#x1f600;", buf));
  ASSERT_STR_EQ("🌸", sax_unescape("&#x1f338;", buf));
  ASSERT_STR_EQ("🎵", sax_unescape("&#x1f3b5;", buf));
  ASSERT_STR_EQ("🚀", sax_unescape("&#x1f680;", buf));
}