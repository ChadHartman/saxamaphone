#include <inttypes.h> // PRIuFAST16
#include <stdarg.h>   // va_start
#include <stdio.h>    // fopen
#include <stdlib.h>   // malloc, realloc, free
#include <string.h>   // memset

#include <saxamaphone.h>

/// @brief Primary state of the `sax_parser_t` hierarchical state machine
typedef enum sax_primary_state_t {

  /// @brief Initial position
  SAX_STATE_INIT = 0,

  /// @brief A '<' has been found
  SAX_STATE_TAG = 1 << 1,

  /// @brief `<` followed by `!--`
  SAX_STATE_COMMENT = 1 << 2,

  /// @brief '<' followed by '?'
  SAX_STATE_PROC_INST = 1 << 3,

  /// @brief '<' + non-control character
  SAX_STATE_TAG_START = 1 << 4,

  /// @brief Characters found between '>' and '<'
  SAX_STATE_CONTENT = 1 << 5,

  /// @brief `<[CDATA[`
  SAX_STATE_CDATA = 1 << 6,

  /// @brief '<' + '/'
  SAX_STATE_TAG_END = 1 << 7,

  /// @brief Iterator returns 0 for character
  SAX_STATE_COMPLETE = 1 << 8,

  /// @brief Fallback state for unexpected input
  SAX_STATE_ERROR = 1 << 9,
} sax_primary_state_t;

/// @brief Secondary state of the `sax_parser_t` hierarchical state machine
typedef enum sax_secondary_state_t {

  SAX_STATE_NONE = 0,

  /// @brief '/' found within start tag or '?' in a processing instruction
  SAX_STATE_TAG_CLOSE = 1 << 10,

  /// @brief Space after `<foo `, `<foo attr `, `<foo attr="value"`
  ///   `<?foo `, `<?foo attr `, or `<?foo attr="value"`
  SAX_STATE_SPACE = 1 << 11,

  /// @brief `<?foo ` `<foo ` + non-control character
  SAX_STATE_ATTR_NAME = 1 << 12,

  /// @brief '=' found after parsing attr name
  SAX_STATE_ATTR_ASSIGN = 1 << 13,

  /// @brief `<foo name="` or `<?foo name="`
  SAX_STATE_ATTR_VALUE = 1 << 14,

  /// @brief '&' found in attr_value and content
  SAX_STATE_ESC_CHAR = 1 << 15,

} sax_secondary_state_t;

/// @brief XML Iterator type
typedef enum {
  SAX_ITER_FILE,
  SAX_ITER_STR,
} sax_iter_type_t;

/// @brief XML File Iterator
typedef struct sax_file_iter_t {
  FILE *fp;
  uint_fast32_t offset;
  uint_fast32_t bytes_read;
  uint8_t *file_buffer;
  size_t file_buffer_size;
} sax_file_iter_t;

/// @brief XML String Iterator
typedef struct sax_str_iter_t {
  const char *value;
  uint_fast32_t offset;
  uint_fast32_t len;
} sax_str_iter_t;

/// @brief Unified XML Iterator
typedef struct sax_iter_t {

  sax_iter_type_t type;

  union {
    sax_file_iter_t file;
    sax_str_iter_t str;
  } impl;

} sax_iter_t;

/// @brief Arena allocator
typedef struct sax_arena_t {
  uint8_t *bytes;
  uint32_t bytes_size;
  uint32_t offset;
  void *(*alloc)(void *, void *, size_t);
  void *alloc_ctx;
} sax_arena_t;

/// @brief SAX Parser definition
struct sax_parser_t {

  // Configuration
  bool untrimmed_content;

  // Resources
  sax_arena_t arena;
  sax_iter_t iter;

  // Metadatas
  sax_primary_state_t primary_state;
  sax_secondary_state_t secondary_state;
  uint_fast32_t line;
  uint_fast32_t column;

  // Workspace fields
  /// @brief Used for error (SAX_EVENT_ERROR), tag (SAX_EVENT_START_TAG), or content (SAX_EVENT_CONTENT)
  char *data;
  /// @brief Used to build intermediate data (such as comments, cdata, and
  ///   ampersand-escapes)
  char *stage;

  /// @brief Pointer to the root sax_attr_t for the current tag
  sax_attr_t *attrs;

  /// @brief Pointer to the current sax_attr_t being constructed
  sax_attr_t *current_attr;
};

/// @brief Used to expose APIs for testing
#ifdef SAXAMAPHONE_TEST
#define SAX_TEST_API
#else
#define SAX_TEST_API static
#endif

/// @brief Illegal XML tag characters
#define SAXAMAPHONE_EXCLUDE_TAG "!\"#$%&'()*+,/;<=>?@[\\]^`{|}~"

/// @brief Illegal XML Starting tag characters
#define SAXAMAPHONE_EXCLUDE_TAG_PREFIX (SAXAMAPHONE_EXCLUDE_TAG ".-0123456789")

/// @brief Various space chars used in switch cases
#define SAXAMAPHONE_SPACE \
  ' ' : case '\f':        \
  case '\n':              \
  case '\r':              \
  case '\t':              \
  case '\v'

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

/// @brief Only used for testing
#ifdef SAXAMAPHONE_TEST
sax_arena_t *sax_parser_arena(sax_parser_t *restrict parser) {
  return &parser->arena;
}
#endif

/// @brief Perform a string copy operation in a platform agnostic manner
/// @param dest destination buffer to fill
/// @param dest_size capacity of the provided buffer
/// @param src string to copy
/// @return the copied string
SAX_TEST_API char *sax_strcpy(char *restrict dest, size_t dest_size, const char *restrict src) {
#ifdef _MSC_VER
  strcpy_s(dest, dest_size, src);
  return dest;
#else
  (void)dest_size;
  return strcpy(dest, src);
#endif
}

