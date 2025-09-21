#include <ctype.h>    // isspace
#include <inttypes.h> // PRIuFAST16
#include <stdarg.h>   // va_start
#include <stdbool.h>
#include <stdio.h>  // fopen
#include <stdlib.h> // atoll
#include <string.h> // memset

#include <saxamaphone.h>

// TODO: Make NULL term strings
// TODO: Allign alloc

#define SAXAMAPHONE_DEBUG

#ifndef SAXAMAPHONE_FILE_BUFFER_SIZE
#define SAXAMAPHONE_FILE_BUFFER_SIZE 4096
#endif

#ifndef SAXAMAPHONE_NODE_BUFFER_SIZE
#define SAXAMAPHONE_NODE_BUFFER_SIZE 2048
#endif

#define SAXAMAPHONE_STRINGIFY(val) #val
// #define SAXAMAPHONE_DEBUG

#if defined(__clang__) || defined(__GNUC__)
#define SAXAMAPHONE_THREAD_LOCAL __thread
#elif defined(_MSC_VER)
#define SAXAMAPHONE_THREAD_LOCAL __declspec(thread)
#elif __STDC_VERSION__ >= 201112L
#define SAXAMAPHONE_THREAD_LOCAL _Thread_local
#else
#error "Compiler does not support thread-local storage"
#endif

// === typedefs === //

typedef uint8_t byte_t;

typedef enum {
  SAX_STATE_INIT,
  SAX_STATE_IN_TAG,
  SAX_STATE_IN_START_TAG,
  SAX_STATE_CLOSING_START_TAG,
  SAX_STATE_IN_ESC_CHAR,
  SAX_STATE_IN_CONTENT,
  SAX_STATE_IN_END_TAG,
  SAX_STATE_IN_ATTR_NAME,
  SAX_STATE_IN_ATTR_VALUE,
  SAX_STATE_IN_COMMENT,
  SAX_STATE_IN_PROC_INST,
  SAX_STATE_ERROR,
} sax_state_t;

static const char *SAXAMAPHONE_STATES[] = {
    SAXAMAPHONE_STRINGIFY(SAX_STATE_INIT),
    SAXAMAPHONE_STRINGIFY(SAX_STATE_IN_TAG),
    SAXAMAPHONE_STRINGIFY(SAX_STATE_IN_START_TAG),
    SAXAMAPHONE_STRINGIFY(SAX_STATE_IN_CONTENT),
    SAXAMAPHONE_STRINGIFY(SAX_STATE_IN_END_TAG),
    SAXAMAPHONE_STRINGIFY(SAX_STATE_IN_ATTR_NAME),
    SAXAMAPHONE_STRINGIFY(SAX_STATE_IN_ATTR_VALUE),
    SAXAMAPHONE_STRINGIFY(SAX_STATE_IN_COMMENT),
    SAXAMAPHONE_STRINGIFY(SAX_STATE_IN_PROC_INST),
    SAXAMAPHONE_STRINGIFY(SAX_STATE_ERROR),
};

typedef enum {
  SAX_ITER_FILE,
  SAX_ITER_STR,
} sax_iter_type_t;

typedef struct sax_alloc_t {
  uint8_t *arena;
  size_t offset;
  size_t arena_size;
} sax_alloc_t;

typedef struct sax_file_iter_t {
  FILE *fp;
  sax_size_t offset;
  sax_size_t bytes_read;
  uint8_t *file_buffer;
  size_t file_buffer_size;
} sax_file_iter_t;

