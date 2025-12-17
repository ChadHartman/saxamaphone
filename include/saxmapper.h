#ifndef SAXMAPPER_H
#define SAXMAPPER_H

#include "saxamaphone.h"

typedef enum {
  SAX_TYPE_ARRAY,
  SAX_TYPE_BOOL,
  SAX_TYPE_FLOAT,
  SAX_TYPE_SIZE,
  SAX_TYPE_STRING
} sax_type_t;

typedef struct sax_field_t {
  const char *name;
  sax_type_t type;
  size_t offset;
  bool optional;
  /// @param If type is TYPE_OBJECT, point to the schema for that sub-struct
  const struct sax_field_t *sub_schema;
} sax_field_t;

typedef struct sax_mapper_t sax_mapper_t;

sax_mapper_t *sax_mapper(sax_parser_t *restrict parser);

void sax_mapper_free(sax_mapper_t *restrict mapper);

#endif