/// @brief Default allocator
/// @param ctx provided to config
/// @param ptr pointer to free or reallocate
/// @param size size of the requested allocation or when 0; free
/// @return the pointer to memory when size > 0 else NULL
SAX_TEST_API void *sax_default_alloc(void *ctx, void *ptr, size_t size) {

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
SAX_TEST_API uint_fast32_t sax_file_buf_size(uint_fast32_t capacity) {

  const uint_fast32_t half = capacity / 2;
  // Min 32 arbitrarily chosen
  uint_fast32_t buf_size = 32;
  while (buf_size * 2 <= half) {
    buf_size = buf_size * 2;
  }
  return buf_size;
}

/// @brief Given the provided byte determine the UTF-8 code point size
/// @param byte byte value
/// @return size 1-4 if value; 0 if invalid
SAX_TEST_API uint_fast8_t sax_code_pt_size(uint8_t byte) {

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
SAX_TEST_API const char *sax_long_to_code_pt(long value, char *restrict buf) {

  if (value < 0 || value > 0x10FFFF || (0xD800 <= value && value <= 0xDFFF)) {
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

/// @brief NULL-safe strlen utility
/// @param str NULLable string to test
/// @return -1 when NULL, otherwise strlen(str)
static int_fast32_t sax_strlen(const char *restrict str) {
  return str == NULL ? -1 : (int_fast32_t)strlen(str);
}

/// @brief NULL-safe string comparison utility
/// @param lhs left-hand-side string to test
/// @param rhs right-hand-side string to test
/// @return true if lhs and rhs are NULL or strcmp(lhs, rhs) == 0
SAX_TEST_API bool sax_str_eq(const char *restrict lhs, const char *restrict rhs) {

  if (lhs == NULL || rhs == NULL) {
    return lhs == rhs;
  }

  return strcmp(lhs, rhs) == 0;
}

/// @brief Test whether a string starts with another string
/// @param subject the subject to test
/// @param prefix the expected starting string
/// @return true if src startswith prefix
SAX_TEST_API bool sax_startswith(const char *restrict subject, const char *restrict prefix) {

  if (subject == NULL) {
    return false;
  }

  if (prefix == NULL) {
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
SAX_TEST_API bool sax_endswith(const char *restrict subject, const char *restrict suffix) {

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

/// @brief Test whether the provided char isspace in a platform independant way
///   (invalid chars in windows i.e. utf-8 causes aborts)
/// @param c char to test
/// @return true if is a space
static bool sax_isspace(const char c) {

  switch (c) {
  case SAXAMAPHONE_SPACE:
    return true;

  default:
    return false;
  }
}

/// @brief Test whether the provided string is all spaces
/// @param str string to test
/// @return true if all spaces
static bool sax_str_isspace(const char *restrict str) {

  for (uint_fast64_t i = 0;
       str[i] != 0;
       i += sax_code_pt_size(str[i])) {

    if (!sax_isspace(str[i])) {
      return false;
    }
  }

  return true;
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
SAX_TEST_API char *sax_ltrim(char *src) {

  if (src == NULL) {
    return NULL;
  }

  const size_t src_size = strlen(src);

  for (uint_fast64_t i = 0;
       i < src_size;
       i += sax_code_pt_size(src[i])) {

    if (!sax_isspace(src[i])) {
      src += i;
      break;
    }
  }

  return src;
}

/// @brief Remove spaces (' ', '\t', '\n', etc) to the right of the first non-space character
/// @param src string to rtrim
/// @return src mutated to be NULL-termed
SAX_TEST_API char *sax_rtrim(char *src) {

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

    if (!sax_isspace(src[i])) {
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
SAX_TEST_API char *sax_trim(char *str) {
  return sax_ltrim(sax_rtrim(str));
}

/// @brief Unescape the provided string and populate the buffer with the
///   resulting utf-8 character
/// @param src string to convert
/// @param buf at least sized 5; may be populated
/// @return string literal or populated buf depending on the encoding
SAX_TEST_API const char *sax_unescape(const char *restrict src, char *restrict buf) {

  if (src == NULL) {
    return NULL;
  }

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

  if (buf == NULL) {
    return NULL;
  }

  // Longest is &#1114111; (10 chars)
  char num[16];

  if (sax_startswith(src, "&#x") && sax_endswith(src, ";")) {
    // -3 for "&#x" + ";"
    snprintf(num, sizeof(num), "%.*s", (int)(strlen(src) - 4), src + 3);
    const long code_pt = strtol(num, NULL, 16);
    if (code_pt == 0) {
      return src;
    }

    return sax_long_to_code_pt(code_pt, buf);
  }

  if (sax_startswith(src, "&#") && sax_endswith(src, ";")) {

    // -3 for "&#" + ";"
    snprintf(num, sizeof(num), "%.*s", (int)(strlen(src) - 3), src + 2);
    const long code_pt = strtol(num, NULL, 10);
    if (code_pt == 0) {
      return src;
    }

    return sax_long_to_code_pt(code_pt, buf);
  }

  return src;
}

/// @brief Expand the arena if bytes have ran out
/// @param arena instance
/// @return true if successfully expanded
static bool sax_arena_expand(sax_arena_t *restrict arena) {

  if (!arena->alloc) {
    return false;
  }

  const uint_fast32_t size = arena->bytes_size * 2;
  arena->bytes = arena->alloc(arena->alloc_ctx, arena->bytes, size);
  if (arena->bytes == NULL) {
    return false;
  }

  arena->bytes_size = size;
  SAXAMAPHONE_LOG("Called realloc for arena and received %p sized %d", arena->bytes, arena->bytes_size);
  return true;
}

/// @brief Perform an object allocation
/// @param alloc instance
/// @param size in bytes of the allocation
/// @return the pointer or NULL if insufficient memory
SAX_TEST_API void *sax_arena_alloc(sax_arena_t *restrict arena, uint_fast32_t size) {

  if (arena == NULL || arena->bytes == NULL || size == 0) {
    return NULL;
  }

  uint_fast32_t align = ((uintptr_t)arena->bytes + arena->offset + size) % sizeof(uint8_t *);
  if (align != 0) {
    align = sizeof(uint8_t *) - align;
  }

  if (arena->offset + size + align > arena->bytes_size) {
    if (sax_arena_expand(arena)) {
      return sax_arena_alloc(arena, size);
    }

    // Out of memory
    return NULL;
  }

  void *restrict res = arena->bytes + arena->offset + align;
  arena->offset += size + align;
  return res;
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
        fclose(fiter->fp);
        fiter->fp = NULL;
        return 0;
      }

      fiter->offset = 0;
      fiter->bytes_read = (uint_fast32_t)fread(
          fiter->file_buffer,
          sizeof(uint8_t),
          fiter->file_buffer_size,
          fiter->fp);

      if (fiter->bytes_read == 0) {
        fclose(fiter->fp);
        fiter->fp = NULL;
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
/// @param glyph the file buffer to populate; must be at least 5 chars (4 for max utf-8 + NULL term)
/// @return non-NULL glyph string sized 0-4 bytes
static const char *sax_iter_next_glyph(sax_iter_t *restrict iter, char *restrict glyph) {

  uint8_t byte = sax_iter_next_byte(iter);
  if (byte == 0) {
    return "";
  }

  glyph[0] = (char)byte;
  const uint_fast8_t pt_size = sax_code_pt_size(byte);

  uint_fast8_t i = 1;
  for (; i < pt_size && byte != 0; ++i) {
    byte = sax_iter_next_byte(iter);
    glyph[i] = (char)byte;
  }
  glyph[i] = '\0';
  return glyph;
}

/// @brief Reset parser to parse a new XML tag
/// @param parser instance
static void sax_parser_reset(sax_parser_t *restrict parser) {

  // Special case; <foo/> generates 2 events: START_TAG & END_TAG; both sharing
  //   the same data (tag)
  if (parser->secondary_state != SAX_STATE_TAG_CLOSE) {
    parser->data = NULL;
    parser->arena.offset = 0;
  }

  parser->attrs = NULL;
  parser->current_attr = NULL;
  parser->stage = NULL;
}

/// @brief Set the parser to the error state
/// @param parser instance
/// @param format message format
/// @param  ... message args
static void sax_parser_error(sax_parser_t *restrict parser, const char *restrict format, ...) {

  parser->primary_state = SAX_STATE_ERROR;
  parser->secondary_state = SAX_STATE_NONE;
  sax_parser_reset(parser);

  if (strchr(format, '%') == NULL) {
    // No args; can use it directly
    parser->data = (char *)format;
    return;
  }

  va_list args;
  va_start(args, format);

  // Safety factor of 2 to ensure there's enough space for vsnprintf; despite
  //   the contract, vsnprintf still overflowed the buffer on clang
  parser->data = sax_arena_alloc(&parser->arena, (uint_fast32_t)(strlen(format) * 2));
  if (parser->data == NULL) {
    parser->data = "Out of memory";
  } else {
    vsnprintf(parser->data, parser->arena.bytes_size, format, args);
  }

  va_end(args);
}

/// @brief Append a glyph to the current token (tag, content, attr name, attr value, or escaped)
/// @param parser instance
/// @param token [OUT] to append to
/// @param glyph glyph to append
/// @return 0 on success or SAX_EVENT_ERROR otherwise
static uint_fast8_t sax_parser_append(
    sax_parser_t *restrict parser,
    char **token,
    const char *restrict glyph) {

  const uint8_t *end = parser->arena.bytes + parser->arena.bytes_size;
  const size_t glyph_size = strlen(glyph) + 1;
  const size_t token_len = (*token) == NULL ? 0 : strlen(*token);
  uint8_t *candidate = NULL;
  uintptr_t alignment = 0;

  if ((*token) == NULL) {
    candidate = parser->arena.bytes + parser->arena.offset;
    alignment = (uintptr_t)(candidate) % sizeof(char *);
    candidate += alignment;
  } else {
    candidate = (uint8_t *)(*token);
  }

  if ((candidate + token_len + glyph_size) >= end) {
    if (sax_arena_expand(&parser->arena)) {
      return sax_parser_append(parser, token, glyph);
    }

    sax_parser_error(parser, "Out of memory");
    return SAX_EVENT_ERROR;
  }

  if ((*token) == NULL) {
    // Initialize
    (*token) = (char *)candidate;
  }

  char *restrict dest = (*token) + token_len;
  const size_t dest_size = (size_t)((uintptr_t)(parser->arena.bytes + parser->arena.bytes_size) - (uintptr_t)dest);
  sax_strcpy(dest, dest_size, glyph);
  parser->arena.offset += (uint_fast32_t)(glyph_size + alignment);
  return 0;
}

/// @brief Throw the parser into an error state
/// @param parser instance
/// @param glyph the unexpected glyph
/// @return SAX_EVENT_ERROR
static sax_event_t sax_parser_error_unexpected_glyph(
    sax_parser_t *restrict parser,
    const char *restrict glyph) {

  sax_parser_error(parser,
                   "Unexpected character \'%s\' located on line %" PRIuFAST32 " column %" PRIuFAST32,
                   glyph,
                   parser->line,
                   parser->column);

  return SAX_EVENT_ERROR;
}

/// @brief Handle the glyph when in the SAX_STATE_INITIAL state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
sax_event_t sax_parser_state_init(sax_parser_t *restrict parser, const char *restrict glyph) {

  switch (glyph[0]) {

  case SAXAMAPHONE_SPACE:
    return 0;

  case '<':
    parser->primary_state = SAX_STATE_TAG;
    return 0;

  default:
    return sax_parser_error_unexpected_glyph(parser, glyph);
  }
}

/// @brief Handle the glyph when in the SAX_STATE_TAG state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_tag(sax_parser_t *restrict parser, const char *restrict glyph) {

  switch (glyph[0]) {

  case SAXAMAPHONE_SPACE:
    return sax_parser_error_unexpected_glyph(parser, glyph);

  case '?':
    if (parser->stage == NULL) {
      parser->primary_state = SAX_STATE_PROC_INST;
      return 0;
    }

    // Comment or CDATA started
    return sax_parser_error_unexpected_glyph(parser, glyph);

  case '/':

    if (parser->stage == NULL) {
      parser->primary_state = SAX_STATE_TAG_END;
      return 0;
    }

    // Comment or CDATA started
    return sax_parser_error_unexpected_glyph(parser, glyph);

  case '!':
    if (parser->stage == NULL) {
      return sax_parser_append(parser, &parser->stage, glyph);
    }

    return sax_parser_error_unexpected_glyph(parser, glyph);

  default:
    if (parser->stage == NULL) {
      if (strchr(SAXAMAPHONE_EXCLUDE_TAG_PREFIX, glyph[0]) != NULL) {
        return sax_parser_error_unexpected_glyph(parser, glyph);
      }

      parser->primary_state = SAX_STATE_TAG_START;
      return sax_parser_append(parser, &parser->data, glyph);
    }

    if (SAX_EVENT_ERROR == sax_parser_append(parser, &parser->stage, glyph)) {
      return SAX_EVENT_ERROR;
    }

    if (sax_str_eq("!--", parser->stage)) {
      sax_parser_reset(parser);
      parser->primary_state = SAX_STATE_COMMENT;
      return 0;
    }

    if (sax_str_eq("![CDATA[", parser->stage)) {
      sax_parser_reset(parser);
      parser->primary_state = SAX_STATE_CDATA;
      return 0;
    }

    // Building token
    if (sax_startswith("!--", parser->stage) || sax_startswith("![CDATA[", parser->stage)) {
      return 0;
    }

    return sax_parser_error_unexpected_glyph(parser, glyph);
  }
}

/// @brief Handle the glyph when in the SAX_STATE_PROC_INST state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_proc_inst(sax_parser_t *restrict parser, const char *restrict glyph) {

  switch (glyph[0]) {

  case SAXAMAPHONE_SPACE:
    if (sax_str_empty(parser->data)) {
      return sax_parser_error_unexpected_glyph(parser, glyph);
    }
    parser->secondary_state = SAX_STATE_SPACE;
    return 0;

  case '?':
    if (sax_strlen(parser->data) > 0) {
      parser->secondary_state = SAX_STATE_TAG_CLOSE;
      return 0;
    }
    return sax_parser_error_unexpected_glyph(parser, glyph);

  default:
    if (sax_strlen(parser->data) > 0) {
      if (strchr(SAXAMAPHONE_EXCLUDE_TAG, glyph[0]) != NULL) {
        return sax_parser_error_unexpected_glyph(parser, glyph);
      }
    } else if (strchr(SAXAMAPHONE_EXCLUDE_TAG_PREFIX, glyph[0]) != NULL) {
      return sax_parser_error_unexpected_glyph(parser, glyph);
    }

    return sax_parser_append(parser, &parser->data, glyph);
  }
}

/// @brief Handle the glyph when in the SAX_STATE_PROC_INST | SAX_STATE_TAG_CLOSE state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_proc_inst_close(sax_parser_t *restrict parser, const char *restrict glyph) {

  if (glyph[0] == '>') {
    parser->primary_state = sax_str_eq("xml", parser->data)
                                ? SAX_STATE_INIT
                                : SAX_STATE_CONTENT;
    parser->secondary_state = SAX_STATE_NONE;
    return SAX_EVENT_PROCESSING_INSTRUCTION;
  }

  return sax_parser_error_unexpected_glyph(parser, glyph);
}

/// @brief Handle the glyph when in the SAX_STATE_COMMENT state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_comment(sax_parser_t *restrict parser, const char *restrict glyph) {

  if (SAX_EVENT_ERROR == sax_parser_append(parser, &parser->data, glyph)) {
    return SAX_EVENT_ERROR;
  }

  if (sax_endswith(parser->data, "-->")) {
    parser->primary_state = SAX_STATE_CONTENT;
    sax_parser_reset(parser);
  }

  return 0;
}

/// @brief Handle the glyph when in the SAX_STATE_PROC_INST | SAX_STATE_SPACE state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_proc_inst_space(sax_parser_t *restrict parser, const char *restrict glyph) {

  switch (glyph[0]) {

  case SAXAMAPHONE_SPACE:
    // noop
    return 0;

  case '?':
    return sax_parser_append(parser, &parser->stage, glyph);

  case '>':
    if (sax_str_eq("?", parser->stage)) {
      parser->primary_state = sax_str_eq("xml", parser->data)
                                  ? SAX_STATE_INIT
                                  : SAX_STATE_CONTENT;
      parser->secondary_state = SAX_STATE_NONE;
      SAXAMAPHONE_LOG("Parsed processing instruction \"%s\"", parser->data);
      return SAX_EVENT_PROCESSING_INSTRUCTION;
    }
    return sax_parser_error_unexpected_glyph(parser, glyph);

  default:

    parser->secondary_state = SAX_STATE_ATTR_NAME;

    // Starting attr
    if (strchr(SAXAMAPHONE_EXCLUDE_TAG_PREFIX, glyph[0]) != NULL) {
      return sax_parser_error_unexpected_glyph(parser, glyph);
    }

    // Create attribute
    parser->current_attr = sax_arena_alloc(&parser->arena, sizeof(sax_attr_t));
    if (parser->current_attr == NULL) {
      sax_parser_error(parser, "Out of memory");
      return SAX_EVENT_ERROR;
    }
    memset(parser->current_attr, 0, sizeof(sax_attr_t));

    // Link
    if (parser->attrs == NULL) {
      parser->attrs = parser->current_attr;
    } else {
      sax_attr_t *restrict attr = parser->attrs;
      for (; attr->next != NULL; attr = attr->next) {
      }
      attr->next = parser->current_attr;
    }

    // Populate
    return sax_parser_append(parser, &parser->current_attr->name, glyph);
  }
}

/// @brief Handle the glyph when in the (SAX_STATE_PROC_INST or SAX_STATE_TAG_START)
///    | SAX_STATE_ATTR_NAME state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_attr_name(sax_parser_t *restrict parser, const char *restrict glyph) {

  switch (glyph[0]) {

  case '=':
    SAXAMAPHONE_LOG("Parsed attribute name \"%s\"", parser->current_attr->name);
    parser->secondary_state = SAX_STATE_ATTR_ASSIGN;
    return 0;

  case SAXAMAPHONE_SPACE:
    return sax_parser_error_unexpected_glyph(parser, glyph);

  default:
    if (strchr(SAXAMAPHONE_EXCLUDE_TAG, glyph[0]) != NULL) {
      return sax_parser_error_unexpected_glyph(parser, glyph);
    }
    return sax_parser_append(parser, &parser->current_attr->name, glyph);
  }
}

/// @brief Handle the glyph when in the (SAX_STATE_PROC_INST or SAX_STATE_TAG_START)
///    | SAX_STATE_ATTR_ASSIGN state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_attr_assign(sax_parser_t *restrict parser, const char *restrict glyph) {

  switch (glyph[0]) {

  case '"':
    parser->secondary_state = SAX_STATE_ATTR_VALUE;
    return 0;

  default:
    return sax_parser_error_unexpected_glyph(parser, glyph);
  }
}

/// @brief Handle the glyph when in the (SAX_STATE_PROC_INST or SAX_STATE_TAG_START)
///    | SAX_STATE_ATTR_VALUE state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_attr_value(sax_parser_t *restrict parser, const char *restrict glyph) {

  switch (glyph[0]) {

  case '"':
    SAXAMAPHONE_LOG("Parsed attribute value \"%s\"", parser->current_attr->value);
    parser->current_attr = NULL;
    parser->secondary_state = SAX_STATE_SPACE;
    return 0;

  case '&':
    parser->secondary_state = SAX_STATE_ESC_CHAR;
    return sax_parser_append(parser, &parser->stage, glyph);

  default:
    return sax_parser_append(parser, &parser->current_attr->value, glyph);
  }
}

/// @brief Handle the glyph when in the SAX_STATE_TAG_START state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_tag_start(sax_parser_t *restrict parser, const char *restrict glyph) {

  switch (glyph[0]) {

  case '>':
    parser->primary_state = SAX_STATE_CONTENT;
    SAXAMAPHONE_LOG("Parsed start tag \"%s\"", parser->data);
    return SAX_EVENT_START_TAG;

  case '/':
    parser->secondary_state = SAX_STATE_TAG_CLOSE;
    SAXAMAPHONE_LOG("Parsed start tag \"%s\"", parser->data);
    return SAX_EVENT_START_TAG;

  case SAXAMAPHONE_SPACE:
    parser->secondary_state = SAX_STATE_SPACE;
    return 0;

  default:
    if (strchr(SAXAMAPHONE_EXCLUDE_TAG, glyph[0]) == NULL) {
      return sax_parser_append(parser, &parser->data, glyph);
    }
    return sax_parser_error_unexpected_glyph(parser, glyph);
  }
}

/// @brief Handle the glyph when in the SAX_STATE_TAG_START | SAX_STATE_TAG_CLOSE state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_tag_start_close(sax_parser_t *restrict parser, const char *restrict glyph) {
  switch (glyph[0]) {
  case '>':
    parser->primary_state = SAX_STATE_CONTENT;
    parser->secondary_state = SAX_STATE_NONE;
    return SAX_EVENT_END_TAG;

  default:
    return sax_parser_error_unexpected_glyph(parser, glyph);
  }
}

/// @brief Handle the glyph when in the SAX_STATE_TAG_START | SAX_STATE_SPACE state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_tag_start_space(sax_parser_t *restrict parser, const char *restrict glyph) {
  switch (glyph[0]) {

  case SAXAMAPHONE_SPACE:
    // noop
    return 0;

  case '/':
    parser->secondary_state = SAX_STATE_TAG_CLOSE;
    SAXAMAPHONE_LOG("Parsed start tag \"%s\"", parser->data);
    return SAX_EVENT_START_TAG;

  case '>':
    parser->primary_state = SAX_STATE_CONTENT;
    parser->secondary_state = SAX_STATE_NONE;
    SAXAMAPHONE_LOG("Parsed start tag \"%s\"", parser->data);
    return SAX_EVENT_START_TAG;

  default:

    parser->secondary_state = SAX_STATE_ATTR_NAME;

    // Starting attr
    if (strchr(SAXAMAPHONE_EXCLUDE_TAG_PREFIX, glyph[0]) != NULL) {
      return sax_parser_error_unexpected_glyph(parser, glyph);
    }

    // Create attribute
    parser->current_attr = sax_arena_alloc(&parser->arena, sizeof(sax_attr_t));
    if (parser->current_attr == NULL) {
      sax_parser_error(parser, "Out of memory");
      return SAX_EVENT_ERROR;
    }
    memset(parser->current_attr, 0, sizeof(sax_attr_t));

    // Link
    if (parser->attrs == NULL) {
      parser->attrs = parser->current_attr;
    } else {
      sax_attr_t *restrict attr = parser->attrs;
      for (; attr->next != NULL; attr = attr->next) {
      }
      attr->next = parser->current_attr;
    }

    // Populate
    return sax_parser_append(parser, &parser->current_attr->name, glyph);
  }
}

