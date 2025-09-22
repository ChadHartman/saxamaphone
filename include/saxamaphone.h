#ifndef SAXAMAPHONE_H
#define SAXAMAPHONE_H

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint8_t

/// @brief Possible Saxamaphone event types
typedef enum {
  SAX_EVENT_START_ELEMENT = 1,
  SAX_EVENT_CONTENT,
  SAX_EVENT_END_ELEMENT,
  SAX_EVENT_END_DOCUMENT,
  SAX_EVENT_ERROR,
} sax_event_t;

/// @brief Saxamaphone parser instance
typedef struct sax_parser_t sax_parser_t;

/// @brief Saxamaphone parser configuration
typedef struct sax_config_t {

  /// @brief The file path to stream XML from
  const char *path;

  /// @brief The XML string to parse; when path is provided; this field is ignored
  const char *string;

  /// @brief Leave content leading and trailing spaces
  bool untrimmed_content;

  /// @brief When specified; neither alloc nor will the default allocator will be used. Buffer sized 4096 is recommended.
  uint8_t *buf;

  /// @brief Size of buf
  size_t buf_size;

  /// @brief Custom allocator to use instead of the default allocator.
  /// @details * When size is zero; ptr should be freed
  ///   * When ptr is NULL and size is non-zero; malloc should be returned
  ///   * When ptr is non-NULL and size is non-zero; realloc should be returned
  /// @param ctx provided alloc_ctx
  /// @param ptr to free or realloc
  /// @param size communicated the desired size; when zero ptr should be freed,
  ///   otherwise a malloc or realloc should be returned
  /// @return allocated block when size is greater than zero else NULL
  void *(*alloc)(void *ctx, void *ptr, size_t size);

  /// @brief First argument in the customer allocator
  void *alloc_ctx;

} sax_config_t;

/// @brief Saxamaphone XML Attribute
typedef struct sax_attr_t {

  /// @brief non-NULL Attribute
  char *name;

  /// @brief non-NULL Attribute value; may by empty string "" when a value is not provided
  char *value;

  /// @brief NULLable pointer to next attribute pair
  struct sax_attr_t *next;

} sax_attr_t;

/// @brief Initialize the thread-local parser (only 1 parser per thread can run
///   at a time). Initializing an in-progress parser will close resources and
///   initialize with the provided configuration.
/// @param config (non-NULL) configuration to use
/// @return Parser instance; or NULL due to configuration error
sax_parser_t *sax_parser(const sax_config_t *restrict config);

/// @brief Advance document iteration to the next event
/// @param  parser instance
/// @return next event
sax_event_t sax_next(sax_parser_t *restrict parser);

/// @brief Retrieve an error message for a @see SAX_EVENT_ERROR
/// @param parser instance
/// @return non-NULL the error string message or "" if not in error state
const char *sax_error(const sax_parser_t *restrict parser);

/// @brief Retrieve the tag for events @see SAX_EVENT_START_ELEMENT or
///   @see SAX_EVENT_END_ELEMENT
/// @param parser instance
/// @return  non-NULL tag name or "" if incorrect event
const char *sax_tag(const sax_parser_t *restrict parser);

/// @brief Retrieve the content for the @see SAX_EVENT_CONTENT event
/// @param parser instance
/// @return non-NULL content or "" if incorrect event
const char *sax_content(const sax_parser_t *restrict parser);

/// @brief Retrieve the XML attributes for the @see SAX_EVENT_START_ELEMENT
///   event
/// @param parser instance
/// @return NULLable attribute linked list
const sax_attr_t *sax_attrs(const sax_parser_t *restrict parser);

/// @brief Retrieve the XML attribute value associated with the provided name
/// @param parser instance
/// @param name to lookup
/// @return paired value or NULL if not found
const char *sax_attr(const sax_parser_t *restrict parser, const char *restrict name);

#ifdef SAXAMAPHONE_IMPLEMENTATION

#include <ctype.h>    // isspace
#include <inttypes.h> // PRIuFAST16
#include <stdarg.h>   // va_start
#include <stdio.h>    // fopen
#include <stdlib.h>   // malloc, realloc, free
#include <string.h>   // memset

#include <saxamaphone.h>

// TODO: test attrvalue  startswith `&`
// TODO: CDATA
// TODO: memory leaks

// #define SAXAMAPHONE_DEBUG

// === typedefs === //

