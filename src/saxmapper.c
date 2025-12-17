#include "saxmapper.h"

struct sax_mapper_t {
  sax_parser_t *parser;
};

sax_mapper_t *sax_mapper(sax_parser_t *restrict parser) {

  (void)parser;

  //   if (parser == NULL) {
  return NULL;
  //   }
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