/// @brief Handle the glyph when in the SAX_STATE_CONTENT state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_content(sax_parser_t *restrict parser, const char *restrict glyph) {

  switch (glyph[0]) {

  case '<':
    parser->primary_state = SAX_STATE_TAG;
    if (sax_str_isspace(parser->data)) {
      sax_parser_reset(parser);
      return 0;
    }

    if (!parser->untrimmed_content) {
      parser->data = sax_trim(parser->data);
    }

    SAXAMAPHONE_LOG("Parsed content \"%s\"", parser->data);
    return SAX_EVENT_CONTENT;

  case '&':
    parser->secondary_state = SAX_STATE_ESC_CHAR;
    return sax_parser_append(parser, &parser->stage, glyph);

  default:
    return sax_parser_append(parser, &parser->data, glyph);
  }
}

/// @brief Handle the glyph when in the SAX_STATE_CDATA state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_cdata(sax_parser_t *restrict parser, const char *restrict glyph) {

  if (SAX_EVENT_ERROR == sax_parser_append(parser, &parser->data, glyph)) {
    return SAX_EVENT_ERROR;
  }

  if (sax_endswith(parser->data, "]]>")) {
    const size_t len = strlen(parser->data);
    parser->data[len - 3] = '\0';
    parser->primary_state = SAX_STATE_CONTENT;
    return SAX_EVENT_CONTENT;
  }

  return 0;
}

