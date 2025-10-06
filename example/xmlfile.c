#include <assert.h>
#include <saxamaphone.h>
#include <stdlib.h> // EXIT_SUCCESS
#include <string.h> // strcmp

int main() {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .path = "../example/example.xml",
  });

  assert(SAX_EVENT_PROCESSING_INSTRUCTION == sax_next(parser));
  assert(0 == strcmp("xml", sax_tag(parser)));
  assert(0 == strcmp("1.0", sax_attr(parser, "version")));
  assert(0 == strcmp("UTF-8", sax_attr(parser, "encoding")));

  assert(SAX_EVENT_START_TAG == sax_next(parser));
  assert(0 == strcmp("hello", sax_tag(parser)));
  assert(0 == strcmp("en", sax_attr(parser, "lang")));

  assert(SAX_EVENT_CONTENT == sax_next(parser));
  assert(0 == strcmp("world!", sax_tag(parser)));

  assert(SAX_EVENT_END_TAG == sax_next(parser));
  assert(0 == strcmp("hello", sax_tag(parser)));

  assert(SAX_EVENT_END_DOCUMENT == sax_next(parser));

  sax_parser_free(parser);

  return EXIT_SUCCESS;
}