#include <string.h> // strcmp

#include "test.h"

#define ASSERT_PROC_INST(arena, xml, tag, ...) \
  assert_proc_inst(                            \
      arena,                                   \
      xml,                                     \
      tag,                                     \
      (pi_attr_t[]){                           \
          __VA_ARGS__,                         \
          {0},                                 \
      });

typedef struct pi_attr_t {
  const char *name;
  const char *value;
} pi_attr_t;

static bool has_attr(const sax_parser_t *restrict parser, const char *restrict name) {

  for (const sax_attr_t *restrict attr = sax_attrs(parser); attr != NULL; attr = attr->next) {
    if (strcmp(name, attr->name) == 0) {
      return true;
    }
  }

  return false;
}

static void assert_proc_inst(
    arena_t *restrict arena,
    const char *restrict xml,
    const char *restrict tag,
    const pi_attr_t *restrict attrs) {

  arena_reset(arena);

  // Happy path
  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .string = xml,
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
  });

  ASSERT_EQ(SAX_EVENT_PROCESSING_INSTRUCTION, sax_next(parser));
  ASSERT_STR_EQ(tag, sax_tag(parser));
  for (const pi_attr_t *restrict attr = attrs; attr->name != NULL; ++attr) {
    if (attr->value == NULL) {
      ASSERT(has_attr(parser, attr->name));
    } else {
      ASSERT_STR_EQ(attr->value, sax_attr(parser, attr->name));
    }
  }
  ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));

  sax_free(parser);
}

TEST(proc_inst) {

  ASSERT_PROC_INST(
      arena,
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>",
      "xml",
      {"version", "1.0"},
      {"encoding", "UTF-8"});

  ASSERT_PROC_INST(
      arena,
      "<?alpha beta=\"gamma\" delta?>",
      "alpha",
      {"beta", "gamma"},
      {"delta", NULL});

  ASSERT_PROC_INST(
      arena,
      "<?alpha beta gamma=\"delta\"?>",
      "alpha",
      {"beta", NULL},
      {"gamma", "delta"});

  ASSERT_PROC_INST(arena, "<?alpha?>", "alpha", {0});
  ASSERT_PROC_INST(arena, "<?alpha ?>", "alpha", {0});

  ASSERT_XML_ERR(arena, "<?>", "Unexpected character '>' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "<\?\?>", "Unexpected character '?' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "<? ?>", "Unexpected character ' ' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "<? xml?>", "Unexpected character ' ' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "<?alpha>", "Unexpected character '>' located on line 1 column 8");
}