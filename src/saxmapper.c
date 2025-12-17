#include "saxmapper.h"

struct sax_mapper_t {
  sax_parser_t *parser;
};

sax_mapper_t *sax_map(sax_parser_t *restrict parser) {

  (void)parser;

  //   if (parser == NULL) {
  return NULL;
  //   }
}

bool sax_map_bool(
    sax_mapper_t *restrict mapper,
    const char *restrict name,
    sax_map_field_t field,
    bool *restrict value) {
  (void)mapper;
  (void)name;
  (void)field;
  (void)value;
  return true;
}

bool sax_map_float(
    sax_mapper_t *restrict mapper,
    const char *restrict name,
    sax_map_field_t field,
    float *restrict value) {
  (void)mapper;
  (void)name;
  (void)field;
  (void)value;
  return true;
}

bool sax_map_size(
    sax_mapper_t *restrict mapper,
    const char *restrict name,
    sax_map_field_t field,
    size_t *restrict value) {
  (void)mapper;
  (void)name;
  (void)field;
  (void)value;
  return true;
}

bool sax_map_string(
    sax_mapper_t *restrict mapper,
    const char *restrict name,
    sax_map_field_t field,
    char **restrict value) {
  (void)mapper;
  (void)name;
  (void)field;
  (void)value;
  return true;
}
