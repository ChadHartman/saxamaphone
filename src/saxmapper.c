#include <stdarg.h> // va_list
#include <stdio.h>  // vsnprintf

#include "saxmapper.h"

/// @brief Simple logger
#ifdef SAXAMAPHONE_DEBUG
#define SAXAMAPHONE_LOG(...)                      \
  printf("\x1b[36m"                               \
         "[SAXAMAPHONE] %s:%d "                   \
         "\x1b[0m",                               \
         (strrchr(__FILE__, '/') + 1), __LINE__); \
  printf(__VA_ARGS__);                            \
  printf("\n")
#else
#define SAXAMAPHONE_LOG(...) ((void)0)
#endif

struct sax_mapper_t {
  sax_parser_t *parser;
  void *alloc_ctx;
  void *(*alloc)(void *, void *, size_t);
  char *err;
};

// static void sax_mapper_set_error(
//     sax_mapper_t *restrict mapper,
//     const char *restrict fmt,
//     ...) {

//   va_list args;
//   va_start(args, fmt);

//   const size_t len = vsnprintf(NULL, 0, fmt, args);
//   mapper->alloc(mapper->alloc_ctx, mapper->err, 0);
//   mapper->err = mapper->alloc(mapper->alloc_ctx, NULL, len + 1);
//   if (mapper->err == NULL) {
//     SAXAMAPHONE_LOG("Failed to set error; allocator returned NULL when requesting %zu bytes for format \"%s\"", len + 1, fmt);
//     va_end(args);
//     return;
//   }
//   vsprintf(mapper->err, fmt, args);
//   va_end(args);
// }

void sax_alloc(
    sax_parser_t *restrict parser,
    void **alloc_ctx,
    void *(**alloc)(void *, void *, size_t));

sax_mapper_t *sax_mapper(sax_parser_t *restrict parser) {

  void *alloc_ctx = NULL;
  void *(*alloc)(void *, void *, size_t) = NULL;

  sax_alloc(parser, &alloc_ctx, &alloc);

  if (parser == NULL || alloc == NULL) {
    SAXAMAPHONE_LOG("Cannot create mapper; invalid parser");
    return NULL;
  }

  sax_mapper_t *restrict mapper = alloc(alloc_ctx, NULL, sizeof(mapper));
  if (mapper == NULL) {
    SAXAMAPHONE_LOG("Failed to create mapper; allocator returned NULL");
    return NULL;
  }

  *mapper = (sax_mapper_t){
      .alloc = alloc,
      .alloc_ctx = alloc_ctx,
      .parser = parser,
  };

  return mapper;
}

void sax_mapper_free(sax_mapper_t *restrict mapper) {

  if (mapper == NULL) {
    return;
  }

  mapper->alloc(mapper->alloc_ctx, mapper->err, 0);
  mapper->alloc(mapper->alloc_ctx, mapper, 0);
}