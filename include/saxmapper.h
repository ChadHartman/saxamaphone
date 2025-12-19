#ifndef SAXMAPPER_H
#define SAXMAPPER_H

#include "saxamaphone.h"

#ifndef SAXAMAPHONE_FIELD_TYPE_MAX
#define SAXAMAPHONE_FIELD_TYPE_MAX 32
#endif

typedef enum {
  SAX_TYPE_NONE,
  SAX_TYPE_ARRAY,
  SAX_TYPE_BOOL,
  SAX_TYPE_DOUBLE,
  SAX_TYPE_FLOAT,
  SAX_TYPE_INT8,
  SAX_TYPE_INT16,
  SAX_TYPE_INT32,
  SAX_TYPE_INT64,
  SAX_TYPE_SIZE,
  SAX_TYPE_STRING,
  SAX_TYPE_STRUCT,
  SAX_TYPE_UINT8,
  SAX_TYPE_UINT16,
  SAX_TYPE_UINT32,
  SAX_TYPE_UINT64,
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

  /// @brief Alternative to the @see sax_field_t::offset field. This is called
  ///   first when non-null. This is also the mechanism to use when appending
  ///   children to arrays or lists
  /// @param alloc_ctx the allocator context to pass in as the first argument
  ///   to the allocator
  /// @param alloc the allocator function
  /// @param parent the parent object which holds the array
  void *(*getter)(void *, void *(*)(void *, void *, size_t), void *);

} sax_field_t;

typedef struct sax_decode_ctx_t {
  void *(*alloc)(void *, void *, size_t);
  void *alloc_ctx;
  const sax_field_t *field;
  const char *encoded;
} sax_decode_ctx_t;

typedef bool (*sax_decoder_t)(const sax_decode_ctx_t *restrict, void *);

typedef struct sax_map_opts_t {
  sax_decoder_t decoders[SAXAMAPHONE_FIELD_TYPE_MAX];
} sax_map_opts_t;

bool sax_decode(
    sax_parser_t *restrict parser,
    const sax_field_t *restrict schema,
    void *restrict value,
    const sax_map_opts_t *restrict opts,
    char **errmsg);

bool sax_decode_bool(const sax_decode_ctx_t *restrict ctx, void *out);

bool sax_decode_double(const sax_decode_ctx_t *restrict ctx, void *out);

bool sax_decode_float(const sax_decode_ctx_t *restrict ctx, void *out);

bool sax_decode_int8(const sax_decode_ctx_t *restrict ctx, void *out);

bool sax_decode_int16(const sax_decode_ctx_t *restrict ctx, void *out);

bool sax_decode_int32(const sax_decode_ctx_t *restrict ctx, void *out);

bool sax_decode_int64(const sax_decode_ctx_t *restrict ctx, void *out);

bool sax_decode_size(const sax_decode_ctx_t *restrict ctx, void *out);

bool sax_decode_string(const sax_decode_ctx_t *restrict ctx, void *out);

bool sax_decode_uint8(const sax_decode_ctx_t *restrict ctx, void *out);

bool sax_decode_uint16(const sax_decode_ctx_t *restrict ctx, void *out);

bool sax_decode_uint32(const sax_decode_ctx_t *restrict ctx, void *out);

bool sax_decode_uint64(const sax_decode_ctx_t *restrict ctx, void *out);

#endif