/// @brief Handle the glyph when in the (SAX_STATE_PROC_INST, SAX_STATE_TAG_START
///   or SAX_STATE_CONTENT) | SAX_STATE_ESC_CHAR state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_esc_char(sax_parser_t *restrict parser, const char *restrict glyph) {

  if (glyph[0] != ';') {
    return sax_parser_append(parser, &parser->stage, glyph);
  }

  if (SAX_EVENT_ERROR == sax_parser_append(parser, &parser->stage, glyph)) {
    return SAX_EVENT_ERROR;
  }

  char buf[5];
  const char *unesc = sax_unescape(parser->stage, buf);
  uint_fast8_t ev = 0;

  switch (parser->primary_state) {
  case SAX_STATE_CONTENT:
    parser->secondary_state = SAX_STATE_NONE;
    ev = sax_parser_append(parser, &parser->data, unesc);
    parser->stage = NULL;
    return ev;

  case SAX_STATE_PROC_INST:
  case SAX_STATE_TAG_START:
    parser->secondary_state = SAX_STATE_ATTR_VALUE;
    ev = sax_parser_append(parser, &parser->current_attr->value, unesc);
    parser->stage = NULL;
    return ev;

  default:
    parser->data = "Unreachable section";
    return SAX_EVENT_ERROR;
  }
}

