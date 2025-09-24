#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  // printf
#include <stdlib.h> // EXIT_SUCCESS
#include <string.h> // strrchr

#include "arena.h"
#include "test.h"
#include <saxamaphone.h>

#define TEST_DECL(test_name) test_##test_name(arena_t *restrict arena)
#define TEST_REG(test_name) {.name = #test_name, .func = test_##test_name}
#define LOG(...)                                        \
  printf("%s:%d - ", strrchr(__FILE__, '/'), __LINE__); \
  printf(__VA_ARGS__);                                  \
  printf("\n");

// === forward declares === //

const char *sax_str_unescape(const char *restrict src, char *restrict buf);
char *sax_str_trim(char *str);

// === utilities === //

static bool sax_attr_present(
    const sax_parser_t *restrict parser,
    const char *restrict name) {

  for (const sax_attr_t *restrict attr = sax_attrs(parser);
       attr != NULL;
       attr = attr->next) {
    if (strcmp(name, attr->name) == 0) {
      return true;
    }
  }

  return false;
}

static sax_parser_t *xml_parser(arena_t *restrict arena, const char *restrict xml) {
  return sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .string = xml,
  });
}

static const char *xml_parse_content(
    arena_t *restrict arena,
    const char *restrict content) {

  char xml[1024];
  snprintf(xml, sizeof(xml), "<content>%s</content>", content);

  sax_parser_t *parser = xml_parser(arena, xml);

  // <content>
  ASSERT_EQ(SAX_EVENT_START_TAG, sax_next(parser));
  // ...
  ASSERT_EQ(SAX_EVENT_CONTENT, sax_next(parser));
  return sax_content(parser);
}

static const char *xml_parse_attr_val(
    arena_t *restrict arena,
    const char *restrict attr_val) {

  char xml[1024];
  snprintf(xml, sizeof(xml), "<content name=\"%s\"/>", attr_val);

  sax_parser_t *parser = xml_parser(arena, xml);
  sax_event_t ev = sax_next(parser);

  if (ev == SAX_EVENT_ERROR) {
    printf("%s\n", sax_error(parser));
    return NULL;
  }

  ASSERT_EQ(SAX_EVENT_START_TAG, ev);
  ASSERT_STR_EQ("content", sax_tag(parser));

  const char *restrict attr = arena_strdup(arena, sax_attr(parser, "name"));
  sax_free(parser);
  return attr;
}

// === test cases === //

static void TEST_DECL(attr_val) {
  ASSERT_STR_EQ("foo", xml_parse_attr_val(arena, "foo"));
  ASSERT_STR_EQ("\tfoo", xml_parse_attr_val(arena, "\tfoo"));
  ASSERT_STR_EQ("foo\n", xml_parse_attr_val(arena, "foo\n"));
  ASSERT_STR_EQ("\t foo \n", xml_parse_attr_val(arena, "\t foo \n"));
  ASSERT_STR_EQ("中", xml_parse_attr_val(arena, "中"));
  ASSERT_STR_EQ("\t中", xml_parse_attr_val(arena, "\t中"));
  ASSERT_STR_EQ("中\n", xml_parse_attr_val(arena, "中\n"));
  ASSERT_STR_EQ("\t 中 \n", xml_parse_attr_val(arena, "\t 中 \n"));
  ASSERT_STR_EQ("😀", xml_parse_attr_val(arena, "&#128512;"));
  ASSERT_STR_EQ("\t😀", xml_parse_attr_val(arena, "\t&#128512;"));
  ASSERT_STR_EQ("😀\n", xml_parse_attr_val(arena, "&#128512;\n"));
  ASSERT_STR_EQ("\t 😀 \n", xml_parse_attr_val(arena, "\t &#128512; \n"));
}

static void TEST_DECL(cdata) {

  sax_parser_t *restrict parser = xml_parser(
      arena,
      "<content>"
      "  Sample content: "
      "  <![CDATA["
      "    <?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "    <body>Hello, world!</body>"
      "  ]]> (xml)"
      "</content>");

  ASSERT_EQ(SAX_EVENT_START_TAG, sax_next(parser));
  ASSERT_STR_EQ("content", sax_tag(parser));

  ASSERT_EQ(SAX_EVENT_CONTENT, sax_next(parser));
  ASSERT_STR_EQ("Sample content:", sax_content(parser));

  ASSERT_EQ(SAX_EVENT_CONTENT, sax_next(parser));
  ASSERT_STR_EQ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                "    <body>Hello, world!</body>",
                sax_content(parser));

  ASSERT_EQ(SAX_EVENT_CONTENT, sax_next(parser));
  ASSERT_STR_EQ("(xml)", sax_content(parser));

  ASSERT_EQ(SAX_EVENT_END_TAG, sax_next(parser));
  ASSERT_STR_EQ("content", sax_tag(parser));

  ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));
}