typedef enum {
  SAX_STATE_INIT,
  SAX_STATE_IN_TAG,
  SAX_STATE_IN_START_TAG,
  SAX_STATE_CLOSING_START_TAG,
  SAX_STATE_ASSIGNING_ATTR_VALUE,
  SAX_STATE_IN_ESC_CHAR,
  SAX_STATE_IN_CONTENT,
  SAX_STATE_IN_END_TAG,
  SAX_STATE_START_TAG_SPACE,
  SAX_STATE_IN_ATTR_NAME,
  SAX_STATE_IN_ATTR_VALUE,
  SAX_STATE_IN_COMMENT,
  SAX_STATE_IN_PROC_INST,
  SAX_STATE_ERROR,
} sax_state_t;

typedef enum {
  SAX_ITER_FILE,
  SAX_ITER_STR,
} sax_iter_type_t;

typedef struct sax_allocator_t {
  uint8_t *bytes;
  size_t offset;
  size_t bytes_size;
} sax_allocator_t;

typedef struct sax_file_iter_t {
  FILE *fp;
  uint_fast32_t offset;
  uint_fast32_t bytes_read;
  uint8_t *file_buffer;
  size_t file_buffer_size;
} sax_file_iter_t;

typedef struct sax_str_iter_t {
  const char *value;
  uint_fast32_t offset;
  uint_fast32_t len;
} sax_str_iter_t;

typedef struct sax_iter_t {

  sax_iter_type_t type;
  /// @brief Largest code pt == 4; +1 null term
  char glyph[5];

  union {
    sax_file_iter_t file;
    sax_str_iter_t str;
  } impl;

} sax_iter_t;

typedef struct sax_arena_t {
  uint8_t *bytes;
  uint32_t bytes_size;
  uint32_t offset;
  void *(*alloc)(void *, void *, size_t);
  void *alloc_ctx;
} sax_arena_t;

struct sax_parser_t {

  // Configuration
  bool untrimmed_content;

  // Resources
  sax_arena_t arena;
  sax_iter_t iter;

  // Metadayas
  sax_state_t state;
  sax_state_t prev_state;
  uint_fast32_t line;
  uint_fast32_t column;

  // Workspace fields
  /// @brief Used for error (SAX_EVENT_ERROR), tag (SAX_EVENT_START_TAG), or content (SAX_EVENT_CONTENT)
  char *data;
  /// @brief Used to build the escaped glyph (for the content or attr value)
  char *escaped;
  sax_attr_t *attrs;
  sax_attr_t *current_attr;
};

// === constants === //

#define SAXAMAPHONE_EXCLUDE_TAG "!\"#$%&'()*+,/;<=>?@[\\]^`{|}~"
#define SAXAMAPHONE_EXCLUDE_TAG_PREFIX (SAXAMAPHONE_EXCLUDE_TAG ".-0123456789")

#define SAXAMAPHONE_SPACE \
  ' ' : case '\f':        \
  case '\n':              \
  case '\r':              \
  case '\t':              \
  case '\v'

// === private methods === //

#ifdef SAXAMAPHONE_DEBUG
#define SAXAMAPHONE_LOG(...)                      \
  printf("\x1b[36m"                               \
         "[SAXAMAPHONE] %s:%d "                   \
         "\x1b[0m",                               \
         (strrchr(__FILE__, '/') + 1), __LINE__); \
  printf(__VA_ARGS__)
#else
#define SAXAMAPHONE_LOG(...) ((void)0)
#endif

static void *sax_default_alloc(void *ctx, void *ptr, size_t size) {

  (void)ctx;

  if (size == 0) {
    free(ptr);
    return NULL;
  }

  if (ptr == NULL) {
    return malloc(size);
  }

  return realloc(ptr, size);
}

/// @brief Compute the file buffer size; it should be half the capacity and a power of 2
/// @param capacity total number of available bytes
/// @return the computed file buffer size
static size_t sax_file_buf_size(size_t capacity) {

  const size_t half = capacity / 2;
  // Min 32 arbitrarily chosen
  size_t buf_size = 32;
  while (buf_size * 2 < half) {
    buf_size = buf_size * 2;
  }
  return buf_size;
}

/// @brief Given the provided byte determine the UTF-8 code point size
/// @param byte byte value
/// @return size 1-4 if value; 0 if invalid
static uint_fast8_t sax_code_pt_size(uint8_t byte) {

  if (byte < 0x7F) {
    return 1;
  }

  if (0xC2 <= byte && byte <= 0xDF) {
    return 2;
  }

  if (0xE0 <= byte && byte <= 0xEF) {
    return 3;
  }

  if (0xF0 <= byte && byte <= 0xF4) {
    return 4;
  }

  // Non-utf-8 encoding
  return 0;
}

