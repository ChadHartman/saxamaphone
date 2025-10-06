#include <assert.h>
#include <saxamaphone.h>
#include <stdlib.h> // EXIT_SUCCESS
#include <string.h> // strcmp

int main() {

  uint8_t buf[2048];

  // Works with `.path` as well but that will result in an `fopen` call which
  //   is an implicit allocation
  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .buf = buf,
      .buf_size = sizeof(buf),
      .xml = "<hello lang=\"en\">world!</hello>",
  });

  assert(SAX_EVENT_START_TAG == sax_next(parser));
  assert(0 == strcmp("hello", sax_tag(parser)));
  assert(0 == strcmp("en", sax_attr(parser, "lang")));

  assert(SAX_EVENT_CONTENT == sax_next(parser));
  assert(0 == strcmp("world!", sax_tag(parser)));

  assert(SAX_EVENT_END_TAG == sax_next(parser));
  assert(0 == strcmp("hello", sax_tag(parser)));

  assert(SAX_EVENT_END_DOCUMENT == sax_next(parser));

  // Technically unnecessary; but will close a file if `.path` was provided
  sax_parser_free(parser);

  return EXIT_SUCCESS;
}