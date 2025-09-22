#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  // printf
#include <stdlib.h> // EXIT_SUCCESS
#include <string.h> // strrchr

#include "arena.h"
#include "test.h"
#include <saxamaphone.h>

#define TEST_DECL(test_name) test_##test_name(void)
#define TEST_REG(test_name) {.name = #test_name, .func = test_##test_name}

static void TEST_DECL(substr) {

  TEST("substr");
  const char *substr = NULL;
  size_t substr_size = 0;

  sax_str_substr("foobar", 1, 4, &substr, &substr_size);
  ASSERT_STRN_EQ("ooba", substr, substr_size);
  sax_str_substr("foobar", 1, UINT32_MAX, &substr, &substr_size);
  ASSERT_STRN_EQ("oobar", substr, substr_size);
  sax_str_substr("foobar", 1, 0, &substr, &substr_size);
  ASSERT_STRN_EQ("", substr, substr_size);
  sax_str_substr("foobar", 0, 4, &substr, &substr_size);
  ASSERT_STRN_EQ("foob", substr, substr_size);
  sax_str_substr("foobar", UINT32_MAX, 1, &substr, &substr_size);
  ASSERT_STRN_EQ("", substr, substr_size);
  sax_str_substr("", 1, 2, &substr, &substr_size);
  ASSERT_STRN_EQ("", substr, substr_size);

  sax_str_substr("Olá, Mun", 3, 5, &substr, &substr_size);
  ASSERT_STRN_EQ(", Mun", substr, substr_size);
  sax_str_substr("Γεια σου Κόσμε", 3, 9, &substr, &substr_size);
  ASSERT_STRN_EQ("α σου Κόσ", substr, substr_size);
  sax_str_substr("こんにちは世界", 3, 4, &substr, &substr_size);
  ASSERT_STRN_EQ("ちは世界", substr, substr_size);
}

const char *sax_str_unescape(const char *restrict src);

static void TEST_DECL(unescaped) {

  TEST("unescaped");
  ASSERT_STR_EQ("foo", sax_str_unescape("foo"));
  ASSERT_STR_EQ("<", sax_str_unescape("&lt;"));
  ASSERT_STR_EQ(">", sax_str_unescape("&gt;"));
  ASSERT_STR_EQ("&", sax_str_unescape("&amp;"));
  ASSERT_STR_EQ("'", sax_str_unescape("&apos;"));
  ASSERT_STR_EQ("\"", sax_str_unescape("&quot;"));
  ASSERT_STR_EQ("A", sax_str_unescape("&#65;"));
  ASSERT_STR_EQ("a", sax_str_unescape("&#97;"));
  ASSERT_STR_EQ("$", sax_str_unescape("&#36;"));
  ASSERT_STR_EQ("?", sax_str_unescape("&#63;"));
  ASSERT_STR_EQ("¡", sax_str_unescape("&#161;"));
  ASSERT_STR_EQ("µ", sax_str_unescape("&#181;"));
  ASSERT_STR_EQ("é", sax_str_unescape("&#233;"));
  ASSERT_STR_EQ("А", sax_str_unescape("&#1040;"));
  ASSERT_STR_EQ("€", sax_str_unescape("&#8364;"));
  ASSERT_STR_EQ("™", sax_str_unescape("&#8482;"));
  ASSERT_STR_EQ("中", sax_str_unescape("&#20013;"));
  ASSERT_STR_EQ("心", sax_str_unescape("&#24515;"));
  ASSERT_STR_EQ("😀", sax_str_unescape("&#128512;"));
  ASSERT_STR_EQ("🌸", sax_str_unescape("&#127800;"));
  ASSERT_STR_EQ("🎵", sax_str_unescape("&#127925;"));
  ASSERT_STR_EQ("🚀", sax_str_unescape("&#128640;"));
}

int main(int argc, char **args) {

  typedef struct test_t {
    const char *name;
    void (*func)(void);
  } test_t;

  const test_t tests[] = {
      TEST_REG(substr),
      TEST_REG(unescaped),
  };
  const size_t count = sizeof(tests) / sizeof(test_t);

  if (argc == 1) {
    for (size_t i = 0; i < count; ++i) {
      tests[i].func();
    }
    return EXIT_SUCCESS;
  }

  if (strcmp("-h", args[1]) == 0) {
    printf("Usage: %s [-h] [-l] [<test-name>]\n", args[0]);
    return EXIT_SUCCESS;
  }

  if (strcmp("-l", args[1]) == 0) {

    for (size_t i = 0; i < count; ++i) {
      printf("%s\n", tests[i].name);
    }
    return EXIT_SUCCESS;
  }

  for (size_t i = 0; i < count; ++i) {
    if (strcmp(tests[i].name, args[1]) == 0) {
      tests[i].func();
      return EXIT_SUCCESS;
    }
  }

  printf("Unknown test \"%s\"\n", args[1]);
  return EXIT_FAILURE;
}