/// @brief Convert a long numerical representation of a utf-8 character to a character string buffer
/// @param value to convert
/// @param buf buffer to populate and return; must be at least sized 5
/// @return buf
static const char *sax_long_to_code_pt(long value, char *restrict buf) {

  if (value < 0 || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) {
    buf[0] = '\0';
  } else if (value <= 0x7F) {
    buf[0] = (char)value;
    buf[1] = '\0';
  } else if (value <= 0x7FF) {
    buf[0] = (char)(0xC0 | ((value >> 6) & 0x1F));
    buf[1] = (char)(0x80 | (value & 0x3F));
    buf[2] = '\0';
  } else if (value <= 0xFFFF) {
    buf[0] = (char)(0xE0 | ((value >> 12) & 0x0F));
    buf[1] = (char)(0x80 | ((value >> 6) & 0x3F));
    buf[2] = (char)(0x80 | (value & 0x3F));
    buf[3] = '\0';
  } else {
    buf[0] = (char)(0xF0 | ((value >> 18) & 0x07));
    buf[1] = (char)(0x80 | ((value >> 12) & 0x3F));
    buf[2] = (char)(0x80 | ((value >> 6) & 0x3F));
    buf[3] = (char)(0x80 | (value & 0x3F));
    buf[4] = '\0';
  }

  return buf;
}

/// @brief Test whether a string starts with another string
/// @param subject the subject to test
/// @param prefix the expected starting string
/// @return true if src startswith prefix
static bool sax_str_startswith(const char *restrict subject, const char *restrict prefix) {

  if (!subject) {
    return false;
  }

  if (!prefix) {
    return false;
  }

  const size_t prefix_size = strlen(prefix);
  const size_t src_size = strlen(subject);

  if (prefix_size > src_size) {
    return false;
  }

  return strncmp(subject, prefix, prefix_size) == 0;
}

/// @brief Test whether a string ends with another string
/// @param subject the string to test
/// @param suffix the suffix to match
/// @return true if subject endswith the suffix
static bool sax_str_endswith(const char *restrict subject, const char *restrict suffix) {

  if (subject == NULL) {
    return suffix == NULL ? true : false;
  }

  if (suffix == NULL) {
    return true;
  }

  const size_t subj_size = strlen(subject);
  const size_t suffix_size = strlen(suffix);

  if (suffix_size == 0) {
    return true;
  }

  if (suffix_size > subj_size) {
    return false;
  }

  const size_t offset = subj_size - suffix_size;
  return strcmp(subject + offset, suffix) == 0;
}

/// @brief Test whether the provided string is all spaces
/// @param str string to test
/// @return true if all spaces
static bool sax_str_is_space(const char *restrict str) {

  for (uint_fast64_t i = 0;
       str[i] != 0;
       i += sax_code_pt_size(str[i])) {

    if (!isspace(str[i])) {
      return false;
    }
  }

  return true;
}

/// @brief Unescape the provided string and populate the buffer with the
///   resulting utf-8 character
/// @param src string to convert
/// @param buf at least sized 5; must be populated
/// @return string literal or populated buf depending on the encoding
#ifndef SAXAMAPHONE_TEST
static
#endif
    const char *
    sax_str_unescape(const char *restrict src, char *restrict buf) {

  if (strcmp("&lt;", src) == 0) {
    return "<";
  }

  if (strcmp("&gt;", src) == 0) {
    return ">";
  }

  if (strcmp("&amp;", src) == 0) {
    return "&";
  }

  if (strcmp("&apos;", src) == 0) {
    return "'";
  }

  if (strcmp("&quot;", src) == 0) {
    return "\"";
  }

  // Longest is &#1114111; (10 chars)
  char num[16];

  if (sax_str_startswith(src, "&#x") && sax_str_endswith(src, ";")) {
    // -3 for "&#x" + ";"
    snprintf(num, sizeof(num), "%.*s", (int)(strlen(src) - 4), src + 3);
    long code_pt = strtol(num, NULL, 16);
    if (code_pt == 0) {
      return src;
    }

    return sax_long_to_code_pt(code_pt, buf);
  }

  if (sax_str_startswith(src, "&#") && sax_str_endswith(src, ";")) {

    // -3 for "&#" + ";"
    snprintf(num, sizeof(num), "%.*s", (int)(strlen(src) - 3), src + 2);
    long code_pt = strtol(num, NULL, 10);
    if (code_pt == 0) {
      return src;
    }

    return sax_long_to_code_pt(code_pt, buf);
  }

  return src;
}

// --- private sax_allocator_t methods --- //