/// @brief Handle the glyph when in the SAX_STATE_TAG_END state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_tag_end(sax_parser_t *restrict parser, const char *restrict glyph) {
  switch (glyph[0]) {
  case '>':
    if (sax_strlen(parser->data) > 0) {
      parser->primary_state = SAX_STATE_CONTENT;
      return SAX_EVENT_END_TAG;
    }
    return sax_parser_error_unexpected_glyph(parser, glyph);

  case SAXAMAPHONE_SPACE:
    parser->secondary_state = SAX_STATE_SPACE;
    return 0;

  default: {
    const char *exclude = parser->data == NULL ? SAXAMAPHONE_EXCLUDE_TAG_PREFIX : SAXAMAPHONE_EXCLUDE_TAG;
    if (strchr(exclude, glyph[0]) == NULL) {
      return sax_parser_append(parser, &parser->data, glyph);
    }
    return sax_parser_error_unexpected_glyph(parser, glyph);
  }
  }
}

/// @brief Handle the glyph when in the SAX_STATE_TAG_END | SAX_STATE_SPACE state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
static uint_fast8_t sax_parser_state_tag_end_space(sax_parser_t *restrict parser, const char *restrict glyph) {

  switch (glyph[0]) {

  case SAXAMAPHONE_SPACE:
    // noop
    return 0;

  case '>':
    parser->primary_state = SAX_STATE_CONTENT;
    parser->secondary_state = SAX_STATE_NONE;
    return SAX_EVENT_END_TAG;

  default:
    return sax_parser_error_unexpected_glyph(parser, glyph);
  }
}

