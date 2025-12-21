#ifndef SAXMAPPER_H
#define SAXMAPPER_H

#include "saxamaphone.h"

#define SAX_CONTENT "@content"
#define SAX_TYPE_NONE 0
#define SAX_TYPE_BOOL 1
#define SAX_TYPE_DOUBLE 2
#define SAX_TYPE_FLOAT 3
#define SAX_TYPE_INT8 4
#define SAX_TYPE_INT16 5
#define SAX_TYPE_INT32 6
#define SAX_TYPE_INT64 7
#define SAX_TYPE_SIZE 8
#define SAX_TYPE_STRING 9
#define SAX_TYPE_UINT8 10
#define SAX_TYPE_UINT16 11
#define SAX_TYPE_UINT32 12
#define SAX_TYPE_UINT64 13

/// @brief SAX Mapping field descriptor
typedef struct sax_field_t {

  /// @brief Field's name which correlates with an XML tag or Attribute
  const char *name;

  /// @brief The C datatype to use
  uint8_t type;

  /// @brief The offset of the field in the provided struct; this is retrieved using @see offsetof
  size_t offset;

  /// @brief Sub schema to use with ARRAY and OBJECT types
  const struct sax_field_t *schema;

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
  const char *encoded;
} sax_decode_ctx_t;

typedef bool (*sax_decoder_t)(const sax_decode_ctx_t *restrict, void *);

typedef struct sax_map_opts_t {
  sax_decoder_t decoders[UINT8_MAX];
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