static sax_arena_t sax_arena(const sax_config_t *restrict config) {

  if (config->buf == NULL) {
    sax_arena_t arena = {
        .alloc = config->alloc == NULL ? sax_default_alloc : config->alloc,
        .alloc_ctx = config->alloc_ctx,
    };
    arena.bytes = arena.alloc(arena.alloc_ctx, NULL, 1024);
    if (arena.bytes == NULL) {
      SAXAMAPHONE_LOG("Failed to allocated arena");
    } else {
      arena.bytes_size = 1024;
    }
    return arena;
  }

  // Protect against 0 buf_size to not overflow
  const uintptr_t align = config->buf_size > sizeof(uint8_t *)
                              ? ((uintptr_t)config->buf) % sizeof(uint8_t *)
                              : 0;

  return (sax_arena_t){
      .bytes = config->buf + align,
      .bytes_size = config->buf_size - align,
  };
}

/// @brief Perform an object allocation
/// @param alloc instance
/// @param size in bytes of the allocation
/// @return the pointer or NULL if insufficient memory
static void *sax_arena_alloc(sax_arena_t *restrict arena, size_t size) {

  if (arena == NULL || arena->bytes == NULL || size == 0) {
    return NULL;
  }

  const uintptr_t align = (arena->offset + size) % sizeof(uint8_t *);
  if (arena->offset + size + align > arena->bytes_size) {

    if (arena->alloc) {
      arena->bytes = arena->alloc(arena->alloc_ctx, arena->bytes, arena->bytes_size * 2);
      return sax_arena_alloc(arena, size);
    }

    // Out of memory
    return NULL;
  }

  void *restrict res = arena->bytes + arena->offset;
  arena->offset += size + align;
  return res;
}

/// @brief Return a new allocator instance which manages the remaining memory
/// @param alloc instance
/// @return arena allocator managing remaining bytes
static sax_arena_t sax_arena_partition(const sax_arena_t *restrict arena) {
  return (sax_arena_t){
      .bytes = arena->bytes + arena->offset,
      .bytes_size = arena->bytes_size - arena->offset,
  };
}

/// @brief Test whether the provided string is empty
/// @param str to test
/// @return true if the size is zero
static bool sax_str_empty(const char *restrict str) {
  return str == NULL || strlen(str) == 0;
}

/// @brief Remove spaces (' ', '\t', '\n', etc) to the left of the first non-space character
/// @param src string to ltrim
/// @return left-trimmed string (pointer arithmatically produced)
static char *sax_str_ltrim(char *src) {

  if (src == NULL) {
    return NULL;
  }

  const size_t src_size = strlen(src);

  for (uint_fast64_t i = 0;
       i < src_size;
       i += sax_code_pt_size(src[i])) {

    if (!isspace(src[i])) {
      src += i;
      break;
    }
  }

  return src;
}

/// @brief Remove spaces (' ', '\t', '\n', etc) to the right of the first non-space character
/// @param src string to rtrim
/// @return src mutated to be NULL-termed
static char *sax_str_rtrim(char *src) {

  if (sax_str_empty(src)) {
    return src;
  }

  uint_fast64_t last_non_space = 0;
  const size_t src_size = strlen(src);
  uint_fast64_t i = 0;

  // Can't tell utf-8 from top to bottom; have to start from bottom
  for (;
       i < src_size;
       i += sax_code_pt_size(src[i])) {

    if (!isspace(src[i])) {
      last_non_space = i;
    }
  }

  const uint_fast64_t end = last_non_space + sax_code_pt_size(src[last_non_space]);
  src[end] = '\0';
  return src;
}

/// @brief Perform both an ltrim & rtrim
/// @param str to trim
/// @return trimmed string
#ifndef SAXAMAPHONE_TEST
static
#endif
    char *
    sax_str_trim(char *str) {
  return sax_str_ltrim(sax_str_rtrim(str));
}

// --- private sax_iter_t methods --- //

/// @brief Release any lingering resources
/// @param iter instance
static void sax_iter_close(sax_iter_t *restrict iter) {

  switch (iter->type) {

  case SAX_ITER_FILE:
    if (iter->impl.file.fp) {
      fclose(iter->impl.file.fp);
      iter->impl.file.fp = NULL;
    }
    break;

  case SAX_ITER_STR:
    iter->impl.str.value = NULL;
    break;
  }

  memset(iter, 0, sizeof(sax_iter_t));
}

