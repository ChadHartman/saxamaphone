#ifndef SAXMAPPER_H
#define SAXMAPPER_H

#include "saxamaphone.h"

typedef enum {
  SAX_TYPE_ARRAY,
  SAX_TYPE_BOOL,
  SAX_TYPE_FLOAT,
  SAX_TYPE_SIZE,
  SAX_TYPE_STRING,
  SAX_TYPE_STRUCT,
} sax_type_t;

typedef struct sax_field_t {
  const char *name;
  sax_type_t type;
  size_t offset;
  /// @param If type is TYPE_OBJECT, point to the schema for that sub-struct
  const struct sax_field_t *sub_schema;
} sax_field_t;

bool sax_deserialize(
    sax_parser_t *restrict parser,
    const sax_field_t *restrict schema,
    void *restrict value,
    char **errmsg);

#endif