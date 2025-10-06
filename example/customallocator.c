#include <assert.h>
#include <saxamaphone.h>
#include <stdlib.h> // malloc, realloc, free
#include <string.h> // strcmp

/// @brief Placeholder to demonstrate how the context is used
typedef struct allocator_t {
  int64_t total;
  int64_t live;
} allocator_t;

static void *custom_alloc(void *ud, void *ptr, size_t size) {

  allocator_t *restrict alloc = ud;

  if (size == 0) {
    --(alloc->live);
    free(ptr);
    return NULL;
  }

  if (ptr == NULL) {
    ++(alloc->live);
    ++(alloc->total);
    return malloc(size);
  }

  return realloc(ptr, size);
}

int main() {

  allocator_t allocator = {0};

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = custom_alloc,
      .alloc_ctx = &allocator,
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

  sax_parser_free(parser);
  assert(2 == allocator.total);
  assert(0 == allocator.live);

  return EXIT_SUCCESS;
}