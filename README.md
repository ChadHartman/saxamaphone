# Saxamaphone

Saxamaphone is a C, SAX-style (Simple API for XML) XML parser with the following features:

* Event-driven model (no callbacks)
* No-allocator capabilities
* c99 minimum standard
* No dependencies
* Simple import
    * Drag and drop `saxamaphone.h` & `saxamaphone.c`
    * Meson import
* Robust test coverage
    * Lines: 98.0%
    * Functions: 100.0%
    * Branches: 97.0%

and unsupported XML features:

* Non UTF-8 support
* Validation
    * Only for well-formed tags; not tag ordering
* Processing instruction interpretation
    * Events are still generated
    * Processing instructions are not consumed in any way
* Namespace interpretation
    * e.g. when calling `sax_tag` on `<app:value>`; "app:value" will be returned
    

## Sample Usages

### XML String

```c
#include <assert.h>
#include <saxamaphone.h>
#include <stdlib.h> // EXIT_SUCCESS
#include <string.h> // strcmp

int main() {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
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

  return EXIT_SUCCESS;
}
```

### XML File

`../example/example.xml`

```xml
<?xml version="1.0" encoding="UTF-8"?>
<hello lang="en">world!</hello>
```

```c
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
```

### Custom Allocator 

```c
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
```

### No-Allocator

```c
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
```

## Integration

`./includes/saxamaphone.h` & `./src/saxamaphone.c` may be dropped into a project, or meson may be used as well.