#include <assert.h>
#include <stdbool.h>
#include <stdio.h>  // printf
#include <stdlib.h> // EXIT_SUCCESS
#include <string.h> // strrchr

#include "test.h"
#include <saxamaphone.h>

static void print_header(const char *header) {

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

static void test_sax_str_substr() {

  print_header("sax_str_substr tests");
  ASSERT_STR_EQ("ooba", sax_str_substr(sax_str("foobar"), 1, 4));
  ASSERT_STR_EQ("oobar", sax_str_substr(sax_str("foobar"), 1, UINT32_MAX));
  ASSERT_STR_EQ("", sax_str_substr(sax_str("foobar"), 1, 0));
  ASSERT_STR_EQ("foob", sax_str_substr(sax_str("foobar"), 0, 4));
  ASSERT_STR_EQ("", sax_str_substr(sax_str("foobar"), UINT32_MAX, 1));
  ASSERT_STR_EQ("", sax_str_substr(sax_str(""), 1, 2));
}

static void test_sax_unescaped() {

  print_header("sax_str_unescaped");
  ASSERT_STR_EQ("foo", sax_str_unescaped(sax_str("foo")));
  ASSERT_STR_EQ("<", sax_str_unescaped(sax_str("&lt;")));
  ASSERT_STR_EQ(">", sax_str_unescaped(sax_str("&gt;")));
  ASSERT_STR_EQ("&", sax_str_unescaped(sax_str("&amp;")));
  ASSERT_STR_EQ("'", sax_str_unescaped(sax_str("&apos;")));
  ASSERT_STR_EQ("\"", sax_str_unescaped(sax_str("&quot;")));
  ASSERT_STR_EQ("A", sax_str_unescaped(sax_str("&#65;")));
  ASSERT_STR_EQ("a", sax_str_unescaped(sax_str("&#97;")));
  ASSERT_STR_EQ("$", sax_str_unescaped(sax_str("&#36;")));
  ASSERT_STR_EQ("?", sax_str_unescaped(sax_str("&#63;")));
  ASSERT_STR_EQ("¡", sax_str_unescaped(sax_str("&#161;")));
  ASSERT_STR_EQ("µ", sax_str_unescaped(sax_str("&#181;")));
  ASSERT_STR_EQ("é", sax_str_unescaped(sax_str("&#233;")));
  ASSERT_STR_EQ("А", sax_str_unescaped(sax_str("&#1040;")));
  ASSERT_STR_EQ("€", sax_str_unescaped(sax_str("&#8364;")));
  ASSERT_STR_EQ("™", sax_str_unescaped(sax_str("&#8482;")));
  ASSERT_STR_EQ("中", sax_str_unescaped(sax_str("&#20013;")));
  ASSERT_STR_EQ("心", sax_str_unescaped(sax_str("&#24515;")));
  ASSERT_STR_EQ("😀", sax_str_unescaped(sax_str("&#128512;")));
  ASSERT_STR_EQ("🌸", sax_str_unescaped(sax_str("&#127800;")));
  ASSERT_STR_EQ("🎵", sax_str_unescaped(sax_str("&#127925;")));
  ASSERT_STR_EQ("🚀", sax_str_unescaped(sax_str("&#128640;")));
}

int main() {

  test_sax_str_substr();
  test_sax_unescaped();

  print_header("end-to-end");
  sax_event_t ev = 0;
  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .path = "../test/files/xml-well-formed.xml",
  });

  for (ev = sax_next(parser);
       ev != SAX_EVENT_END_DOCUMENT && ev != SAX_EVENT_ERROR;
       ev = sax_next(parser)) {
    switch (ev) {

    case SAX_EVENT_START_ELEMENT: {
      const sax_str_t tag = sax_tag(parser);
      printf("SAX_EVENT_START_ELEMENT tag=\"%.*s\" attrs={", tag.size, tag.value);
      const sax_attrs_t attrs = sax_attrs(parser);
      for (uint_fast16_t i = 0; i < attrs.count; ++i) {
        const sax_attr_t attr = attrs.attrs[i];
        printf("\"%.*s\"=\"%.*s\", ",
               attr.name.size,
               attr.name.value,
               attr.value.size,
               attr.value.value);
      }
      printf("}\n");

    } break;

    case SAX_EVENT_CONTENT: {
      sax_str_t content = sax_content(parser);
      printf("SAX_EVENT_CONTENT content=\"%.*s\"\n", content.size, content.value);
    } break;

    case SAX_EVENT_END_ELEMENT: {
      sax_str_t tag = sax_tag(parser);
      printf("SAX_EVENT_END_ELEMENT tag=\"%.*s\"\n", tag.size, tag.value);
    } break;

    default:
      assert(false);
      break;
    }
  }

  if (ev == SAX_EVENT_END_DOCUMENT) {
    printf("SAX_EVENT_END_DOCUMENT\n");
    return EXIT_SUCCESS;
  }

  if (ev == SAX_EVENT_ERROR) {
    sax_str_t err = sax_error(parser);
    printf("SAX_EVENT_ERROR \"%.*s\"\n", err.size, err.value);
  } else {
    printf("Unknown event %d\n", ev);
  }

  return EXIT_FAILURE;
}