#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  // printf
#include <stdlib.h> // EXIT_SUCCESS
#include <string.h> // strrchr

#include "arena.h"
#include "test.h"
#include <saxamaphone.h>

typedef struct string_node_t {
  char *value;
  struct string_t *next;
} string_node_t;

/// @brief Programming Language
typedef struct prog_lang_t {

  char *name;
  int16_t first_appeared;

  string_node_t *paradigms;
  string_node_t *typing;
  string_node_t *exe_model;
  string_node_t *app_doms;

  struct prog_lang_t *next;

} prog_lang_t;

static bool sax_next_is(
    sax_parser_t *restrict parser,
    const sax_event_t expected_ev,
    const char *restrict expected_tag) {

  const sax_event_t ev = sax_next(parser);
  if (expected_ev != ev) {
    return false;
  }

  if (!expected_tag) {
    return true;
  }

  return strcmp(expected_tag, sax_tag(parser)) == 0;
}

static int32_t sax_attr_d32(sax_parser_t *restrict parser, const char *restrict name) {
  const char *restrict value = sax_attr(parser, name);
  if (value == NULL) {
    return 0;
  }

  return (int32_t)atol(value);
}

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

static string_node_t *map_string_node(
    arena_t *restrict arena,
    sax_parser_t *restrict parser,
    const char *restrict tag) {

  if (!sax_next_is(parser, SAX_EVENT_START_ELEMENT, tag)) {
    return NULL;
  }

  string_node_t *restrict node = arena_alloc(arena, sizeof(string_node_t));
  if (SAX_EVENT_CONTENT != sax_next(parser)) {
    // Error missing content
    return NULL;
  }

  node->value = arena_strdup(arena, sax_content(parser));

  if (sax_next_is(parser, SAX_EVENT_END_ELEMENT, tag)) {
    return node;
  }

  return NULL;
}

static prog_lang_t *map_prog_lang(
    arena_t *restrict arena,
    sax_parser_t *restrict parser) {

  if (!sax_next_is(parser, SAX_EVENT_START_ELEMENT, "language")) {
    return NULL;
  }

  prog_lang_t *restrict lang = arena_alloc(arena, sizeof(prog_lang_t));
  lang->name = arena_strdup(arena, sax_attr(parser, "name"));
  lang->first_appeared = sax_attr_d32(parser, "first-appeared");

  for (sax_event_t ev = sax_next(parser);
       ev != SAX_EVENT_END_ELEMENT && ev != SAX_EVENT_ERROR;
       ev = sax_next(parser)) {

    if (strcmp("paradigms", sax_tag(parser)) == 0) {
      lang->paradigms = map_string_node(arena, parser, "paradigm");
    }

    if (strcmp("typing-dicipline", sax_tag(parser)) == 0) {
      lang->typing = map_string_node(arena, parser, "typing");
    }

    if (strcmp("execution-model", sax_tag(parser)) == 0) {
      lang->exe_model = map_string_node(arena, parser, "model");
    }

    if (strcmp("application-domains", sax_tag(parser)) == 0) {
      lang->app_doms = map_string_node(arena, parser, "domain");
    }
  }

  return lang;
}

static void test_object_mapping() {

  print_header("Object Mapping Tests");
  arena_t *restrict arena = arena_create();
  sax_parser_t *parser = sax_parser(&(sax_config_t){
      .path = "../test/files/programming-languages.xml",
  });
  ASSERT_EQ(SAX_EVENT_START_ELEMENT, sax_next(parser));
  ASSERT_STR_EQ("programming-languages", sax_tag(parser));

  prog_lang_t *restrict lang = map_prog_lang(arena, parser);
  ASSERT_NON_NULL(lang);
  ASSERT_STR_EQ("Python", lang->name);
  ASSERT_EQ(1991, lang->first_appeared);
  ASSERT_NULL(lang->next);

  arena_free(arena);
}