typedef struct sax_str_iter_t {
  const char *value;
  sax_size_t offset;
  sax_size_t len;
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

#define PRISAXSIZE PRIuFAST16
typedef uint_fast32_t sax_size_t;

struct sax_parser_t {

  bool untrimmed_content;

  sax_alloc_t alloc;
  sax_iter_t iter;
  sax_state_t state;
  sax_state_t prev_state;
  /// @brief Used for error (SAX_EVENT_ERROR), tag (SAX_EVENT_START_TAG), or content (SAX_EVENT_CONTENT)
  const char *msg;
  sax_size_t line;
  sax_size_t column;
  sax_attr_t *attr;
};

// === undocumented api declarations === //

/// @brief Create a substring
/// @param src value to substring
/// @param start inclusive start character offset
/// @param len number of bytes to leave
/// @return resulting substring
char *sax_str_substr(
    char *src,
    sax_size_t start,
    sax_size_t len);

// === constants === //

#define SAX_SIZE_MAX UINT_FAST32_MAX

#define SAXAMAPHONE_EXCLUDE_TAG "!\"#$%&'()*+,/;<=>?@[\\]^`{|}~"
#define SAXAMAPHONE_EXCLUDE_TAG_PREFIX (SAXAMAPHONE_EXCLUDE_TAG ".-0123456789")

#define SAXAMAPHONE_SPACE \
  ' ' : case '\f':        \
  case '\n':              \
  case '\r':              \
  case '\t':              \
  case '\v'

#if SAXAMAPHONE_FILE_BUFFER_SIZE > 0
static SAXAMAPHONE_THREAD_LOCAL uint8_t SAXAMAPHONE_FILE_BUFFER[SAXAMAPHONE_FILE_BUFFER_SIZE];
#endif

#if SAXAMAPHONE_NODE_BUFFER_SIZE > 0
static SAXAMAPHONE_THREAD_LOCAL uint8_t SAXAMAPHONE_NODE_BUFFER[SAXAMAPHONE_NODE_BUFFER_SIZE];
#endif

// === private methods === //

#ifdef SAXAMAPHONE_DEBUG
#define SAXAMAPHONE_LOG(...)                                              \
  printf("[SAXAMAPHONE] %s:%d ", (strrchr(__FILE__, '/') + 1), __LINE__); \
  printf(__VA_ARGS__)
#else
#define SAXAMAPHONE_LOG(...) ((void)0)
#endif

/// @brief Given the provided byte determine the UTF-8 code point size
/// @param byte byte value
/// @return size 1-4 if value; 0 if invalid
static sax_size_t sax_code_pt_size(byte_t byte) {

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

static sax_size_t sax_long_to_code_pt(long value, char *out) {

  if (value < 0 || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) {
    return 0;
  }

  if (value <= 0x7F) {
    out[0] = (char)value;
    return 1;
  }

  if (value <= 0x7FF) {
    out[0] = (char)(0xC0 | ((value >> 6) & 0x1F));
    out[1] = (char)(0x80 | (value & 0x3F));
    return 2;
  }

  if (value <= 0xFFFF) {
    out[0] = (char)(0xE0 | ((value >> 12) & 0x0F));
    out[1] = (char)(0x80 | ((value >> 6) & 0x3F));
    out[2] = (char)(0x80 | (value & 0x3F));
    return 3;
  }

  out[0] = (char)(0xF0 | ((value >> 18) & 0x07));
  out[1] = (char)(0x80 | ((value >> 12) & 0x3F));
  out[2] = (char)(0x80 | ((value >> 6) & 0x3F));
  out[3] = (char)(0x80 | (value & 0x3F));
  return 4;
}

// --- private sax_alloc_t methods --- //

/// @brief Perform an object allocation
/// @param alloc instance
/// @param size in bytes of the allocation
/// @return the pointer or NULL if insufficient memory
static void *sax_alloc(sax_alloc_t *restrict alloc, size_t size) {

  if (alloc->offset + size > alloc->arena_size) {
    return NULL;
  }

  void *res = alloc->arena + alloc->offset;
  alloc->offset += size;
  return res;
}

/// @brief Reset the allocator for a fresh round of allocations
/// @param alloc instance
static void sax_alloc_reset(sax_alloc_t *restrict alloc) {
  alloc->offset = 0;
}

/// @brief Return a new allocator instance which manages the remaining memory
/// @param alloc instance
/// @return arena allocator managing remaining bytes
static sax_alloc_t sax_alloc_partition(sax_alloc_t *restrict alloc) {
  return (sax_alloc_t){
      .arena = alloc->arena + alloc->offset,
      .arena_size = alloc->arena_size - alloc->offset,
  };
}

// --- private sax_str_t methods --- //

/// @brief Test whether the provided string is all spaces
/// @param str string to test
/// @return true if all spaces
static bool sax_str_is_space(const char *str) {

  for (sax_size_t i = 0;
       str[i] != 0;
       i += sax_code_pt_size(str[i])) {

    if (!isspace(str[i])) {
      return false;
    }
  }

  return true;
}

/// @brief Test whether a string ends with another string
/// @param subject the string to test
/// @param suffix the suffix to match
/// @return true if subject endswith the suffix
static bool sax_str_endswith(const char *subject, const char *suffix) {

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

/// @brief Test whether the provided string is empty
/// @param str to test
/// @return true if the size is zero
static bool sax_str_empty(const char *str) {
  return strlen(str) == 0;
}

static bool sax_str_startswith(const char *src, const char *prefix) {

  const size_t prefix_size = strlen(prefix);
  const size_t src_size = strlen(src);

  if (prefix_size > src_size) {
    return false;
  }

  return strncmp(src, prefix, prefix_size) == 0;
}

/// @brief Remove spaces (' ', '\t', '\n', etc) to the left of the first non-space character
/// @param src string to ltrim
/// @return left-trimmed string
static char *sax_str_ltrim(char *src) {

  const size_t src_size = strlen(src);

  for (sax_size_t i = 0; i < src_size; i += sax_code_pt_size(src[i])) {
    if (!isspace(src[i])) {
      src += i;
      break;
    }
  }

  return src;
}

/// @brief Remove spaces (' ', '\t', '\n', etc) to the right of the first non-space character
/// @param src string to rtrim
/// @return right-trimmed string
static char *sax_str_rtrim(char *src) {

  if (sax_str_empty(src)) {
    return src;
  }

  sax_size_t last_non_space = 0;
  const size_t src_size = strlen(src);

  for (sax_size_t i = 0;
       i < src_size;
       i += sax_code_pt_size(src[i])) {

    if (!isspace(src[i])) {
      last_non_space = i;
    }
  }

  src[last_non_space + 1] = '\0';
  return src;
}

/// @brief Perform both an ltrim & rtrim
/// @param str to trim
/// @return trimmed string
static char *sax_str_trim(char *str) {
  return sax_str_ltrim(sax_str_rtrim(str));
}

// static sax_str_t sax_str_rfind(const sax_str_t str, char c) {

//   sax_size_t offset = SAX_SIZE_MAX;

//   for (sax_size_t i = 0; i < str.size; i += sax_code_pt_size(str.value[i])) {
//     if (str.value[i] == c) {
//       offset = i;
//     }
//   }

//   return offset == SAX_SIZE_MAX
//              ? SAXAMAPHONE_EMPTY_STRING
//              : (sax_str_t){
//                    .value = str.value + offset,
//                    .size = str.size - offset,
//                };
// }

// --- private sax_iter_t methods --- //

/// @brief Close any lingering resources
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

/// @brief Retrieve the next byte value or 0 if iteration is complete
/// @param iter iterator
/// @return next byte value or 0 if completed
static byte_t sax_iter_next_byte(sax_iter_t *restrict iter) {

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

    return (byte_t)fiter->file_buffer[fiter->offset++];
  }

  case SAX_ITER_STR: {
    sax_str_iter_t *restrict siter = &iter->impl.str;
    if (!siter->value) {
      return 0;
    }

    const byte_t value = (byte_t)siter->value[siter->offset++];
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
/// @return glyph string sized 0-4 bytes
static const char *sax_iter_next_glyph(sax_iter_t *restrict iter) {

  byte_t byte = sax_iter_next_byte(iter);
  if (byte == 0) {
    sax_iter_close(iter);
    return "";
  }

  iter->glyph[0] = (char)byte;
  const sax_size_t pt_size = sax_code_pt_size(byte);

  sax_size_t i = 1;
  for (; i < pt_size && byte != 0; ++i) {
    byte = sax_iter_next_byte(iter);
    iter->glyph[i] = (char)byte;
  }
  iter->glyph[i] = '\0';
  return iter->glyph;
}

// --- private parser methods --- //

/// @brief Append a glyph to the current message (tag or content)
/// @param parser
/// @param glyph
static void sax_parser_msg_append(sax_parser_t *restrict parser, const char *glyph) {

  const uint8_t *end = parser->alloc.arena + parser->alloc.arena_size;
  const size_t glyph_size = strlen(glyph);

  if (!parser->msg) {
    // We're kind of abusing the arena here; the first "allocation" from the
    //   arena should always be msg (tag name or content)
    parser->msg = parser->alloc.arena;
  }

  const size_t msg_size = strlen(parser->msg);

  if ((uint8_t *)(parser->msg + msg_size + glyph_size + 1) > end) {
    parser->msg = "Out of memory";
    sax_parser_state(parser, SAX_STATE_ERROR);
    return;
  }

  strcpy(parser->msg + msg_size, glyph);

  // Ensure subsequent "allocations" are after msg
  parser->alloc.offset = msg_size + glyph_size + 1;
}

static void sax_parser_state(sax_parser_t *restrict parser, sax_state_t state) {
  parser->prev_state = parser->state;
  parser->state = state;
}

/// @brief Set the parser to the error state
/// @param parser instance
/// @param format message format
/// @param  ... message args
static void sax_parser_error(sax_parser_t *restrict parser, const char *restrict format, ...) {

  va_list args;
  va_start(args, format);

  sax_parser_state(parser, SAX_STATE_ERROR);
  vsnprintf((char *)parser->alloc.arena, parser->alloc.arena_size, format, args);
  parser->msg = parser->alloc.arena;

  va_end(args);
}

/// @brief Throw the parser into an error state
/// @param parser instance
/// @param glyph the unexpected glyph
/// @return SAX_EVENT_ERROR
static sax_event_t sax_parser_error_unexpected_glyph(sax_parser_t *restrict parser, const char *glyph) {

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

    parser->msg = sax_alloc(&parser->alloc, strlen(glyph) + 1);
    strcpy(parser->msg, glyph);
    sax_parser_state(parser, SAX_STATE_IN_START_TAG);
    break;
  }

  return 0;
}

static sax_event_t sax_parser_state_in_escaped_char(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {
  case ';':
    // TODO
    // sax_str_t node = sax_parser_node(parser);
    // sax_str_t esc = sax_str_rfind(node, '&');
    // sax_str_t unesc = sax_str_unescaped(esc);
    sax_parser_state(parser, parser->prev_state);
    break;

  default:
    sax_parser_msg_append(parser, glyph);
    break;
  }

  return 0;
}

static sax_event_t sax_parser_state_in_start_tag(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {

  case '/':
    sax_parser_state(parser, SAX_STATE_CLOSING_START_TAG);
    break;

  case '>':

    if (parser->msg == NULL || strlen(parser->msg) == 0) {
      sax_parser_error(
          parser,
          "Empty tag found at line %d column %d",
          parser->line,
          parser->column);
      return SAX_EVENT_ERROR;
    }

    sax_parser_state(parser, SAX_STATE_IN_CONTENT);
    return SAX_EVENT_START_ELEMENT;

  case SAXAMAPHONE_SPACE:
    sax_parser_state(parser, SAX_STATE_IN_ATTR_NAME);
    break;

  default:

    if (strchr(SAXAMAPHONE_EXCLUDE_TAG, glyph[0])) {
      sax_parser_error(parser, "Tag names cannot contain '%c'", glyph[0]);
    }

    sax_parser_msg_append(parser, glyph);

    break;
  }

  return 0;
}

static sax_event_t sax_parser_state_in_content(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {

  case '&':
    sax_parser_msg_append(parser, glyph);
    sax_parser_state(parser, SAX_STATE_IN_ESC_CHAR);
    break;

  case '<': {
    sax_parser_state(parser, SAX_STATE_IN_TAG);
    parser->msg = parser->msg ? parser->msg : "";

    if (parser->untrimmed_content) {
      parser->msg = sax_str_trim(parser->msg);
    }

    if (sax_str_is_space(parser->msg)) {
      parser->msg = NULL;
      sax_alloc_reset(&parser->alloc);
    } else {
      return SAX_EVENT_CONTENT;
    }

  } break;

  default:
    sax_parser_msg_append(parser, glyph);
    break;
  }

  return 0;
}

static sax_event_t sax_parser_state_in_end_tag(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph[0]) {
  case '>':
    if (parser->msg == NULL || strlen(parser->msg) == 0) {
      sax_parser_error(parser, "Empty closing tag found at line %d column %d", parser->line, parser->column);
      return SAX_EVENT_ERROR;
    }
    sax_parser_state(parser, SAX_STATE_IN_CONTENT);
    return SAX_EVENT_END_ELEMENT;

  case SAXAMAPHONE_SPACE:
    sax_parser_error_unexpected_glyph(parser, glyph);
    break;

  default:
    if (strchr(SAXAMAPHONE_EXCLUDE_TAG, glyph[0])) {
      sax_parser_error_unexpected_glyph(parser, glyph);
      return SAX_EVENT_ERROR;
    }

    sax_parser_msg_append(parser, glyph);
    break;
  }

  return 0;
}

static sax_event_t sax_parser_state_in_attr_name(sax_parser_t *restrict parser, const char *glyph) {

  switch (glyph.value[0]) {

  case '>':
    sax_parser_state(parser, SAX_STATE_IN_CONTENT);
    return SAX_EVENT_START_ELEMENT;

  case SAXAMAPHONE_SPACE:
    if (++parser->attr_offset == SAXAMAPHONE_ATTR_MAX) {
      sax_parser_state(parser, SAX_STATE_ERROR);
      parser->error = sax_str("Maximum XML attributes exceeded");
      return SAX_EVENT_ERROR;
    }

    break;

  case '=':
    sax_parser_state(parser, SAX_STATE_IN_ATTR_VALUE);
    break;

  default: {
    sax_attr_t *restrict attr = &parser->attrs[parser->attr_offset];
    if (sax_str_empty(attr->name)) {
      attr->name = (sax_str_t){
          .size = 0,
          .value = (char *)parser->alloc.arena + (parser->alloc.offset - glyph.size),
      };
    }
    attr->name.size += glyph.size;
  } break;
  }

  return 0;
}

static sax_event_t sax_parser_state_in_attr_value(sax_parser_t *restrict parser, const sax_str_t glyph) {

  switch (glyph.value[0]) {

  case '>':
    sax_parser_state(parser, SAX_STATE_IN_CONTENT);
    return SAX_EVENT_START_ELEMENT;

  case '"': {
    const sax_str_t node = sax_parser_node(parser);
    if (sax_str_endswith(node, sax_str("\"\""))) {
      if (++parser->attr_offset == SAXAMAPHONE_ATTR_MAX) {
        sax_parser_state(parser, SAX_STATE_ERROR);
        parser->error = sax_str("Maximum XML attributes exceeded");
        return SAX_EVENT_ERROR;
      }
    }

    sax_attr_t *restrict attr = &parser->attrs[parser->attr_offset];
    if (sax_str_empty(attr->value)) {
      // initializing
      attr->value.value = (char *)parser->alloc.arena + parser->alloc.offset;
    } else {
      sax_parser_state(parser, SAX_STATE_IN_ATTR_NAME);
    }
  } break;

  default: {
    sax_attr_t *restrict attr = &parser->attrs[parser->attr_offset];
    attr->value.size += glyph.size;
  } break;
  }

  return 0;
}

static sax_event_t sax_parser_state_in_comment(sax_parser_t *restrict parser, const sax_str_t glyph) {

  switch (glyph.value[0]) {

  case '>': {
    const sax_str_t subj = {
        .size = parser->alloc.offset,
        .value = (char *)parser->alloc.arena,
    };

    if (sax_str_endswith(subj, sax_str("-->"))) {
      parser->alloc.offset = 0;
      sax_parser_state(parser, SAX_STATE_IN_CONTENT);
    }
  } break;

  default:
    // noop
    break;
  }

  return 0;
}

static sax_event_t sax_parser_state_in_proc_inst(sax_parser_t *restrict parser, const sax_str_t glyph) {

  switch (glyph.value[0]) {
  case '>':
    if (parser->alloc.arena[parser->alloc.offset - 2] == '?') {
      parser->alloc.offset = 0;
      sax_parser_state(parser, SAX_STATE_IN_CONTENT);
    } else {
      return sax_parser_error_unexpected_glyph(parser, glyph);
    }
    break;
  }

  return 0;
}

// === public methods === //

sax_parser_t *sax_parser(const sax_config_t *restrict config) {

  SAXAMAPHONE_LOG("[SAXAMAPHONE] Configuration:\n"
                  "    SAXAMAPHONE_FILE_BUFFER_SIZE=%d\n"
                  "    SAXAMAPHONE_NODE_BUFFER_SIZE=%d\n",
                  SAXAMAPHONE_FILE_BUFFER_SIZE,
                  SAXAMAPHONE_NODE_BUFFER_SIZE);

  if (!config) {
    SAXAMAPHONE_LOG("[SAXAMAPHONE] ERROR: No configuration provided\n");
    return NULL;
  }

  sax_alloc_t alloc = {
#if SAXAMAPHONE_NODE_BUFFER_SIZE == 0
      .arena = config->arena,
      .arena_size = config->arena_size,
#else
      .arena = config->arena ? config->arena : SAXAMAPHONE_NODE_BUFFER,
      .arena_size = config->arena ? config->arena_size : SAXAMAPHONE_NODE_BUFFER_SIZE,
#endif
  };

  sax_parser_t *parser = sax_alloc(&alloc, sizeof(sax_parser_t));

  if (!parser) {
#ifdef SAXAMAPHONE_DEBUG
    printf(
        "[SAXAMAPHONE] Failed to allocate parser; sizeof(sax_parser_t) {%zu} >= %zu\n",
        sizeof(sax_parser_t),
        alloc.arena_size);
#endif

    return NULL;
  }

  *parser = (sax_parser_t){
      .alloc = sax_alloc_partition(&alloc),
      .untrimmed_content = config->untrimmed_content,
  };

  if (config->path) {

#if SAXAMAPHONE_NODE_BUFFER_SIZE == 0
    if (!config->file_buffer || config->file_buffer_size == 0) {
      SAXAMAPHONE_LOG("Path \"%s\" provided but insufficient file_buffer %p sized %zu provided\n",
                      config->path,
                      config->file_buffer,
                      config->file_buffer_size);
      // TODO: error message
      parser->state = SAX_STATE_ERROR;
      return parser;
    }
#endif

    FILE *fp = fopen(config->path, "r");

    if (!fp) {
      sax_parser_error(parser, "Failed to open \"%s\"", config->path);
      return parser;
    }

    parser->iter = (sax_iter_t){
        .type = SAX_ITER_FILE,
        .impl = {
            .file = {
                .fp = fp,
#if SAXAMAPHONE_NODE_BUFFER_SIZE == 0
                .file_buffer = config->file_buffer,
                .file_buffer_size = config->file_buffer_size,
#else
                .file_buffer = config->file_buffer ? config->file_buffer : SAXAMAPHONE_FILE_BUFFER,
                .file_buffer_size = config->file_buffer ? config->file_buffer_size : SAXAMAPHONE_FILE_BUFFER_SIZE,
#endif
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

  if (!parser || parser->state == SAX_STATE_ERROR) {
    return SAX_EVENT_ERROR;
  }

  sax_event_t ev = 0;

  // Reset state
  parser->msg = NULL;
  sax_alloc_reset(&parser->alloc);

  for (sax_str_t glyph = sax_iter_next_glyph(&parser->iter);
       glyph.size > 0;
       glyph = sax_iter_next_glyph(&parser->iter)) {

#ifdef SAXAMAPHONE_DEBUG
    printf("    DEBUG: buffer=\"%.*s\" glyph=\"%.*s\" state=%s\n",
           (int)parser->alloc.offset,
           parser->alloc.arena,
           glyph.size,
           glyph.value,
           SAXAMAPHONE_STATES[parser->state]);
#else
    (void)SAXAMAPHONE_STATES;
#endif

    if (glyph.size + parser->alloc.offset >= parser->alloc.arena_size) {
      parser->error = sax_str("Token buffer overflow");
      sax_parser_state(parser, SAX_STATE_ERROR);
      return SAX_EVENT_ERROR;
    }

    memcpy(parser->alloc.arena + parser->alloc.offset, glyph.value, glyph.size);
    parser->alloc.offset += glyph.size;

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
      parser->error = sax_str("Unreachable section reached");
      ev = SAX_EVENT_ERROR;
      break;
    }

    if (glyph.value[0] == '\n') {
      ++parser->line;
      parser->column = 0;
    } else {
      ++parser->column;
    }

    if (ev != 0) {
      return ev;
    }
  }

  return SAX_EVENT_END_DOCUMENT;
}

sax_str_t sax_error(const sax_parser_t *restrict parser) {

  if (!parser) {
    return sax_str("NULL sax_parser_t provided");
  }

  return parser->error;
}

sax_str_t sax_tag(const sax_parser_t *restrict parser) {
  return parser ? parser->tag : SAXAMAPHONE_EMPTY_STRING;
}

sax_str_t sax_content(const sax_parser_t *restrict parser) {
  return parser ? parser->content : SAXAMAPHONE_EMPTY_STRING;
}

sax_attrs_t sax_attrs(const sax_parser_t *restrict parser) {

  if (!parser) {
    return (sax_attrs_t){0};
  }

  for (sax_size_t i = 0; i < SAXAMAPHONE_ATTR_MAX; ++i) {
    if (parser->attrs[i].name.size == 0) {
      return (sax_attrs_t){
          .attrs = parser->attrs,
          .count = i,
      };
    }
  }

  ((sax_parser_t *restrict)parser)->state = SAX_STATE_ERROR;
  ((sax_parser_t *restrict)parser)->error = sax_str(
      "Unreachable code reached in " __FILE__
      " " SAXAMAPHONE_STRINGIFY(__LINE__));
  return (sax_attrs_t){0};
}

// --- public undocumented methods --- //

char *sax_str_substr(
    char *str,
    sax_size_t start,
    sax_size_t len) {

  size_t size = strlen(str);
  sax_size_t byte_offset = 0;
  sax_size_t char_offset = 0;

  for (;
       byte_offset < size && char_offset < start;
       byte_offset += sax_code_pt_size(str[byte_offset]), ++char_offset) {
  }

  str += byte_offset;
  size = strlen(str);

  for (byte_offset = 0, char_offset = 0;
       byte_offset < size && char_offset < len;
       byte_offset += sax_code_pt_size(str[byte_offset]), ++char_offset) {
  }

  str[byte_offset] = '/0';

  return str;
}

sax_str_t sax_str_unescaped(const sax_str_t src) {

  static SAXAMAPHONE_THREAD_LOCAL char buf[16];

  if (sax_str_equals(src, sax_str("&lt;"))) {
    return (sax_str_t){.size = 1, .value = "<"};
  }

  if (sax_str_equals(src, sax_str("&gt;"))) {
    return (sax_str_t){.size = 1, .value = ">"};
  }

  if (sax_str_equals(src, sax_str("&amp;"))) {
    return (sax_str_t){.size = 1, .value = "&"};
  }

  if (sax_str_equals(src, sax_str("&apos;"))) {
    return (sax_str_t){.size = 1, .value = "'"};
  }

  if (sax_str_equals(src, sax_str("&quot;"))) {
    return (sax_str_t){.size = 1, .value = "\""};
  }

  // if (sax_str_startswith(src, sax_str("&#x")) && sax_str_endswith(src, sax_str(";"))) {

  // }

  if (sax_str_startswith(src, sax_str("&#")) && sax_str_endswith(src, sax_str(";"))) {

    const sax_str_t value = sax_str_substr(src, 2, src.size - 3);
    snprintf(buf, sizeof(buf), "%.*s", value.size, value.value);
    long code_pt = atol(value.value);
    if (code_pt == 0) {
      return src;
    }

    return (sax_str_t){
        .size = sax_long_to_code_pt(code_pt, buf),
        .value = buf,
    };
  }

  return src;
}