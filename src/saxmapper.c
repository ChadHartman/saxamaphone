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

static const sax_decoder_t sax_default_encoders[SAXAMAPHONE_FIELD_TYPE_MAX] = {
    NULL,
    NULL,
    sax_decode_float,
    NULL,
    sax_decode_string,
    NULL};

typedef struct sax_mapper_t {
  sax_parser_t *parser;
  void *alloc_ctx;
  void *(*alloc)(void *, void *, size_t);
  char *err;
  sax_map_opts_t opts;
} sax_mapper_t;

bool sax_endswith(const char *restrict subject, const char *restrict suffix);

static char *sax_mapper_error(
    sax_mapper_t *restrict mapper,
    const char *restrict fmt,
    ...) {

  va_list args;
  va_start(args, fmt);

  const size_t len = vsnprintf(NULL, 0, fmt, args);
  mapper->err = mapper->alloc(mapper->alloc_ctx, NULL, len + 1);
  if (mapper->err == NULL) {
    SAXAMAPHONE_LOG("Failed to set error; allocator returned NULL when requesting %zu bytes for format \"%s\"", len + 1, fmt);
    va_end(args);
    return NULL;
  }
  vsprintf(mapper->err, fmt, args);
  va_end(args);
  return mapper->err;
}

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

static bool sax_field_set_attr(
    sax_mapper_t *restrict mapper,
    const sax_field_t *restrict field,
    void *value,
    const char *restrict serialized) {

  sax_decoder_t decoder = mapper->opts.decoders[field->type] == NULL
                              ? sax_default_encoders[field->type]
                              : mapper->opts.decoders[field->type];

  if (decoder == NULL) {
    SAXAMAPHONE_LOG("Skipped setting type %d \"%s\" with \"%s\", no encoder was found",
                    field->type,
                    field->name,
                    serialized);

    // TODO return false
    return true;
  }

  sax_decode_ctx_t ctx = {
      .alloc = mapper->alloc,
      .alloc_ctx = mapper->alloc_ctx,
      .encoded = serialized,
      .value = value,
  };

  if (decoder(&ctx)) {
    return true;
  }

  sax_mapper_error(mapper, "Failed to set \"%s\" with \"%s\"", field->name, serialized);
  return false;

  return true;
}

static bool sax_mapper_deserialize_attrs(
    sax_mapper_t *restrict mapper,
    const sax_field_t *restrict schema,
    uint8_t *restrict value) {

  if (schema == NULL || value == NULL) {
    return true;
  }

  for (const sax_attr_t *i = sax_attrs(mapper->parser); i != NULL; i = i->next) {
    const sax_field_t *restrict field = sax_field(schema, i->name);
    if (field == NULL) {
      continue;
    }
    sax_field_set_attr(mapper, field, value + field->offset, i->value);
  }

  return true;
}

static bool sax_mapper_deserialize(
    sax_mapper_t *restrict mapper,
    const sax_field_t *restrict schema,
    uint8_t *restrict value) {

  sax_event_t ev = SAX_EVENT_ERROR;

  for (ev = sax_next(mapper->parser);
       ev != SAX_EVENT_END_DOCUMENT && ev != SAX_EVENT_ERROR;
       ev = sax_next(mapper->parser)) {

    if (ev == SAX_EVENT_PROCESSING_INSTRUCTION) {
      continue;
    }

    if (ev == SAX_EVENT_START_TAG) {

      const char *restrict tag = sax_tag(mapper->parser);
      SAXAMAPHONE_LOG("SAX_EVENT_START_TAG: \"%s\"", tag);

      const sax_field_t *restrict field = sax_field(schema, tag);
      SAXAMAPHONE_LOG("Selected field \"%s\"", field == NULL ? "NULL" : field->name);

      const sax_field_t *child_schema = field == NULL ? NULL : field->sub_schema;
      uint8_t *child_value = field == NULL ? NULL : (value == NULL ? NULL : value + field->offset);

      const bool res = sax_mapper_deserialize_attrs(mapper, child_schema, child_value) &&
                       sax_mapper_deserialize(mapper, child_schema, child_value);

      if (!res) {
        return false;
      }
    }

    if (ev == SAX_EVENT_CONTENT) {
      SAXAMAPHONE_LOG("SAX_EVENT_CONTENT: \"%s\"", sax_content(mapper->parser));
    }

    if (ev == SAX_EVENT_END_TAG) {
      SAXAMAPHONE_LOG("SAX_EVENT_END_TAG: \"%s\"", sax_tag(mapper->parser));
      return true;
    }
  }

  if (ev == SAX_EVENT_ERROR) {
    sax_mapper_error(mapper, "%s", sax_error(mapper->parser));
    return false;
  }

  return true;
}

void sax_alloc(
    sax_parser_t *restrict parser,
    void **alloc_ctx,
    void *(**alloc)(void *, void *, size_t));

bool sax_decode(
    sax_parser_t *restrict parser,
    const sax_field_t *restrict schema,
    void *restrict value,
    const sax_map_opts_t *restrict opts,
    char **errmsg) {

  void *alloc_ctx = NULL;
  void *(*alloc)(void *, void *, size_t) = NULL;

  sax_alloc(parser, &alloc_ctx, &alloc);

  if (parser == NULL || alloc == NULL) {
    SAXAMAPHONE_LOG("sax_deserialize failed; invalid parser");
    return false;
  }

  sax_mapper_t mapper = {
      .alloc = alloc,
      .alloc_ctx = alloc_ctx,
      .parser = parser,
      .opts = opts == 0 ? (sax_map_opts_t){0} : *opts,
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

  bool res = sax_mapper_deserialize(&mapper, schema, value);
  if (errmsg) {
    *errmsg = mapper.err;
  }

  return res;
}

bool sax_decode_float(sax_decode_ctx_t *restrict ctx) {

  *(float *)ctx->value = strtof(ctx->encoded, NULL) *
                         (sax_endswith(ctx->encoded, "%%") ? 0.01f : 1.0f);

  return true;
}

bool sax_decode_string(sax_decode_ctx_t *restrict ctx) {

  const size_t len = strlen(ctx->encoded);
  char **out = ctx->value;
  *out = ctx->alloc(ctx->alloc_ctx, NULL, len + 1);
  if (*out == NULL) {
    SAXAMAPHONE_LOG("Failed to set \"%s\"; alloc returned NULL", ctx->encoded);
    return false;
  }

  memcpy(*out, ctx->encoded, len + 1);

  return true;
}