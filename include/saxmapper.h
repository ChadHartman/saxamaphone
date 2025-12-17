#ifndef SAXMAPPER_H
#define SAXMAPPER_H

#include "saxamaphone.h"

typedef struct sax_mapper_t sax_mapper_t;

sax_mapper_t *sax_mapper(sax_parser_t *restrict parser);

void sax_mapper_free(sax_mapper_t *restrict mapper);

typedef enum {
  SAX_FIELD_ATTR,
  SAX_FIELD_NODE,
  SAX_FIELD_CONTENT,
} sax_map_field_t;

bool sax_map_bool(
    sax_mapper_t *restrict mapper,
    const char *restrict name,
    sax_map_field_t field,
    bool required,
    bool *restrict value);

bool sax_map_float(
    sax_mapper_t *restrict mapper,
    const char *restrict name,
    sax_map_field_t field,
    bool required,
    float *restrict value);

bool sax_map_size(
    sax_mapper_t *restrict mapper,
    const char *restrict name,
    sax_map_field_t field,
    bool required,
    size_t *restrict value);

bool sax_map_string(
    sax_mapper_t *restrict mapper,
    const char *restrict name,
    sax_map_field_t field,
    bool required,
    char **restrict value);

#endif