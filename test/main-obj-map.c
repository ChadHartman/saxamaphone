#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  // printf
#include <stdlib.h> // EXIT_SUCCESS
#include <string.h> // strrchr

#include "arena.h"
#include "test.h"
#include <saxamaphone.h>

#if 1
#define LOG(...)                                                                   \
  printf(COLOR_CYAN "%s:%d " COLOR_RESET, (strrchr(__FILE__, '/') + 1), __LINE__); \
  printf(__VA_ARGS__);                                                             \
  printf("\n")
#else
#define LOG(...) ((void)0)
#endif

/// @brief Programming Language
typedef struct prog_lang_t {

  char *name;
  int16_t first_appeared;

  char *paradigms[8];
  char *typing[8];
  char *exe_model[8];
  char *app_doms[8];

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

static char *map_string_node(
    arena_t *restrict arena,
    sax_parser_t *restrict parser,
    const char *restrict tag) {

  const sax_event_t ev = sax_next(parser);
  if (SAX_EVENT_CONTENT != ev) {
    LOG("ERROR: missing content for \"%s\"\n", tag);
    return NULL;
  }

  char *restrict value = arena_strdup(arena, sax_content(parser));
  LOG("Mapped \"%s\" value \"%s\"\n", tag, value);

  if (sax_next_is(parser, SAX_EVENT_END_TAG, tag)) {
    return value;
  }

  LOG("ERROR: unexpected tag \"%s\"\n", sax_tag(parser));
  return NULL;
}

static bool map_string_nodes(
    arena_t *restrict arena,
    sax_parser_t *restrict parser,
    char **nodes,
    const char *restrict collection_tag,
    const char *restrict tag) {

  size_t node_offset = 0;

  for (sax_event_t ev = sax_next(parser);
       ev != SAX_EVENT_END_DOCUMENT && ev != SAX_EVENT_ERROR;
       ev = sax_next(parser)) {

    if (ev == SAX_EVENT_START_TAG && sax_tag_is(parser, tag)) {
      nodes[node_offset++] = map_string_node(arena, parser, tag);
      if (!nodes[node_offset - 1]) {
        return false;
      }

    } else if (ev == SAX_EVENT_END_TAG && sax_tag_is(parser, collection_tag)) {
      return true;

    } else {
      LOG("Unexpected tag \"%s\"", sax_tag(parser));
      return false;
    }
  }

  LOG("Unreachable section");
  return false;
}

static bool map_prog_lang(
    arena_t *restrict arena,
    sax_parser_t *restrict parser,
    prog_lang_t *restrict out) {

  // In <language>
  out->name = arena_strdup(arena, sax_attr(parser, "name"));
  out->first_appeared = sax_attr_d32(parser, "first-appeared");

  for (sax_event_t ev = sax_next(parser);
       ev != SAX_EVENT_END_DOCUMENT && ev != SAX_EVENT_ERROR;
       ev = sax_next(parser)) {

    if (ev == SAX_EVENT_END_TAG && sax_tag_is(parser, "language")) {
      return true;
    }

    if (sax_tag_is(parser, "paradigms")) {
      if (!map_string_nodes(arena, parser, out->paradigms, "paradigms", "paradigm"))
        return false;
    }

    if (sax_tag_is(parser, "typing-dicipline")) {
      if (!map_string_nodes(arena, parser, out->typing, "typing-dicipline", "typing")) {
        return false;
      }
    }

    if (sax_tag_is(parser, "execution-model")) {
      if (!map_string_nodes(arena, parser, out->exe_model, "execution-model", "model")) {
        return false;
      }
    }

    if (sax_tag_is(parser, "application-domains")) {
      if (!map_string_nodes(arena, parser, out->app_doms, "application-domains", "domain")) {
        return false;
      }
    }
  }

  LOG("Unreachable section\n");
  return false;
}

int main() {

  TEST("Object Mapping Tests");
  arena_t *restrict arena = arena_create();
  sax_parser_t *parser = sax_parser(&(sax_config_t){
      .path = "../test/files/programming-languages.xml",
      .publish_processing_instructions = true,
  });

  sax_event_t ev = sax_next(parser);
  if (ev == SAX_EVENT_ERROR) {
    LOG("error: \"%s\"", sax_error(parser));
  }
  ASSERT_EQ(SAX_EVENT_PROCESSING_INSTRUCTION, ev);
  ASSERT_STR_EQ("xml", sax_tag(parser));
  ASSERT_STR_EQ("1.0", sax_attr(parser, "version"));
  ASSERT_STR_EQ("UTF-8", sax_attr(parser, "encoding"));

  ASSERT_EQ(SAX_EVENT_START_TAG, sax_next(parser));
  ASSERT_STR_EQ("programming-languages", sax_tag(parser));

  prog_lang_t langs[8] = {0};
  size_t lang_offset = 0;

  for (sax_event_t ev = sax_next(parser);
       ev != SAX_EVENT_END_DOCUMENT && ev != SAX_EVENT_ERROR;
       ev = sax_next(parser)) {

    if (ev == SAX_EVENT_START_TAG && sax_tag_is(parser, "language")) {
      if (!map_prog_lang(arena, parser, &langs[lang_offset++])) {
        FAIL("Failed to map language");
      }

    } else if (ev == SAX_EVENT_END_TAG && sax_tag_is(parser, "programming-languages")) {
      break;
    } else {
      FAIL("Unexpected tag \"%s\"", sax_tag(parser));
    }
  }

  ASSERT_STR_EQ("Python", langs[0].name);
  ASSERT_EQ(1991, langs[0].first_appeared);
  ASSERT_STR_EQ("Procedural", langs[0].paradigms[0]);
  ASSERT_STR_EQ("Object-Oriented", langs[0].paradigms[1]);
  ASSERT_STR_EQ("Functional", langs[0].paradigms[2]);
  ASSERT_STR_EQ("Dynamic", langs[0].typing[0]);
  ASSERT_STR_EQ("Strong", langs[0].typing[1]);
  ASSERT_STR_EQ("Interpreted", langs[0].exe_model[0]);
  ASSERT_STR_EQ("Web Development", langs[0].app_doms[0]);
  ASSERT_STR_EQ("Data Science", langs[0].app_doms[1]);
  ASSERT_STR_EQ("Machine Learning", langs[0].app_doms[2]);
  ASSERT_STR_EQ("Scripting", langs[0].app_doms[3]);

  ASSERT_STR_EQ("Java", langs[1].name);
  ASSERT_EQ(1995, langs[1].first_appeared);
  ASSERT_STR_EQ("Object-Oriented", langs[1].paradigms[0]);
  ASSERT_STR_EQ("Concurrent", langs[1].paradigms[1]);
  ASSERT_STR_EQ("Static", langs[1].typing[0]);
  ASSERT_STR_EQ("Strong", langs[1].typing[1]);
  ASSERT_STR_EQ("Compiled (to bytecode)", langs[1].exe_model[0]);
  ASSERT_STR_EQ("Interpreted (by JVM)", langs[1].exe_model[1]);
  ASSERT_STR_EQ("Enterprise Systems", langs[1].app_doms[0]);
  ASSERT_STR_EQ("Android Development", langs[1].app_doms[1]);
  ASSERT_STR_EQ("Web Development", langs[1].app_doms[2]);

  ASSERT_STR_EQ("C", langs[2].name);
  ASSERT_EQ(1972, langs[2].first_appeared);
  ASSERT_STR_EQ("Procedural", langs[2].paradigms[0]);
  ASSERT_STR_EQ("Static", langs[2].typing[0]);
  ASSERT_STR_EQ("Weak", langs[2].typing[1]);
  ASSERT_STR_EQ("Compiled (to machine code)", langs[2].exe_model[0]);
  ASSERT_STR_EQ("System Programming", langs[2].app_doms[0]);
  ASSERT_STR_EQ("Embedded Systems", langs[2].app_doms[1]);
  ASSERT_STR_EQ("Game Development", langs[2].app_doms[2]);

  ASSERT_STR_EQ("JavaScript", langs[3].name);
  ASSERT_EQ(1995, langs[3].first_appeared);
  ASSERT_STR_EQ("Event-Driven", langs[3].paradigms[0]);
  ASSERT_STR_EQ("Object-Oriented", langs[3].paradigms[1]);
  ASSERT_STR_EQ("Functional", langs[3].paradigms[2]);
  ASSERT_STR_EQ("Dynamic", langs[3].typing[0]);
  ASSERT_STR_EQ("Weak", langs[3].typing[1]);
  ASSERT_STR_EQ("Interpreted (JIT compilation)", langs[3].exe_model[0]);
  ASSERT_STR_EQ("Web Development (Frontend & Backend)", langs[3].app_doms[0]);
  ASSERT_STR_EQ("Mobile Development", langs[3].app_doms[1]);

  arena_free(arena);
  return EXIT_SUCCESS;
}