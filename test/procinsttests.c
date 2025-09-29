#include "test.h"

typedef struct pi_attr_t {
  const char *name;
  const char *value;
} pi_attr_t;

static void assert_proc_inst(
    arena_t *restrict arena,
    const char *restrict xml,
    const char *restrict tag,
    const pi_attr_t *restrict attrs) {

  // Happy path
  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .string = xml,
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
  });

  ASSERT_EQ(SAX_EVENT_PROCESSING_INSTRUCTION, sax_next(parser));
  ASSERT_STR_EQ(tag, sax_tag(parser));
  for (const pi_attr_t *restrict attr = attrs; attr->name != NULL; ++attr) {
    ASSERT_STR_EQ(attr->value, sax_attr(parser, attr->name));
  }
  ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));

  sax_free(parser);
}

TEST(proc_inst) {

  assert_proc_inst(
      arena,
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>",
      "xml",
      (pi_attr_t[]){
          {"version", "1.0"},
          {"encoding", "UTF-8"},
          {0},
      });
}