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

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .xml = xml,
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
  });

  ASSERT_EQ(SAX_EVENT_PROCESSING_INSTRUCTION, sax_next(parser));
  ASSERT_STR_EQ(tag, sax_tag(parser));

  if (attrs[0].name == NULL) {
    ASSERT_NULL(sax_attrs(parser));
  }

  for (const pi_attr_t *restrict attr = attrs; attr->name != NULL; ++attr) {
    if (attr->value == NULL) {
      ASSERT(has_attr(parser, attr->name));
    } else {
      ASSERT_STR_EQ(attr->value, sax_attr(parser, attr->name));
    }
  }
  ASSERT_EQ(SAX_EVENT_END_DOCUMENT, sax_next(parser));

  sax_parser_free(parser);
}

TEST(proc_inst) {

  ASSERT_PROC_INST(arena, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>", "xml", {"version", "1.0"}, {"encoding", "UTF-8"});
  ASSERT_PROC_INST(arena, "<?alpha beta=\"gamma\" \ndelta=\"epsilon\" \nzeta=\"eta\" ?>", "alpha", {"beta", "gamma"}, {"delta", "epsilon"}, {"zeta", "eta"});
  ASSERT_PROC_INST(arena, "<?alpha?>", "alpha", {0});
  ASSERT_PROC_INST(arena, "<?alpha ?>", "alpha", {0});

  ASSERT_PROC_INST(arena, "<?alpha beta=\"foo\"?>", "alpha", {"beta", "foo"});
  ASSERT_PROC_INST(arena, "<?alpha beta=\"\tfoo\"?>", "alpha", {"beta", "\tfoo"});
  ASSERT_PROC_INST(arena, "<?alpha beta=\"foo\n\"?>", "alpha", {"beta", "foo\n"});
  ASSERT_PROC_INST(arena, "<?alpha beta=\"\t foo \n\"?>", "alpha", {"beta", "\t foo \n"});
  ASSERT_PROC_INST(arena, "<?alpha beta=\"中\"?>", "alpha", {"beta", "中"});
  ASSERT_PROC_INST(arena, "<?alpha beta=\"\t中\"?>", "alpha", {"beta", "\t中"});
  ASSERT_PROC_INST(arena, "<?alpha beta=\"中\n\"?>", "alpha", {"beta", "中\n"});
  ASSERT_PROC_INST(arena, "<?alpha beta=\"\t 中 \n\"?>", "alpha", {"beta", "\t 中 \n"});
  ASSERT_PROC_INST(arena, "<?alpha beta=\"&#128512;\"?>", "alpha", {"beta", "😀"});
  ASSERT_PROC_INST(arena, "<?alpha beta=\"\t&#128512;\"?>", "alpha", {"beta", "\t😀"});
  ASSERT_PROC_INST(arena, "<?alpha beta=\"&#128512;\n\"?>", "alpha", {"beta", "😀\n"});
  ASSERT_PROC_INST(arena, "<?alpha beta=\"\t &#128512; \n\"?>", "alpha", {"beta", "\t 😀 \n"});

  ASSERT_XML_ERR(arena, "<?alpha beta=\"gamma\" delta \n?>", "Unexpected character ' ' located on line 1 column 27");
  ASSERT_XML_ERR(arena, "<?alpha \t beta  gamma=\"delta\" \n?>", "Unexpected character ' ' located on line 1 column 15");
  ASSERT_XML_ERR(arena, "<?alpha beta=\"gamma\" \tdelta?>", "Unexpected character '?' located on line 1 column 28");
  ASSERT_XML_ERR(arena, "Hello, world!", "Unexpected character 'H' located on line 1 column 1");
  ASSERT_XML_ERR(arena, "<?>", "Unexpected character '>' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "<?xml )>", "Unexpected character ')' located on line 1 column 7");
  ASSERT_XML_ERR(arena, "<\?\?>", "Unexpected character '?' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "<? ?>", "Unexpected character ' ' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "<? xml?>", "Unexpected character ' ' located on line 1 column 3");
  ASSERT_XML_ERR(arena, "<?xml? <foo/>", "Unexpected character ' ' located on line 1 column 7");
  ASSERT_XML_ERR(arena, "<?alpha>", "Unexpected character '>' located on line 1 column 8");
  ASSERT_XML_ERR(arena, "<?alpha >", "Unexpected character '>' located on line 1 column 9");
  ASSERT_XML_ERR(arena, "<?xml alpha)?>", "Unexpected character ')' located on line 1 column 12");
}