static void TEST_DECL(cdata_malformed) {
  sax_parser_t *restrict parser = xml_parser(
      arena,
      "<content>"
      "  Sample content: "
      "  <![CDATA["
      "    <?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "    <body>Hello, world!</body>"
      "  ] ]> (xml)"
      "</content>");

  ASSERT_EQ(SAX_EVENT_START_TAG, sax_next(parser));
  ASSERT_STR_EQ("content", sax_tag(parser));

  ASSERT_EQ(SAX_EVENT_CONTENT, sax_next(parser));
  ASSERT_STR_EQ("Sample content:", sax_content(parser));

  ASSERT_EQ(SAX_EVENT_ERROR, sax_next(parser));
  ASSERT_STR_EQ("Unexpected termination at line 1 column 133", sax_error(parser));
}

static void TEST_DECL(comment) {
  sax_parser_t *restrict parser = xml_parser(arena, "<!-- <hello, world!> -->");
  ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));
}

static void TEST_DECL(empty_element_tag) {

  sax_parser_t *restrict parser = xml_parser(arena, "<foo/>");

  ASSERT_EQ(SAX_EVENT_START_TAG, sax_next(parser));
  ASSERT_STR_EQ("foo", sax_tag(parser));
  ASSERT_NULL(sax_attrs(parser));
  ASSERT_EQ(SAX_EVENT_END_TAG, sax_next(parser));
  ASSERT_STR_EQ("foo", sax_tag(parser));
  ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));
  ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));

  sax_free(parser);
}

static void TEST_DECL(start_tag) {

  sax_parser_t *restrict parser = xml_parser(
      arena,
      "<content \n"
      "  alpha \n"
      "  beta=\"1\"\n"
      "  gamma=\"2\"\n"
      "  delta\n"
      "/>");

  ASSERT_EQ(SAX_EVENT_START_TAG, sax_next(parser));
  ASSERT_STR_EQ("content", sax_tag(parser));
  ASSERT_NON_NULL(sax_attrs(parser));
  ASSERT(sax_attr_present(parser, "alpha"));
  ASSERT_STR_EQ("1", sax_attr(parser, "beta"));
  ASSERT_STR_EQ("2", sax_attr(parser, "gamma"));
  ASSERT(sax_attr_present(parser, "delta"));
  ASSERT_EQ(SAX_EVENT_END_TAG, sax_next(parser));
  ASSERT_STR_EQ("content", sax_tag(parser));
  ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));
}

static void TEST_DECL(unescape) {

  (void)arena;

  char buf[5];
  ASSERT_STR_EQ("foo", sax_str_unescape("foo", buf));
  ASSERT_STR_EQ("<", sax_str_unescape("&lt;", buf));
  ASSERT_STR_EQ(">", sax_str_unescape("&gt;", buf));
  ASSERT_STR_EQ("&", sax_str_unescape("&amp;", buf));
  ASSERT_STR_EQ("'", sax_str_unescape("&apos;", buf));
  ASSERT_STR_EQ("\"", sax_str_unescape("&quot;", buf));
}

static void TEST_DECL(unescape10) {

  (void)arena;

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

  (void)arena;

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
  ASSERT_STR_EQ("Foo Bar", xml_parse_content(arena, "   Foo Bar   \n"));
  ASSERT_STR_EQ("😀", xml_parse_content(arena, "&#x1f600;"));
  ASSERT_STR_EQ("🌸", xml_parse_content(arena, "    \t  &#x1f338;"));
  ASSERT_STR_EQ("🎵", xml_parse_content(arena, "&#x1f3b5;    \t  \n"));
  ASSERT_STR_EQ("🚀", xml_parse_content(arena, "\t   \r\n   &#x1f680;  \t  \n"));
}

static void TEST_DECL(trim) {

  (void)arena;

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
    void (*func)(arena_t *restrict);
  } test_t;

  const test_t tests[] = {
      TEST_REG(attr_val),
      TEST_REG(cdata),
      TEST_REG(cdata_malformed),
      TEST_REG(comment),
      TEST_REG(content),
      TEST_REG(empty_element_tag),
      TEST_REG(start_tag),
      TEST_REG(trim),
      TEST_REG(unescape),
      TEST_REG(unescape10),
      TEST_REG(unescape16),

  };
  const size_t count = sizeof(tests) / sizeof(test_t);

  // Run all tests
  if (argc == 1) {

    arena_t *arena = arena_create();

    for (size_t i = 0; i < count; ++i) {
      TEST(tests[i].name);
      tests[i].func(arena);
      arena_reset(arena);
    }

    LOG("Arena managed %zu bytes", arena_size(arena));
    arena_free(arena);
    return EXIT_SUCCESS;
  }

  // Help
  if (strcmp("-h", args[1]) == 0) {
    printf("Usage: %s [-h] [-l] [<test-name>]\n", args[0]);
    return EXIT_SUCCESS;
  }

  // List tests
  if (strcmp("-l", args[1]) == 0) {

    for (size_t i = 0; i < count; ++i) {
      printf("%s\n", tests[i].name);
    }
    return EXIT_SUCCESS;
  }

  // Run specific test
  for (size_t i = 0; i < count; ++i) {
    if (strcmp(tests[i].name, args[1]) == 0) {
      TEST(tests[i].name);
      arena_t *restrict arena = arena_create();
      tests[i].func(arena);
      LOG("Arena managed %zu bytes", arena_size(arena));
      arena_free(arena);
      return EXIT_SUCCESS;
    }
  }

  printf("Unknown test \"%s\"\n", args[1]);
  return EXIT_FAILURE;
}
