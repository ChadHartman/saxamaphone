#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  // printf
#include <stdlib.h> // EXIT_SUCCESS
#include <string.h> // strrchr

#include "arena.h"
#include "test.h"
#include "testlist.h"
#include <saxamaphone.h>

static void print_test(const char *restrict header) {
  const size_t len = strlen(header);
  putchar('+');
  for (size_t i = 0; i < len + 4; ++i) {
    putchar('-');
  }
  printf("+\n| " COLOR_MAGENTA " %s " COLOR_RESET " |\n+", header);
  for (size_t i = 0; i < len + 4; ++i) {
    putchar('-');
  }
  printf("+\n");
}

// === forward declares === //

size_t sax_file_buf_size(size_t capacity);
void *sax_default_alloc(void *ctx, void *ptr, size_t size);

// === utilities === //

static sax_parser_t *xml_parser(arena_t *restrict arena, const char *restrict xml) {
  return sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .string = xml,
  });
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

TEST(attr_val) {
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

TEST(cdata) {

  sax_parser_t *restrict parser = xml_parser(
      arena,
      "<content>"
      "  Sample content: "
      "  <![CDATA[<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "    <body>Hello, world!</body>]]> (xml)"
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

TEST(cdata_malformed) {
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

TEST(comment) {
  sax_parser_t *restrict parser = xml_parser(arena, "<!-- <hello, world!> -->");
  ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));
}

TEST(default_alloc) {

  (void)arena;

  size_t *size = sax_default_alloc(NULL, NULL, sizeof(size_t));
  ASSERT_NON_NULL(size);
  *size = 42;
  size = sax_default_alloc(NULL, size, 2 * sizeof(size_t));
  ASSERT_EQ(42, *size);
  ASSERT_NULL(sax_default_alloc(NULL, size, 0));
}

TEST(file_buf_size) {

  (void)arena;
  ASSERT_EQ(32, sax_file_buf_size(0));
  ASSERT_EQ(32, sax_file_buf_size(31));
  ASSERT_EQ(256, sax_file_buf_size(512));
  ASSERT_EQ(1024, sax_file_buf_size(4095));
}

// === main === //

int main(int argc, char **args) {

  const size_t count = sizeof(tests) / sizeof(test_t);

  // Run all tests
  if (argc == 1) {

    arena_t *arena = arena_create();

    for (size_t i = 0; i < count; ++i) {
      print_test(tests[i].name);
      tests[i].func(arena);
      arena_reset(arena);
    }

    TEST_LOG("Arena managed %zu bytes", arena_size(arena));
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
      print_test(tests[i].name);
      arena_t *restrict arena = arena_create();
      tests[i].func(arena);
      TEST_LOG("Arena managed %zu bytes", arena_size(arena));
      arena_free(arena);
      return EXIT_SUCCESS;
    }
  }

  printf("Unknown test \"%s\"\n", args[1]);
  return EXIT_FAILURE;
}
