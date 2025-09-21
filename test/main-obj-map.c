#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  // printf
#include <stdlib.h> // EXIT_SUCCESS
#include <string.h> // strrchr

#include "arena.h"
#include "test.h"
#include <saxamaphone.h>

#define LOG(...)                                                                   \
  printf(COLOR_CYAN "%s:%d " COLOR_RESET, (strrchr(__FILE__, '/') + 1), __LINE__); \
  printf(__VA_ARGS__)

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

static bool sax_tag_is(
    sax_parser_t *restrict parser,
    const char *restrict expected) {
  return strcmp(expected, sax_tag(parser)) == 0;
}

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
    LOG("ERROR: unexpected tag \"%s\"\n", sax_tag(parser));
    return NULL;
  }

  string_node_t *restrict node = arena_alloc(arena, sizeof(string_node_t));
  if (SAX_EVENT_CONTENT != sax_next(parser)) {
    LOG("ERROR: missing content for \"%s\"\n", tag);
    return NULL;
  }

  node->value = arena_strdup(arena, sax_content(parser));

  if (sax_next_is(parser, SAX_EVENT_END_ELEMENT, tag)) {
    return node;
  }

  LOG("ERROR: unexpected tag \"%s\"\n", sax_tag(parser));

  return NULL;
}

static prog_lang_t *map_prog_lang(
    arena_t *restrict arena,
    sax_parser_t *restrict parser) {

  prog_lang_t *restrict lang = arena_alloc(arena, sizeof(prog_lang_t));
  lang->name = arena_strdup(arena, sax_attr(parser, "name"));
  lang->first_appeared = sax_attr_d32(parser, "first-appeared");

  for (sax_event_t ev = sax_next(parser);
       ev != SAX_EVENT_END_ELEMENT && ev != SAX_EVENT_ERROR;
       ev = sax_next(parser)) {

    if (sax_tag_is(parser, "paradigms")) {
      lang->paradigms = map_string_node(arena, parser, "paradigm");
      if (!lang->paradigms) {
        return NULL;
      }
    }

    if (sax_tag_is(parser, "typing-dicipline")) {
      lang->typing = map_string_node(arena, parser, "typing");
      if (!lang->typing) {
        return NULL;
      }
    }

    if (sax_tag_is(parser, "execution-model")) {
      lang->exe_model = map_string_node(arena, parser, "model");
      if (!lang->exe_model) {
        return NULL;
      }
    }

    if (sax_tag_is(parser, "application-domains")) {
      lang->app_doms = map_string_node(arena, parser, "domain");
      if (!lang->app_doms) {
        return NULL;
      }
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

  prog_lang_t *restrict langs = NULL;

  for (sax_event_t ev = sax_next(parser);
       ev != SAX_EVENT_END_DOCUMENT && ev != SAX_EVENT_ERROR;
       ev = sax_next(parser)) {

    if (ev == SAX_EVENT_START_ELEMENT && sax_tag_is(parser, "language")) {

      prog_lang_t *restrict lang = map_prog_lang(arena, parser);
      ASSERT_NON_NULL(lang);

      if (langs) {
        langs->next = langs;
      }

      langs = lang;
    } else if (ev == SAX_EVENT_END_ELEMENT && sax_tag_is(parser, "programming-languages")) {
      break;
    } else {
      FAIL("Unexpected tag %s", sax_tag(parser));
    }
  }

  ASSERT_NON_NULL(langs);
  ASSERT_STR_EQ("JavaScript", langs->name);

  langs = langs->next;
  ASSERT_NON_NULL(langs);
  ASSERT_STR_EQ("C", langs->name);

  langs = langs->next;
  ASSERT_NON_NULL(langs);
  ASSERT_STR_EQ("Java", langs->name);

  langs = langs->next;
  ASSERT_NON_NULL(langs);
  ASSERT_STR_EQ("Python", langs->name);

  arena_free(arena);
  return EXIT_SUCCESS;
}