/// @brief Create the parser instance
/// @param parser allocated parser
/// @param config passed-in config
/// @param arena_bytes allocated arena bytes
/// @param arena_bytes_size number of allocated arena bytes
/// @param file_buffer (NULLable) allocated file buffer bytes
/// @param file_buffer_size number of allocated file buffer bytes
/// @param alloc (NULLable) custom allocator to use
/// @param alloc_ctx (NULLable) custom allocator context to provide to allocator
/// @return the created instance
static sax_parser_t *sax_parser_create(
    sax_parser_t *parser,
    const sax_config_t *restrict config,
    uint8_t *restrict arena_bytes,
    size_t arena_bytes_size,
    uint8_t *restrict file_buffer,
    size_t file_buffer_size,
    void *(*alloc)(void *, void *, size_t),
    void *alloc_ctx) {

  *parser = (sax_parser_t){
      .arena = {
          .bytes = arena_bytes,
          .bytes_size = (uint_fast32_t)arena_bytes_size,
          .alloc = alloc,
          .alloc_ctx = alloc_ctx,
      },
      .untrimmed_content = config->untrimmed_content,
      .line = 1,
      .column = 1,
  };

  if (config->path != NULL) {

#ifdef _MSC_VER
    FILE *fp = NULL;
    fopen_s(&fp, config->path, "r");
#else
    FILE *fp = fopen(config->path, "r");
#endif

    if (!fp) {
      sax_parser_error(parser, "Failed to open \"%s\"", config->path);
      return parser;
    }

    parser->iter = (sax_iter_t){
        .type = SAX_ITER_FILE,
        .impl = {
            .file = {
                .fp = fp,
                .file_buffer = file_buffer,
                .file_buffer_size = file_buffer_size,
            },
        },
    };
  } else if (config->xml != NULL) {
    parser->iter = (sax_iter_t){
        .type = SAX_ITER_STR,
        .impl.str = {
            .value = config->xml,
        },
    };
  } else {
    sax_parser_error(parser, "No XML source provided");
  }

  return parser;
}