/// @brief Retrieve the next byte value or 0 if iteration is complete (or malformed utf-8)
/// @param iter iterator
/// @return next byte value or 0 if completed
static uint8_t sax_iter_next_byte(sax_iter_t *restrict iter) {

  switch (iter->type) {

  case SAX_ITER_FILE: {

    sax_file_iter_t *restrict fiter = &iter->impl.file;

    if (!fiter->fp) {
      return 0;
    }

    if (fiter->offset == fiter->bytes_read) {

      if (0 < fiter->bytes_read &&
          fiter->bytes_read < fiter->file_buffer_size) {
        // Hit eof
        return 0;
      }

      fiter->offset = 0;
      fiter->bytes_read = fread(
          fiter->file_buffer,
          sizeof(uint8_t),
          fiter->file_buffer_size,
          fiter->fp);

      if (fiter->bytes_read == 0) {
        return 0;
      }
    }

    return (uint8_t)fiter->file_buffer[fiter->offset++];
  }

  case SAX_ITER_STR: {
    sax_str_iter_t *restrict siter = &iter->impl.str;
    if (!siter->value) {
      return 0;
    }

    const uint8_t value = (uint8_t)siter->value[siter->offset++];
    if (siter->offset == siter->len || value == 0) {
      siter->value = NULL;
    }
    return value;
  }

  default:
    //   Unreachable
    return 0;
  }
}

/// @brief Retrieve the next glyph to process (0-4 sized string); when UTF-8 is
///   malformed or reached the end of file; an empty string is returned
/// @param iter instance
/// @return non-NULL glyph string sized 0-4 bytes
static const char *sax_iter_next_glyph(sax_iter_t *restrict iter) {

  uint8_t byte = sax_iter_next_byte(iter);
  if (byte == 0) {
    sax_iter_close(iter);
    return "";
  }

  iter->glyph[0] = (char)byte;
  const uint_fast8_t pt_size = sax_code_pt_size(byte);

  uint_fast8_t i = 1;
  for (; i < pt_size && byte != 0; ++i) {
    byte = sax_iter_next_byte(iter);
    iter->glyph[i] = (char)byte;
  }
  iter->glyph[i] = '\0';
  return iter->glyph;
}

// --- private parser methods --- //

/// @brief Reset parser to parse a new XML tag
/// @param parser instance
static void sax_parser_reset(sax_parser_t *restrict parser) {
  parser->data = NULL;
  parser->attrs = NULL;
  parser->current_attr = NULL;
  parser->escaped = NULL;
  parser->arena.offset = 0;
}

/// @brief Set the parser's new state
/// @param parser instance
/// @param state new
static void sax_parser_state(sax_parser_t *restrict parser, sax_state_t state) {
  parser->prev_state = parser->state;
  parser->state = state;
}

/// @brief Append a glyph to the current token (tag, content, attr name, attr value, or escaped)
/// @param parser instance
/// @param token [OUT] to append to
/// @param glyph glyph to append
/// @return 0 on success or SAX_EVENT_ERROR otherwise
static sax_event_t sax_parser_append(
    sax_parser_t *restrict parser,
    char **token,
    const char *restrict glyph) {

  const uint8_t *end = parser->arena.bytes + parser->arena.bytes_size;
  const size_t glyph_size = strlen(glyph) + 1;
  uintptr_t alignment = 0;

  if (!(*token)) {
    // Initialize
    *token = (char *)(parser->arena.bytes + parser->arena.offset);
    alignment = (uintptr_t)(*token) % sizeof(char *);
    (*token) += alignment; // align
    (*token)[0] = '\0';
  }

  const size_t token_len = strlen(*token);

  if ((uint8_t *)(*token + token_len + glyph_size) > end) {
    parser->data = "Out of memory";
    sax_parser_state(parser, SAX_STATE_ERROR);
    return SAX_EVENT_ERROR;
  }

  strcpy(*token + token_len, glyph);
  parser->arena.offset += glyph_size + alignment;
  return 0;
}

/// @brief Set the parser to the error state
/// @param parser instance
/// @param format message format
/// @param  ... message args
static void sax_parser_error(sax_parser_t *restrict parser, const char *restrict format, ...) {

  va_list args;
  va_start(args, format);

  sax_parser_state(parser, SAX_STATE_ERROR);
  parser->data = (char *)parser->arena.bytes;
  vsnprintf(parser->data, parser->arena.bytes_size, format, args);

  va_end(args);
}

/// @brief Throw the parser into an error state
/// @param parser instance
/// @param glyph the unexpected glyph
/// @return SAX_EVENT_ERROR
static sax_event_t sax_parser_error_unexpected_glyph(
    sax_parser_t *restrict parser,
    const char *restrict glyph) {

  sax_parser_error(parser,
                   "Unexpected character \"%s\" located on line %" PRIuFAST16 " column %" PRIuFAST16,
                   glyph,
                   parser->line,
                   parser->column);

  return SAX_EVENT_ERROR;
}

