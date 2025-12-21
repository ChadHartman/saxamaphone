#include <ctype.h>
#include <errno.h>
#include <stdarg.h> // va_list
#include <stdio.h>  // vsnprintf
#include <stdlib.h> // atof
#include <string.h> // strcmp

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

#define SAX_DECODE_INT(type_pfx, max)                                           \
  bool sax_decode_##type_pfx(const sax_decode_ctx_t *restrict ctx, void *out) { \
    char *sfx = NULL;                                                           \
    long long val = strtoll(ctx->encoded, &sfx, 10);                            \
    errno = 0;                                                                  \
    if (ctx->encoded == sfx || errno == ERANGE || val > max) {                  \
      return false;                                                             \
    }                                                                           \
    *(type_pfx##_t *)out = (type_pfx##_t)val;                                   \
    return true;                                                                \
  }

#define SAX_DECODE_UINT(type_pfx, max)                                          \
  bool sax_decode_##type_pfx(const sax_decode_ctx_t *restrict ctx, void *out) { \
    char *sfx = NULL;                                                           \
    unsigned long val = strtoul(ctx->encoded, &sfx, 10);                        \
    errno = 0;                                                                  \
    if (ctx->encoded == sfx || errno == ERANGE || val > max) {                  \
      return false;                                                             \
    }                                                                           \
    *(type_pfx##_t *)out = (type_pfx##_t)val;                                   \
    return true;                                                                \
  }

static const sax_decoder_t sax_default_encoders[64] = {
    NULL,
    sax_decode_bool,
    sax_decode_double,
    sax_decode_float,
    sax_decode_int8,
    sax_decode_int16,
    sax_decode_int32,
    sax_decode_int64,
    sax_decode_size,
    sax_decode_string,
    sax_decode_uint8,
    sax_decode_uint16,
    sax_decode_uint32,
    sax_decode_uint64,
    NULL};

typedef struct sax_mapper_t {
  sax_parser_t *parser;
  char *err;
  sax_map_opts_t opts;
  char *out_buf;
  size_t out_buf_size;
  size_t out_buf_offset;
} sax_mapper_t;

bool sax_endswith(const char *restrict subject, const char *restrict suffix);

void sax_alloc(
    sax_parser_t *restrict parser,
    void **alloc_ctx,
    void *(**alloc)(void *, void *, size_t));

static void *sax_sys_alloc(void *ctx, void *ptr, size_t size) {

  (void)ctx;

  if (size == 0) {
    free(ptr);
    return NULL;
  }

  return ptr == NULL ? malloc(size) : realloc(ptr, size);
}

static uint8_t *sax_mapper_child_value(
    sax_mapper_t *restrict mapper,
    const sax_field_t *restrict field,
    uint8_t *restrict parent) {

  if (field == NULL || parent == NULL) {
    return NULL;
  }

  return field->getter == NULL
             ? parent + field->offset
             : field->getter(mapper->opts.alloc_ctx, mapper->opts.alloc, parent);
}

/// @brief Set the error message
/// @param mapper instance
/// @param fmt format to use
/// @param args format args
/// @return the created message
static char *sax_mapper_error(
    sax_mapper_t *restrict mapper,
    const char *restrict fmt,
    ...) {

  va_list args;
  va_start(args, fmt);

  const size_t len = vsnprintf(NULL, 0, fmt, args);
  mapper->err = mapper->opts.alloc(mapper->opts.alloc_ctx, NULL, len + 1);
  if (mapper->err == NULL) {
    SAXAMAPHONE_LOG("Failed to set error; allocator returned NULL when requesting %zu bytes for format \"%s\"", len + 1, fmt);
    va_end(args);
    return NULL;
  }
  vsprintf(mapper->err, fmt, args);
  va_end(args);
  return mapper->err;
}

/// @brief Retrieve a field using the provided name
/// @param schema list of fields to use
/// @param name name of the field to find
/// @return the found field or NULL if not found
static const sax_field_t *sax_field(
    const sax_field_t *restrict schema,
    const char *restrict name) {

  if (schema == NULL || name == NULL) {
    return NULL;
  }

  for (const sax_field_t *i = schema; i->name != NULL; ++i) {
    if (strcmp(name, i->name) == 0) {
      return i;
    }
  }

  return NULL;
}