/// @brief Create a parser instance using an allocator
/// @param config passed-in config to use
/// @return the created parser
static sax_parser_t *sax_parser_create_alloc(const sax_config_t *restrict config) {

  void *(*alloc)(void *, void *, size_t) = config->alloc == NULL ? sax_default_alloc : config->alloc;
  void *alloc_ctx = config->alloc_ctx;

  sax_parser_t *restrict parser = alloc(alloc_ctx, NULL, sizeof(sax_parser_t));
  if (parser == NULL) {
    SAXAMAPHONE_LOG("Allocator returned NULL for %zu\n", sizeof(sax_parser_t));
    return NULL;
  }
  memset(parser, 0, sizeof(sax_parser_t));

  const size_t arena_size = config->arena_size == 0 ? 1024 : config->arena_size;
  void *arena_bytes = alloc(alloc_ctx, NULL, arena_size);
  if (arena_bytes == NULL) {
    sax_parser_error(parser, "Allocator returned NULL for arena");
    return parser;
  }

  const size_t file_buf_size = config->file_buf_size == 0 ? 4096 : config->file_buf_size;
  void *file_buffer = NULL;

  if (config->path) {
    file_buffer = alloc(alloc_ctx, NULL, file_buf_size);
    if (file_buffer == NULL) {
      sax_parser_error(parser, "Allocator returned NULL for file buffer");
      return parser;
    }
  }

  return sax_parser_create(
      parser,
      config,
      arena_bytes,
      arena_size,
      file_buffer,
      file_buf_size,
      alloc,
      alloc_ctx);
}

/// @brief Create a parser instance using a static buffer
/// @param config passed-in config to use
/// @return the created parser
static sax_parser_t *sax_parser_create_buf(const sax_config_t *restrict config) {

  sax_arena_t arena = {
      .bytes = config->buf,
      .bytes_size = (uint_fast32_t)config->buf_size,
  };

  sax_parser_t *restrict parser = sax_arena_alloc(&arena, sizeof(sax_parser_t));
  if (!parser) {
    SAXAMAPHONE_LOG("ERROR: Unsufficient buffer size {%zu} for allocation %zu\n",
                    config->buf_size,
                    sizeof(sax_parser_t));
    return NULL;
  }

  uint_fast32_t file_buffer_size = 0;
  void *file_buffer = NULL;
  if (config->path != NULL) {
    file_buffer_size = sax_file_buf_size(arena.bytes_size - arena.offset);
    file_buffer = sax_arena_alloc(&arena, file_buffer_size);
    if (file_buffer == NULL) {
      memset(parser, 0, sizeof(sax_parser_t));
      parser->primary_state = SAX_STATE_ERROR;
      parser->data = "Provided buffer size too small; a minimum of 4096 is recommended";
      return parser;
    }
  }

  return sax_parser_create(
      parser,
      config,
      arena.bytes + arena.offset,
      arena.bytes_size - arena.offset,
      file_buffer,
      file_buffer_size,
      NULL,
      NULL);
}

// === public methods === //

SAXAMAPHONE_API sax_parser_t *sax_parser(const sax_config_t *restrict config) {

  if (!config) {
    SAXAMAPHONE_LOG("[SAXAMAPHONE] ERROR: No configuration provided\n");
    return NULL;
  }

  return config->buf == NULL
             ? sax_parser_create_alloc(config)
             : sax_parser_create_buf(config);
}