/// @brief Handle the glyph when in the SAX_STATE_INITIAL state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
sax_event_t sax_parser_state_init(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {
  case '<':
    sax_parser_state(parser, SAX_STATE_IN_TAG);
    break;

  default:
    return sax_parser_error_unexpected_glyph(parser, glyph);
  }

  return 0;
}

/// @brief Handle the glyph when in the SAX_STATE_IN_TAG state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static sax_event_t sax_parser_state_in_tag(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {

  case '?':
    sax_parser_state(parser, SAX_STATE_IN_PROC_INST);
    break;

  case '/':
    sax_parser_state(parser, SAX_STATE_IN_END_TAG);
    break;

  case '!':
    // TODO: CDATA
    sax_parser_state(parser, SAX_STATE_IN_COMMENT);
    break;

  default:
    if (strchr(SAXAMAPHONE_EXCLUDE_TAG_PREFIX, glyph[0])) {
      return sax_parser_error_unexpected_glyph(parser, glyph);
    }

    parser->data = sax_arena_alloc(&parser->arena, strlen(glyph) + 1);
    strcpy(parser->data, glyph);
    sax_parser_state(parser, SAX_STATE_IN_START_TAG);
    break;
  }

  return 0;
}

static sax_event_t sax_parser_state_in_escaped_char(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {
  case ';':

    if (SAX_EVENT_ERROR == sax_parser_append(parser, &parser->escaped, glyph)) {
      return SAX_EVENT_ERROR;
    }

    sax_parser_state(parser, parser->prev_state);
    char unesc[5];
    sax_event_t ev = sax_parser_append(
        parser,
        &parser->data,
        sax_str_unescape(parser->escaped, unesc));
    parser->escaped = NULL;
    return ev;

  default:
    return sax_parser_append(parser, &parser->escaped, glyph);
  }
}

static sax_event_t sax_parser_state_in_start_tag(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {

  case '/':
    SAXAMAPHONE_LOG("Parsed tag \"%s\"\n", parser->data);
    sax_parser_state(parser, SAX_STATE_CLOSING_START_TAG);
    return 0;

  case '>':
    SAXAMAPHONE_LOG("Parsed tag \"%s\"\n", parser->data);
    sax_parser_state(parser, SAX_STATE_IN_CONTENT);
    return SAX_EVENT_START_ELEMENT;

  case SAXAMAPHONE_SPACE:
    if (parser->data == NULL || strlen(parser->data) == 0) {
      return sax_parser_error_unexpected_glyph(parser, glyph);
    }
    SAXAMAPHONE_LOG("Parsed tag \"%s\"\n", parser->data);
    sax_parser_state(parser, SAX_STATE_START_TAG_SPACE);
    return 0;

  default:

    if (strchr(SAXAMAPHONE_EXCLUDE_TAG, glyph[0])) {
      return sax_parser_error_unexpected_glyph(parser, glyph);
    }

    return sax_parser_append(parser, &parser->data, glyph);
  }
}

static sax_event_t sax_parser_state_start_tag_space(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {

  case '>':
    sax_parser_state(parser, SAX_STATE_IN_CONTENT);
    return SAX_EVENT_START_ELEMENT;

  case '/':
    sax_parser_state(parser, SAX_STATE_CLOSING_START_TAG);
    return 0;

  case SAXAMAPHONE_SPACE:
    // noop
    return 0;

  default:

    if (strchr(SAXAMAPHONE_EXCLUDE_TAG_PREFIX, glyph[0])) {
      // Invalid start of attr name
      return sax_parser_error_unexpected_glyph(parser, glyph);
    }

    parser->current_attr = sax_arena_alloc(&parser->arena, sizeof(sax_attr_t));
    memset(parser->current_attr, 0, sizeof(sax_attr_t));

    // Add to end of linked list
    if (parser->attrs) {
      sax_attr_t *attr = parser->attrs;
      for (; attr->next != NULL; attr = attr->next) {
      }
      attr->next = parser->current_attr;
    } else {
      parser->attrs = parser->current_attr;
    }

    sax_parser_state(parser, SAX_STATE_IN_ATTR_NAME);
    return sax_parser_append(parser, &parser->current_attr->name, glyph);
  }
}

static sax_event_t sax_parser_state_assigning_attr_value(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {
  case '"':
    sax_parser_state(parser, SAX_STATE_IN_ATTR_VALUE);
    return 0;

  default:
    return sax_parser_error_unexpected_glyph(parser, glyph);
  }
}