/// @brief Decode a value using a field
/// @param mapper instance
/// @param field to use
/// @param value to write
/// @param encoded value to decode
/// @return true if no errors occurred
static bool sax_mapper_decode_w_field(
    sax_mapper_t *restrict mapper,
    const sax_field_t *restrict field,
    void *value,
    const char *restrict encoded) {

  if (field == NULL || value == NULL) {
    // Just traversing unmapped fields
    return true;
  }

  sax_decoder_t decoder = mapper->opts.decoders[field->type] == NULL
                              ? sax_default_encoders[field->type]
                              : mapper->opts.decoders[field->type];

  if (decoder == NULL) {
    sax_mapper_error(mapper, "No decoder set for attribute \"%s\" with value \"%s\"", field->name, encoded);
    return false;
  }

  sax_decode_ctx_t ctx = {
      .alloc = mapper->opts.alloc,
      .alloc_ctx = mapper->opts.alloc_ctx,
      .encoded = encoded,
  };

  if (decoder(&ctx, value)) {
    return true;
  }

  sax_mapper_error(mapper, "Failed to set \"%s\" with \"%s\"; decoder returned false", field->name, encoded);
  return false;
}

/// @brief Traverse XML Attributes and decode their values
/// @param mapper instance
/// @param schema fields to use
/// @param value parent object whose fields are to be set
/// @return true if no error occurred
static bool sax_mapper_decode_attrs(
    sax_mapper_t *restrict mapper,
    const sax_field_t *restrict schema,
    uint8_t *restrict value) {

  if (schema == NULL || value == NULL) {
    // Just traversing unmapped XML
    return true;
  }

  for (const sax_attr_t *i = sax_attrs(mapper->parser); i != NULL; i = i->next) {
    const sax_field_t *restrict field = sax_field(schema, i->name);
    if (field == NULL) {
      continue;
    }
    if (!sax_mapper_decode_w_field(mapper, field, value + field->offset, i->value)) {
      return false;
    }
  }

  return true;
}

/// @brief Decode an XML Element with a mapper
/// @param mapper instance
/// @param tag the tag of the element in the current scope
/// @param schema to use
/// @param value to decode
/// @return true if no errors occurred
static bool sax_mapper_decode(
    sax_mapper_t *restrict mapper,
    const sax_field_t *restrict schema,
    uint8_t type,
    uint8_t *restrict value) {

  sax_event_t ev = SAX_EVENT_ERROR;

  for (ev = sax_next(mapper->parser);
       ev != SAX_EVENT_END_DOCUMENT && ev != SAX_EVENT_ERROR;
       ev = sax_next(mapper->parser)) {

    if (ev == SAX_EVENT_PROCESSING_INSTRUCTION) {
      continue;
    }

    if (ev == SAX_EVENT_START_TAG) {

      const char *restrict child_tag = sax_tag(mapper->parser);
      const sax_field_t *restrict field = sax_field(schema, child_tag);
      const sax_field_t *restrict child_schema = field == NULL ? NULL : field->schema;
      const uint8_t child_type = field == NULL ? SAX_TYPE_NONE : field->type;
      uint8_t *child_value = sax_mapper_child_value(mapper, field, value);
      const bool res = sax_mapper_decode_attrs(mapper, child_schema, child_value) &&
                       sax_mapper_decode(mapper, child_schema, child_type, child_value);

      if (!res) {
        return false;
      }
    }

    if (ev == SAX_EVENT_CONTENT) {
      const char *restrict content = sax_content(mapper->parser);
      const sax_field_t *restrict lookup = sax_field(schema, SAX_CONTENT);
      sax_field_t field = lookup == NULL
                              ? (sax_field_t){.type = type}
                              : *lookup;

      if (!sax_mapper_decode_w_field(mapper, &field, value + field.offset, content)) {
        return false;
      }
    }

    if (ev == SAX_EVENT_END_TAG) {
      return true;
    }
  }

  if (ev == SAX_EVENT_ERROR) {
    sax_mapper_error(mapper, "%s", sax_error(mapper->parser));
    return false;
  }

  return true;
}

static bool sax_mapper_encode(
    sax_mapper_t *restrict mapper,
    const sax_field_t *restrict schema,
    const uint8_t *restrict value) {

  for (const sax_field_t *i = schema; i->name != NULL; ++i) {

    if (i->schema != NULL) {
      sax_mapper_encode(mapper, i->schema, value + i->offset);
      continue;
    }
  }

  return true;
}

