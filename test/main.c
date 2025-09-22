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

// === forward declares === //

const char *sax_str_unescape(const char *restrict src, char *restrict buf);
char *sax_str_trim(char *str);

// === utilities === //

static const char *xml_parse_content(arena_t *restrict arena, const char *restrict content) {

  char xml[1024];
  snprintf(xml, sizeof(xml), "<content>%s</content>", content);

  sax_parser_t *parser = sax_parser(&(sax_config_t){
      .string = xml,
      .arena = arena_alloc(arena, 2048),
      .arena_size = 2048,
  });

  // <content>
  ASSERT_EQ(SAX_EVENT_START_ELEMENT, sax_next(parser));
  // ...
  ASSERT_EQ(SAX_EVENT_CONTENT, sax_next(parser));
  return sax_content(parser);
}

// === test cases === //

static void TEST_DECL(unescape) {

  char buf[5];
  ASSERT_STR_EQ("foo", sax_str_unescape("foo", buf));
  ASSERT_STR_EQ("<", sax_str_unescape("&lt;", buf));
  ASSERT_STR_EQ(">", sax_str_unescape("&gt;", buf));
  ASSERT_STR_EQ("&", sax_str_unescape("&amp;", buf));
  ASSERT_STR_EQ("'", sax_str_unescape("&apos;", buf));
  ASSERT_STR_EQ("\"", sax_str_unescape("&quot;", buf));
}

static void TEST_DECL(unescape10) {

  char buf[5];
  ASSERT_STR_EQ("A", sax_str_unescape("&#65;", buf));
  ASSERT_STR_EQ("a", sax_str_unescape("&#97;", buf));
  ASSERT_STR_EQ("$", sax_str_unescape("&#36;", buf));
  ASSERT_STR_EQ("?", sax_str_unescape("&#63;", buf));
  ASSERT_STR_EQ("¡", sax_str_unescape("&#161;", buf));
  ASSERT_STR_EQ("µ", sax_str_unescape("&#181;", buf));
  ASSERT_STR_EQ("é", sax_str_unescape("&#233;", buf));
  ASSERT_STR_EQ("А", sax_str_unescape("&#1040;", buf));
  ASSERT_STR_EQ("€", sax_str_unescape("&#8364;", buf));
  ASSERT_STR_EQ("™", sax_str_unescape("&#8482;", buf));
  ASSERT_STR_EQ("中", sax_str_unescape("&#20013;", buf));
  ASSERT_STR_EQ("心", sax_str_unescape("&#24515;", buf));
  ASSERT_STR_EQ("😀", sax_str_unescape("&#128512;", buf));
  ASSERT_STR_EQ("🌸", sax_str_unescape("&#127800;", buf));
  ASSERT_STR_EQ("🎵", sax_str_unescape("&#127925;", buf));
  ASSERT_STR_EQ("🚀", sax_str_unescape("&#128640;", buf));
}

static void TEST_DECL(unescape16) {

  char buf[5];
  ASSERT_STR_EQ("A", sax_str_unescape("&#x41;", buf));
  ASSERT_STR_EQ("a", sax_str_unescape("&#x61;", buf));
  ASSERT_STR_EQ("$", sax_str_unescape("&#x24;", buf));
  ASSERT_STR_EQ("?", sax_str_unescape("&#x3f;", buf));
  ASSERT_STR_EQ("¡", sax_str_unescape("&#xa1;", buf));
  ASSERT_STR_EQ("µ", sax_str_unescape("&#xb5;", buf));
  ASSERT_STR_EQ("é", sax_str_unescape("&#xe9;", buf));
  ASSERT_STR_EQ("А", sax_str_unescape("&#x410;", buf));
  ASSERT_STR_EQ("€", sax_str_unescape("&#x20ac;", buf));
  ASSERT_STR_EQ("™", sax_str_unescape("&#x2122;", buf));
  ASSERT_STR_EQ("中", sax_str_unescape("&#x4e2d;", buf));
  ASSERT_STR_EQ("心", sax_str_unescape("&#x5fc3;", buf));
  ASSERT_STR_EQ("😀", sax_str_unescape("&#x1f600;", buf));
  ASSERT_STR_EQ("🌸", sax_str_unescape("&#x1f338;", buf));
  ASSERT_STR_EQ("🎵", sax_str_unescape("&#x1f3b5;", buf));
  ASSERT_STR_EQ("🚀", sax_str_unescape("&#x1f680;", buf));
}

static void TEST_DECL(content) {

  arena_t *restrict arena = arena_create();

  ASSERT_STR_EQ("Foo Bar", xml_parse_content(arena, "   Foo Bar   \n"));

  arena_reset(arena);
  ASSERT_STR_EQ("😀", xml_parse_content(arena, "&#x1f600;"));

  arena_reset(arena);
  ASSERT_STR_EQ("🌸", xml_parse_content(arena, "    \t  &#x1f338;"));

  arena_reset(arena);
  ASSERT_STR_EQ("🎵", xml_parse_content(arena, "&#x1f3b5;    \t  \n"));

  arena_reset(arena);
  ASSERT_STR_EQ("🚀", xml_parse_content(arena, "\t   \r\n   &#x1f680;  \t  \n"));

  arena_free(arena);
}

static void TEST_DECL(trim) {

  char buf[1024];

  strcpy(buf, "   Foo Bar   \n");
  ASSERT_STR_EQ("Foo Bar", sax_str_trim(buf));

  strcpy(buf, "🎵");
  ASSERT_STR_EQ("🎵", sax_str_trim(buf));

  strcpy(buf, "   😀   \n");
  ASSERT_STR_EQ("😀", sax_str_trim(buf));

  strcpy(buf, "   🌸");
  ASSERT_STR_EQ("🌸", sax_str_trim(buf));

  strcpy(buf, "🚀   \n");
  ASSERT_STR_EQ("🚀", sax_str_trim(buf));
}

// === main === //

int main(int argc, char **args) {

  typedef struct test_t {
    const char *name;
    void (*func)(void);
  } test_t;

  const test_t tests[] = {
      TEST_REG(content),
      TEST_REG(trim),
      TEST_REG(unescape),
      TEST_REG(unescape10),
      TEST_REG(unescape16),

  };
  const size_t count = sizeof(tests) / sizeof(test_t);

  if (argc == 1) {
    for (size_t i = 0; i < count; ++i) {
      TEST(tests[i].name);
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
      TEST(tests[i].name);
      tests[i].func();
      return EXIT_SUCCESS;
    }
  }

  printf("Unknown test \"%s\"\n", args[1]);
  return EXIT_FAILURE;
}