static sax_event_t sax_parser_state_in_content(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {

  case '&':
    if (SAX_EVENT_ERROR == sax_parser_append(parser, &parser->escaped, glyph)) {
      return SAX_EVENT_ERROR;
    }
    sax_parser_state(parser, SAX_STATE_IN_ESC_CHAR);
    return 0;

  case '<':
    sax_parser_state(parser, SAX_STATE_IN_TAG);
    if (parser->data) {

      if (!parser->untrimmed_content) {
        parser->data = sax_str_trim(parser->data);
      }

      if (sax_str_is_space(parser->data)) {
        sax_parser_reset(parser);
        return 0;
      }

      return SAX_EVENT_CONTENT;
    }
    return 0;

  default:
    return sax_parser_append(parser, &parser->data, glyph);
  }
}

static sax_event_t sax_parser_state_in_end_tag(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {
  case '>':
    if (parser->data == NULL || strlen(parser->data) == 0) {
      sax_parser_error(parser, "Empty closing tag found at line %d column %d", parser->line, parser->column);
      return SAX_EVENT_ERROR;
    }
    sax_parser_state(parser, SAX_STATE_IN_CONTENT);
    return SAX_EVENT_END_ELEMENT;

  case SAXAMAPHONE_SPACE:
    sax_parser_error_unexpected_glyph(parser, glyph);
    return 0;

  default:
    if (strchr(SAXAMAPHONE_EXCLUDE_TAG, glyph[0])) {
      sax_parser_error_unexpected_glyph(parser, glyph);
      return SAX_EVENT_ERROR;
    }

    return sax_parser_append(parser, &parser->data, glyph);
  }
}

static sax_event_t sax_parser_state_in_attr_name(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {

  case '>':
    SAXAMAPHONE_LOG("Parsed attr name \"%s\"\n", parser->current_attr->name);
    parser->current_attr = NULL;
    sax_parser_state(parser, SAX_STATE_IN_CONTENT);
    return SAX_EVENT_START_ELEMENT;

  case SAXAMAPHONE_SPACE:
    SAXAMAPHONE_LOG("Parsed attr name \"%s\"\n", parser->current_attr->name);
    parser->current_attr = NULL;
    sax_parser_state(parser, SAX_STATE_START_TAG_SPACE);
    return 0;

  case '=':
    SAXAMAPHONE_LOG("Parsed attr name \"%s\"\n", parser->current_attr->name);
    sax_parser_state(parser, SAX_STATE_ASSIGNING_ATTR_VALUE);
    return 0;

  default:
    if (strchr(SAXAMAPHONE_EXCLUDE_TAG, glyph[0])) {
      return sax_parser_error_unexpected_glyph(parser, glyph);
    }
    return sax_parser_append(parser, &parser->current_attr->name, glyph);
  }
}

static sax_event_t sax_parser_state_in_attr_value(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {

  case '"':
    SAXAMAPHONE_LOG("Parsed attr value \"%s\"\n", parser->current_attr->value);
    parser->current_attr = NULL;
    sax_parser_state(parser, SAX_STATE_START_TAG_SPACE);
    return 0;

  case '&':
    sax_parser_state(parser, SAX_STATE_IN_ESC_CHAR);
    return sax_parser_append(parser, &parser->escaped, glyph);

  default:
    return sax_parser_append(parser, &parser->current_attr->value, glyph);
  }
}

static sax_event_t sax_parser_state_in_comment(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {

  case '>': {
    if (sax_str_endswith(parser->data, "-->")) {
      parser->data = NULL;
      sax_parser_reset(parser);
      sax_parser_state(parser, SAX_STATE_IN_CONTENT);
    }
  }
    return 0;

  default:
    return sax_parser_append(parser, &parser->data, glyph);
  }
}

static sax_event_t sax_parser_state_in_proc_inst(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {
  case '>':
    sax_parser_reset(parser);
    sax_parser_state(parser, SAX_STATE_IN_CONTENT);
    break;
  }

  return 0;
}

// === public methods === //

