#ifndef SAXMAPPER_H
#define SAXMAPPER_H

#include "saxamaphone.h"

#ifndef SAXAMAPHONE_FIELD_TYPE_MAX
#define SAXAMAPHONE_FIELD_TYPE_MAX 16
#endif

typedef enum {
  SAX_TYPE_ARRAY,
  SAX_TYPE_BOOL,
  SAX_TYPE_FLOAT,
  SAX_TYPE_SIZE,
  SAX_TYPE_STRING,
  SAX_TYPE_STRUCT,
} sax_field_type_t;

typedef struct sax_field_t {
  const char *name;
  sax_field_type_t type;
  size_t offset;
  /// @param If type is TYPE_OBJECT, point to the schema for that sub-struct
  const struct sax_field_t *sub_schema;
} sax_field_t;

typedef struct sax_decode_ctx_t {
  void *(*alloc)(void *, void *, size_t);
  void *alloc_ctx;
  void *value;
  const char *encoded;
} sax_decode_ctx_t;

typedef bool (*sax_decoder_t)(sax_decode_ctx_t *restrict);

typedef struct sax_map_opts_t {
  sax_decoder_t decoders[SAXAMAPHONE_FIELD_TYPE_MAX];
} sax_map_opts_t;

bool sax_decode(
    sax_parser_t *restrict parser,
    const sax_field_t *restrict schema,
    void *restrict value,
    const sax_map_opts_t *restrict opts,
    char **errmsg);

bool sax_decode_bool(sax_decode_ctx_t *restrict ctx);

bool sax_decode_float(sax_decode_ctx_t *restrict ctx);

bool sax_decode_string(sax_decode_ctx_t *restrict ctx);

#endif