SAXAMAPHONE_API sax_event_t sax_next(sax_parser_t *restrict parser) {

  if (parser == NULL || parser->primary_state == SAX_STATE_ERROR) {
    return SAX_EVENT_ERROR;
  }

  uint_fast8_t ev = 0;

  /// @brief Largest code pt == 4; +1 null term
  char buf[5];

  // Reset state for next node
  sax_parser_reset(parser);

  for (const char *restrict glyph = sax_iter_next_glyph(&parser->iter, buf);
       glyph[0] != '\0';
       glyph = sax_iter_next_glyph(&parser->iter, buf)) {

    switch (parser->primary_state | parser->secondary_state) {

    case SAX_STATE_INIT:
      ev = sax_parser_state_init(parser, glyph);
      break;

    case SAX_STATE_TAG:
      ev = sax_parser_state_tag(parser, glyph);
      break;

    case SAX_STATE_PROC_INST:
      ev = sax_parser_state_proc_inst(parser, glyph);
      break;

    case SAX_STATE_PROC_INST | SAX_STATE_TAG_CLOSE:
      ev = sax_parser_state_proc_inst_close(parser, glyph);
      break;

    case SAX_STATE_PROC_INST | SAX_STATE_SPACE:
      ev = sax_parser_state_proc_inst_space(parser, glyph);
      break;

    case SAX_STATE_PROC_INST | SAX_STATE_ATTR_NAME:
      ev = sax_parser_state_attr_name(parser, glyph);
      break;

    case SAX_STATE_PROC_INST | SAX_STATE_ATTR_ASSIGN:
      ev = sax_parser_state_attr_assign(parser, glyph);
      break;

    case SAX_STATE_PROC_INST | SAX_STATE_ATTR_VALUE:
      ev = sax_parser_state_attr_value(parser, glyph);
      break;

    case SAX_STATE_PROC_INST | SAX_STATE_ESC_CHAR:
      ev = sax_parser_state_esc_char(parser, glyph);
      break;

    case SAX_STATE_COMMENT:
      ev = sax_parser_state_comment(parser, glyph);
      break;

    case SAX_STATE_TAG_START:
      ev = sax_parser_state_tag_start(parser, glyph);
      break;

    case SAX_STATE_TAG_START | SAX_STATE_TAG_CLOSE:
      ev = sax_parser_state_tag_start_close(parser, glyph);
      break;

    case SAX_STATE_TAG_START | SAX_STATE_SPACE:
      ev = sax_parser_state_tag_start_space(parser, glyph);
      break;

    case SAX_STATE_TAG_START | SAX_STATE_ATTR_NAME:
      ev = sax_parser_state_attr_name(parser, glyph);
      break;

    case SAX_STATE_TAG_START | SAX_STATE_ATTR_ASSIGN:
      ev = sax_parser_state_attr_assign(parser, glyph);
      break;

    case SAX_STATE_TAG_START | SAX_STATE_ATTR_VALUE:
      ev = sax_parser_state_attr_value(parser, glyph);
      break;

    case SAX_STATE_TAG_START | SAX_STATE_ESC_CHAR:
      ev = sax_parser_state_esc_char(parser, glyph);
      break;

    case SAX_STATE_CONTENT:
      ev = sax_parser_state_content(parser, glyph);
      break;

    case SAX_STATE_CDATA:
      ev = sax_parser_state_cdata(parser, glyph);
      break;

    case SAX_STATE_CONTENT | SAX_STATE_ESC_CHAR:
      ev = sax_parser_state_esc_char(parser, glyph);
      break;

    case SAX_STATE_TAG_END:
      ev = sax_parser_state_tag_end(parser, glyph);
      break;

    case SAX_STATE_TAG_END | SAX_STATE_SPACE:
      ev = sax_parser_state_tag_end_space(parser, glyph);
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
      return (sax_event_t)ev;
    }
  }

  switch (parser->primary_state) {

  case SAX_STATE_INIT:
  case SAX_STATE_CONTENT:
    if (sax_str_empty(parser->data)) {
      return SAX_EVENT_END_DOCUMENT;
    }

  default:
    sax_parser_error(
        parser,
        "Unexpected termination at line %" PRIuFAST32 " column %" PRIuFAST32,
        parser->line,
        parser->column);
    return SAX_EVENT_ERROR;
  }
}

SAXAMAPHONE_API const char *sax_error(const sax_parser_t *restrict parser) {

  if (!parser) {
    return NULL;
  }

  return parser->primary_state == SAX_STATE_ERROR ? parser->data : NULL;
}

SAXAMAPHONE_API const char *sax_tag(const sax_parser_t *restrict parser) {
  return parser ? parser->data : NULL;
}

SAXAMAPHONE_API const char *sax_content(const sax_parser_t *restrict parser) {
  return parser ? parser->data : NULL;
}

SAXAMAPHONE_API const sax_attr_t *sax_attrs(const sax_parser_t *restrict parser) {
  return parser ? parser->attrs : NULL;
}

SAXAMAPHONE_API const char *sax_attr(const sax_parser_t *restrict parser, const char *restrict name) {

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

SAXAMAPHONE_API void sax_parser_free(sax_parser_t *restrict parser) {

  if (!parser) {
    return;
  }

  if (parser->arena.alloc == NULL) {
    // Used sax_config_t::buf
    return;
  }

  void *(*alloc)(void *, void *, size_t) = parser->arena.alloc;
  void *alloc_ctx = parser->arena.alloc_ctx;

  if (parser->iter.type == SAX_ITER_FILE) {
    if (parser->iter.impl.file.fp != NULL) {
      fclose(parser->iter.impl.file.fp);
    }
    alloc(alloc_ctx, parser->iter.impl.file.file_buffer, 0);
  }

  alloc(alloc_ctx, parser->arena.bytes, 0);
  memset(&parser->arena, 0, sizeof(sax_arena_t));
  alloc(alloc_ctx, parser, 0);
}
