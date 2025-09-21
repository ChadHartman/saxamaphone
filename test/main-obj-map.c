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

int main() {

  TEST("Object Mapping Tests");
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
  return EXIT_SUCCESS;
}