sax_parser_t *sax_parser(const sax_config_t *restrict config) {

  if (!config) {
    SAXAMAPHONE_LOG("[SAXAMAPHONE] ERROR: No configuration provided\n");
    return NULL;
  }

  sax_arena_t arena = sax_arena(config);
  if (arena.bytes == 0 || arena.bytes_size == 0) {
    return NULL;
  }

  sax_parser_t *restrict parser = sax_arena_alloc(&arena, sizeof(sax_parser_t));
  if (!parser) {
    return NULL;
  }

  *parser = (sax_parser_t){
      .arena = sax_arena_partition(&arena),
      .untrimmed_content = config->untrimmed_content,
      .line = 1,
      .column = 1,
  };

  if (config->path) {

    FILE *fp = fopen(config->path, "r");

    if (!fp) {
      sax_parser_error(parser, "Failed to open \"%s\"", config->path);
      return parser;
    }

    uint8_t *file_buf = NULL;
    size_t file_buf_size = 0;

    if (parser->arena.alloc) {
      file_buf_size = 4096;
      file_buf = parser->arena.alloc(parser->arena.alloc_ctx, NULL, 4096);
      if (file_buf == NULL) {
        // TODO: free
        SAXAMAPHONE_LOG("Failed to allocate file buffer");
        return NULL;
      }
    } else {
      file_buf_size = sax_file_buf_size(parser->arena.bytes_size);
      file_buf = sax_arena_alloc(&parser->arena, file_buf_size);
      parser->arena = sax_arena_partition(&parser->arena);
    }

    parser->iter = (sax_iter_t){
        .type = SAX_ITER_FILE,
        .impl = {
            .file = {
                .fp = fp,
                .file_buffer = file_buf,
                .file_buffer_size = file_buf_size,
            },
        },
    };
  } else if (config->string) {

    parser->iter = (sax_iter_t){
        .type = SAX_ITER_STR,
        .impl.str = {
            .value = config->string,
        },
    };
  }

  return parser;
}

sax_event_t sax_next(sax_parser_t *restrict parser) {

  if (parser == NULL || parser->state == SAX_STATE_ERROR) {
    return SAX_EVENT_ERROR;
  }

  sax_event_t ev = 0;

  // Reset state
  sax_parser_reset(parser);

  for (const char *restrict glyph = sax_iter_next_glyph(&parser->iter);
       glyph[0] != '\0';
       glyph = sax_iter_next_glyph(&parser->iter)) {

    switch (parser->state) {

    case SAX_STATE_ERROR:
      ev = SAX_EVENT_ERROR;
      break;

    case SAX_STATE_INIT:
      ev = sax_parser_state_init(parser, glyph);
      break;

    case SAX_STATE_IN_TAG:
      ev = sax_parser_state_in_tag(parser, glyph);
      break;

    case SAX_STATE_IN_START_TAG:
      ev = sax_parser_state_in_start_tag(parser, glyph);
      break;

    case SAX_STATE_START_TAG_SPACE:
      ev = sax_parser_state_start_tag_space(parser, glyph);
      break;

    case SAX_STATE_ASSIGNING_ATTR_VALUE:
      ev = sax_parser_state_assigning_attr_value(parser, glyph);
      break;

    case SAX_STATE_IN_ESC_CHAR:
      ev = sax_parser_state_in_escaped_char(parser, glyph);
      break;

    case SAX_STATE_IN_CONTENT:
      ev = sax_parser_state_in_content(parser, glyph);
      break;

    case SAX_STATE_IN_END_TAG:
      ev = sax_parser_state_in_end_tag(parser, glyph);
      break;

    case SAX_STATE_IN_ATTR_NAME:
      ev = sax_parser_state_in_attr_name(parser, glyph);
      break;

    case SAX_STATE_IN_ATTR_VALUE:
      ev = sax_parser_state_in_attr_value(parser, glyph);
      break;

    case SAX_STATE_IN_COMMENT:
      ev = sax_parser_state_in_comment(parser, glyph);
      break;

    case SAX_STATE_IN_PROC_INST:
      ev = sax_parser_state_in_proc_inst(parser, glyph);
      break;

    default:
      // Unreachable
      parser->data = "Unreachable section reached";
      ev = SAX_EVENT_ERROR;
      break;
    }

    if (glyph[0] == '\n') {
      ++parser->line;
      parser->column = 1;
    } else {
      ++parser->column;
    }

    if (ev != 0) {
      return ev;
    }
  }

  return SAX_EVENT_END_DOCUMENT;
}

const char *sax_error(const sax_parser_t *restrict parser) {
  return parser && parser->data ? parser->data : NULL;
}

const char *sax_tag(const sax_parser_t *restrict parser) {
  return parser && parser->data ? parser->data : NULL;
}

const char *sax_content(const sax_parser_t *restrict parser) {
  return parser && parser->data ? parser->data : NULL;
}

const sax_attr_t *sax_attrs(const sax_parser_t *restrict parser) {
  return parser && parser->attrs ? parser->attrs : NULL;
}

const char *sax_attr(const sax_parser_t *restrict parser, const char *restrict name) {

  if (parser == NULL || name == NULL) {
    return NULL;
  }

  for (const sax_attr_t *restrict attr = parser->attrs;
       attr != NULL;
       attr = attr->next) {
    if (strcmp(name, attr->name) == 0) {
      return attr->value;
    }
  }

  return NULL;
}

#endif // SAXAMAPHONE_IMPLEMENTATION

#endif // SAXAMAPHONE_H