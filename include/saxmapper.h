#ifndef SAXMAPPER_H
#define SAXMAPPER_H

#include "saxamaphone.h"

#ifndef SAXAMAPHONE_FIELD_TYPE_MAX
#define SAXAMAPHONE_FIELD_TYPE_MAX 16
#endif

typedef enum {
  SAX_TYPE_NONE,
  SAX_TYPE_ARRAY,
  SAX_TYPE_BOOL,
  SAX_TYPE_FLOAT,
  SAX_TYPE_SIZE,
  SAX_TYPE_STRING,
  SAX_TYPE_STRUCT,
} sax_field_type_t;

/// @brief SAX Mapping field descriptor
typedef struct sax_field_t {

  /// @brief Field's name which correlates with an XML tag or Attribute
  const char *name;

  /// @brief The C datatype to use
  sax_field_type_t type;

  /// @brief The offset of the field in the provided struct; this is retrieved using @see offsetof
  size_t offset;

  /// @brief Sub schema to use with ARRAY and OBJECT types
  const struct sax_field_t *sub_schema;

  /// @brief When type is @see SAX_TYPE_ARRAY, this method will be called for children by passing in the parent
  /// @param alloc_ctx the allocator context to pass in as the first argument to the allocator
  /// @param alloc the allocator function
  /// @param parent the parent object which holds the array
  void *(*arr_append)(void *, void *(*)(void *, void *, size_t), void *);

} sax_field_t;

typedef struct sax_decode_ctx_t {
  void *(*alloc)(void *, void *, size_t);
  void *alloc_ctx;
  const sax_field_t *field;
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