bool sax_decode(
    sax_parser_t *restrict parser,
    const sax_field_t *restrict schema,
    void *restrict value,
    const sax_map_opts_t *restrict opts,
    char **errmsg) {

  sax_map_opts_t config = opts == NULL ? (sax_map_opts_t){0} : *opts;

  if (config.alloc == NULL) {
    sax_alloc(parser, &config.alloc_ctx, &config.alloc);
  }

  if (parser == NULL || config.alloc == NULL) {
    SAXAMAPHONE_LOG("sax_deserialize failed; invalid parser");
    return false;
  }

  sax_mapper_t mapper = {
      .parser = parser,
      .opts = config,
  };

  if (schema == NULL) {
    SAXAMAPHONE_LOG("sax_deserialize failed; schema was NULL");
    if (errmsg) {
      *errmsg = sax_mapper_error(&mapper, "sax_deserialize failed; schema was NULL");
    }
    return false;
  }

  if (value == NULL) {
    SAXAMAPHONE_LOG("sax_deserialize failed; value was NULL");
    if (errmsg) {
      *errmsg = sax_mapper_error(&mapper, "sax_deserialize failed; value was NULL");
    }
    return false;
  }

  bool res = sax_mapper_decode(&mapper, schema, SAX_TYPE_NONE, value);
  if (errmsg == NULL) {
    mapper.opts.alloc(mapper.opts.alloc_ctx, mapper.err, 0);
  } else {
    *errmsg = mapper.err;
  }

  return res;
}

bool sax_decode_bool(const sax_decode_ctx_t *restrict ctx, void *out) {

  bool *decoded = out;
  const size_t len = strlen(ctx->encoded);
  if (len < 4) {
    *decoded = false;
    return true;
  }

  *decoded = tolower(ctx->encoded[0]) == 't' &&
             tolower(ctx->encoded[1]) == 'r' &&
             tolower(ctx->encoded[2]) == 'u' &&
             tolower(ctx->encoded[3]) == 'e' &&
             ctx->encoded[4] == 0;

  return true;
}

bool sax_decode_double(const sax_decode_ctx_t *restrict ctx, void *out) {
  char *sfx = NULL;
  double res = strtod(ctx->encoded, &sfx);
  if (ctx->encoded == sfx || errno == ERANGE) {
    return false;
  }
  res = sfx != NULL && sfx[0] == '%' ? res / 100.0 : res;
  *(double *)out = res;
  return true;
}

bool sax_decode_float(const sax_decode_ctx_t *restrict ctx, void *out) {

  char *sfx = NULL;
  float res = strtod(ctx->encoded, &sfx);
  errno = 0;
  if (ctx->encoded == sfx || errno == ERANGE) {
    return false;
  }

  res = sfx != NULL && sfx[0] == '%' ? res / 100.0f : res;
  *(float *)out = res;
  return true;
}

SAX_DECODE_INT(int8, INT8_MAX)
SAX_DECODE_INT(int16, INT16_MAX)
SAX_DECODE_INT(int32, INT32_MAX)
SAX_DECODE_INT(int64, INT64_MAX)
SAX_DECODE_UINT(size, SIZE_MAX)

bool sax_decode_string(const sax_decode_ctx_t *restrict ctx, void *out) {

  const size_t len = strlen(ctx->encoded);
  char **res = out;
  *res = ctx->alloc(ctx->alloc_ctx, NULL, len + 1);
  if (*res == NULL) {
    SAXAMAPHONE_LOG("Failed to set \"%s\"; alloc returned NULL", ctx->encoded);
    return false;
  }

  memcpy(*res, ctx->encoded, len + 1);

  return true;
}

SAX_DECODE_UINT(uint8, UINT8_MAX)
SAX_DECODE_UINT(uint16, UINT16_MAX)
SAX_DECODE_UINT(uint32, UINT32_MAX)
SAX_DECODE_UINT(uint64, UINT64_MAX)

bool sax_encode(
    const sax_field_t *restrict schema,
    const void *restrict value,
    const sax_map_opts_t *restrict opts,
    char **out) {

  sax_mapper_t mapper = {
      .opts = opts == NULL ? (sax_map_opts_t){0} : *opts,
  };

  if (mapper.opts.alloc == NULL) {
    mapper.opts.alloc = sax_sys_alloc;
  }

  // TODO: validate

  const bool res = sax_mapper_encode(&mapper, schema, value);
  if (res) {
    *out = mapper.out_buf;
  };

  return res;
}