// static void test_sax_unescaped() {

//   print_header("sax_str_unescaped");
//   ASSERT_STR_EQ("foo", sax_str_unescaped(sax_str("foo")));
//   ASSERT_STR_EQ("<", sax_str_unescaped(sax_str("&lt;")));
//   ASSERT_STR_EQ(">", sax_str_unescaped(sax_str("&gt;")));
//   ASSERT_STR_EQ("&", sax_str_unescaped(sax_str("&amp;")));
//   ASSERT_STR_EQ("'", sax_str_unescaped(sax_str("&apos;")));
//   ASSERT_STR_EQ("\"", sax_str_unescaped(sax_str("&quot;")));
//   ASSERT_STR_EQ("A", sax_str_unescaped(sax_str("&#65;")));
//   ASSERT_STR_EQ("a", sax_str_unescaped(sax_str("&#97;")));
//   ASSERT_STR_EQ("$", sax_str_unescaped(sax_str("&#36;")));
//   ASSERT_STR_EQ("?", sax_str_unescaped(sax_str("&#63;")));
//   ASSERT_STR_EQ("¡", sax_str_unescaped(sax_str("&#161;")));
//   ASSERT_STR_EQ("µ", sax_str_unescaped(sax_str("&#181;")));
//   ASSERT_STR_EQ("é", sax_str_unescaped(sax_str("&#233;")));
//   ASSERT_STR_EQ("А", sax_str_unescaped(sax_str("&#1040;")));
//   ASSERT_STR_EQ("€", sax_str_unescaped(sax_str("&#8364;")));
//   ASSERT_STR_EQ("™", sax_str_unescaped(sax_str("&#8482;")));
//   ASSERT_STR_EQ("中", sax_str_unescaped(sax_str("&#20013;")));
//   ASSERT_STR_EQ("心", sax_str_unescaped(sax_str("&#24515;")));
//   ASSERT_STR_EQ("😀", sax_str_unescaped(sax_str("&#128512;")));
//   ASSERT_STR_EQ("🌸", sax_str_unescaped(sax_str("&#127800;")));
//   ASSERT_STR_EQ("🎵", sax_str_unescaped(sax_str("&#127925;")));
//   ASSERT_STR_EQ("🚀", sax_str_unescaped(sax_str("&#128640;")));
// }

int main(int argc, char **args) {

  if (argc == 2) {

    if (strcmp("-h", args[1]) == 0) {
      printf("Usage: %s [-h, substr, objmap]\n", args[0]);
      return EXIT_SUCCESS;
    }

    if (strcmp("substr", args[1]) == 0) {
      test_sax_str_substr();
      return EXIT_SUCCESS;
    }

    if (strcmp("objmap", args[1]) == 0) {
      test_object_mapping();
      return EXIT_SUCCESS;
    }

    printf("Unknown test \"%s\"\n", args[1]);
    return EXIT_FAILURE;
  }

  test_sax_str_substr();
  // test_sax_unescaped();
  test_object_mapping();

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
      const char *tag = sax_tag(parser);
      printf("SAX_EVENT_START_ELEMENT tag=\"%s\" attrs={", tag);
      for (const sax_attr_t *attr = sax_attrs(parser);
           attr != NULL;
           attr = attr->next) {
        printf("\"%s\"=\"%s\", ", attr->name, attr->value);
      }
      printf("}\n");

    } break;

    case SAX_EVENT_CONTENT: {
      printf("SAX_EVENT_CONTENT content=\"%s\"\n", sax_content(parser));
    } break;

    case SAX_EVENT_END_ELEMENT: {
      printf("SAX_EVENT_END_ELEMENT tag=\"%s\"\n", sax_tag(parser));
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
    printf("SAX_EVENT_ERROR \"%s\"\n", sax_error(parser));
  } else {
    printf("Unknown event %d\n", ev);
  }

  return EXIT_FAILURE;
}