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
};

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

bool sax_map_bool(
    sax_mapper_t *restrict mapper,
    const char *restrict name,
    sax_map_field_t field,
    bool required,
    bool *restrict value) {
  (void)mapper;
  (void)name;
  (void)field;
  (void)required;
  (void)value;
  return true;
}

bool sax_map_float(
    sax_mapper_t *restrict mapper,
    const char *restrict name,
    sax_map_field_t field,
    bool required,
    float *restrict value) {
  (void)mapper;
  (void)name;
  (void)field;
  (void)required;
  (void)value;
  return true;
}

bool sax_map_size(
    sax_mapper_t *restrict mapper,
    const char *restrict name,
    sax_map_field_t field,
    bool required,
    size_t *restrict value) {
  (void)mapper;
  (void)name;
  (void)field;
  (void)required;
  (void)value;
  return true;
}

bool sax_map_string(
    sax_mapper_t *restrict mapper,
    const char *restrict name,
    sax_map_field_t field,
    bool required,
    char **restrict value) {
  (void)mapper;
  (void)name;
  (void)field;
  (void)required;
  (void)value;
  return true;
}

void sax_mapper_free(sax_mapper_t *restrict mapper) {

  if (mapper == NULL) {
    return;
  }

  mapper->alloc(mapper->alloc_ctx, mapper, 0);
}