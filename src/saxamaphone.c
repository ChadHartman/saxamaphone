#include <ctype.h>    // isspace
#include <inttypes.h> // PRIuFAST16
#include <stdarg.h>   // va_start
#include <stdbool.h>
#include <stdio.h>  // fopen
#include <stdlib.h> // atoll
#include <string.h> // memset

#include <saxamaphone.h>

// TODO: Remove config storage from parser
// TODO: Support provided buffers (and 0 sized thread locals)
// TODO: Make parser instance from buffer
// TODO: Make NULL term strings

#define SAXAMAPHONE_DEBUG

#ifndef SAXAMAPHONE_FILE_BUFFER_SIZE
#define SAXAMAPHONE_FILE_BUFFER_SIZE 4096
#endif

#ifndef SAXAMAPHONE_NODE_BUFFER_SIZE
#define SAXAMAPHONE_NODE_BUFFER_SIZE 2048
#endif

#ifndef SAXAMAPHONE_ATTR_MAX
#define SAXAMAPHONE_ATTR_MAX 32
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
  char glyph[4];

  union {
    sax_file_iter_t file;
    sax_str_iter_t str;
  } impl;

} sax_iter_t;

struct sax_parser_t {
  sax_config_t config;
  sax_alloc_t alloc;
  sax_iter_t iter;
  sax_state_t state;
  sax_state_t prev_state;
  sax_str_t error;
  sax_str_t tag;
  sax_str_t content;
  sax_size_t attr_offset;
  sax_size_t line;
  sax_size_t column;
  sax_attr_t attrs[SAXAMAPHONE_ATTR_MAX];
};

// === undocumented api declarations === //

/// @brief Create a substring
/// @param src value to substring
/// @param start inclusive start character offset
/// @param len number of bytes to leave
/// @return resulting substring
sax_str_t sax_str_substr(
    const sax_str_t src,
    sax_size_t start,
    sax_size_t len);

/// @brief Wrap a NULL-terminated string into a sax_str_t
/// @param value the value to wrap
/// @return sax_str_t instance
sax_str_t sax_str(const char *restrict value);

/// @brief Test whether the provided strings are equal
/// @param lhs left-hand-arg
/// @param rhs right-hand-arg
/// @return true if they are equal and their values match
bool sax_str_equals(const sax_str_t lhs, const sax_str_t rhs);

// === constants === //

#define SAX_SIZE_MAX UINT_FAST32_MAX

#define SAXAMAPHONE_EXCLUDE_TAG_PREFIX "!\"#$%&'()*+,/;<=>?@[\\]^`{|}~ .-0123456789"

#define SAXAMAPHONE_SPACE \
  ' ' : case '\f':        \
  case '\n':              \
  case '\r':              \
  case '\t':              \
  case '\v'

static const sax_str_t SAXAMAPHONE_EMPTY_STRING = {
    .size = 0,
    .value = "",
};

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
static bool sax_str_is_space(const sax_str_t str) {

  for (sax_size_t i = 0;
       i < str.size;
       i += sax_code_pt_size(str.value[i])) {

    if (!isspace(str.value[i])) {
      return false;
    }
  }

  return true;
}

/// @brief Test whether a string ends with another string
/// @param subject the string to test
/// @param suffix the suffix to match
/// @return true if subject endswith the suffix
static bool sax_str_endswith(const sax_str_t subject, const sax_str_t suffix) {

  if (suffix.size == 0) {
    return true;
  }

  if (suffix.size > subject.size) {
    return false;
  }

  const size_t offset = subject.size - suffix.size;
  return strcmp(subject.value + offset, suffix.value) == 0;
}

/// @brief Test whether the provided string is empty
/// @param str to test
/// @return true if the size is zero
static bool sax_str_empty(sax_str_t str) {
  return str.size == 0;
}

static bool sax_str_startswith(const sax_str_t src, const sax_str_t prefix) {

  if (prefix.size > src.size) {
    return false;
  }

  return strncmp(src.value, prefix.value, prefix.size) == 0;
}

/// @brief Remove spaces (' ', '\t', '\n', etc) to the left of the first non-space character
/// @param src string to ltrim
/// @return left-trimmed string
static sax_str_t sax_str_ltrim(const sax_str_t src) {

  sax_str_t copy = SAXAMAPHONE_EMPTY_STRING;

  for (sax_size_t i = 0; i < src.size; i += sax_code_pt_size(src.value[i])) {
    if (!isspace(src.value[i])) {
      copy = (sax_str_t){
          .size = src.size - i,
          .value = src.value + i,
      };
      break;
    }
  }

  return copy;
}

/// @brief Remove spaces (' ', '\t', '\n', etc) to the right of the first non-space character
/// @param src string to rtrim
/// @return right-trimmed string
static sax_str_t sax_str_rtrim(const sax_str_t src) {

  if (sax_str_empty(src)) {
    return src;
  }

  sax_size_t last_non_space = 0;

  for (sax_size_t i = 0;
       i < src.size;
       i += sax_code_pt_size(src.value[i])) {

    if (!isspace(src.value[i])) {
      last_non_space = i;
    }
  }

  return (sax_str_t){
      // +1 to include the last non_space char
      .size = last_non_space + 1,
      .value = src.value,
  };
}

/// @brief Perform both an ltrim & rtrim
/// @param str to trim
/// @return trimmed string
static sax_str_t sax_str_trim(sax_str_t str) {
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
static sax_str_t sax_iter_next_glyph(sax_iter_t *restrict iter) {

  byte_t byte = sax_iter_next_byte(iter);
  if (byte == 0) {
    sax_iter_close(iter);
    return SAXAMAPHONE_EMPTY_STRING;
  }

  iter->glyph[0] = (char)byte;
  const sax_size_t pt_size = sax_code_pt_size(byte);

  for (sax_size_t i = 1; i < pt_size && byte != 0; ++i) {
    byte = sax_iter_next_byte(iter);
    iter->glyph[i] = (char)byte;
  }

  return (sax_str_t){
      .size = pt_size,
      .value = iter->glyph,
  };
}

// --- private parser methods --- //

static void sax_parser_state(sax_parser_t *restrict parser, sax_state_t state) {
  parser->prev_state = parser->state;
  parser->state = state;
}

/// @brief Get a string representation of the contents of the node buffer
/// @param parser instance
/// @return string representation of the node buffer
static sax_str_t sax_parser_node(sax_parser_t *restrict parser) {
  return (sax_str_t){
      .size = parser->alloc.offset,
      .value = (char *)parser->alloc.arena,
  };
}

/// @brief Set the parser to the error state
/// @param parser instance
/// @param format message format
/// @param  ... message args
static void sax_parser_error(sax_parser_t *restrict parser, const char *restrict format, ...) {

  va_list args;
  va_start(args, format);

  sax_parser_state(parser, SAX_STATE_ERROR);
  parser->error = (sax_str_t){
      .size = vsnprintf((char *)parser->alloc.arena, parser->alloc.offset, format, args),
      .value = (char *)parser->alloc.arena,
  };

  va_end(args);
}

/// @brief Throw the parser into an error state
/// @param parser instance
/// @param glyph the unexpected glyph
/// @return SAX_EVENT_ERROR
static sax_event_t sax_parser_error_unexpected_glyph(sax_parser_t *restrict parser, const sax_str_t glyph) {

  sax_parser_error(parser,
                   "Unexpected character \"%.*s\" located on line %" PRIuFAST16 " column %" PRIuFAST16,
                   glyph.size,
                   glyph.value,
                   parser->line,
                   parser->column);

  return SAX_EVENT_ERROR;
}

/// @brief Handle the glyph when in the SAX_STATE_INITIAL state
/// @param parser instance
/// @param glyph glyph to process
/// @return event if raised
sax_event_t sax_parser_state_init(sax_parser_t *restrict parser, const sax_str_t glyph) {

  switch (glyph.value[0]) {
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
static sax_event_t sax_parser_state_in_tag(sax_parser_t *restrict parser, const sax_str_t glyph) {

  // We could be coming from sax_parser_in_content which eats the '<' char
  parser->alloc.arena[0] = '<';
  memcpy(parser->alloc.arena + 1, glyph.value, glyph.size);
  parser->alloc.offset = 1 + glyph.size;

  switch (glyph.value[0]) {

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
    if (strchr(SAXAMAPHONE_EXCLUDE_TAG_PREFIX, glyph.value[0])) {
      return sax_parser_error_unexpected_glyph(parser, glyph);
    }

    parser->tag = (sax_str_t){
        .size = glyph.size,
        // +1 to skip the '<'
        .value = (char *)parser->alloc.arena + 1,
    };
    sax_parser_state(parser, SAX_STATE_IN_START_TAG);
    break;
  }

  return 0;
}

static sax_event_t sax_parser_state_in_escaped_char(sax_parser_t *restrict parser, const sax_str_t glyph) {

  switch (glyph.value[0]) {
  case ';':
    // sax_str_t node = sax_parser_node(parser);
    // sax_str_t esc = sax_str_rfind(node, '&');
    // sax_str_t unesc = sax_str_unescaped(esc);
    sax_parser_state(parser, parser->prev_state);
    break;

  default:
    // noop
    break;
  }

  return 0;
}

static sax_event_t sax_parser_state_in_start_tag(sax_parser_t *restrict parser, const sax_str_t glyph) {

  // TODO: validate valid chars

  switch (glyph.value[0]) {

  case '"':
  case '\'':
  case '!':
  case '?':
    return sax_parser_error_unexpected_glyph(parser, glyph);

  case '/':
    if (parser->tag.size == 0) {
      return sax_parser_error_unexpected_glyph(parser, glyph);
    }
    break;

  case '>':
    if (parser->tag.size == 0) {
      return sax_parser_error_unexpected_glyph(parser, glyph);
    }
    sax_parser_state(parser, SAX_STATE_IN_CONTENT);
    return SAX_EVENT_START_ELEMENT;

  case SAXAMAPHONE_SPACE:
    sax_parser_state(parser, SAX_STATE_IN_ATTR_NAME);
    break;

  default:
    if (sax_str_empty(parser->tag)) {
      parser->tag.value = (char *)parser->alloc.arena + parser->alloc.offset;
    }
    parser->tag.size += glyph.size;
    break;
  }

  return 0;
}

static sax_event_t sax_parser_state_in_content(sax_parser_t *restrict parser, const sax_str_t glyph) {

  switch (glyph.value[0]) {

  case '&':
    sax_parser_state(parser, SAX_STATE_IN_ESC_CHAR);
    break;

  case '<': {
    sax_parser_state(parser, SAX_STATE_IN_TAG);
    sax_str_t content = sax_parser_node(parser);
    --content.size; // For the '<'
    content = parser->config.untrimmed_content ? content : sax_str_trim(content);

    if (sax_str_is_space(parser->content)) {
      parser->alloc.offset = 1;
      parser->alloc.arena[0] = '<';
    } else {
      parser->content = content;
      return SAX_EVENT_CONTENT;
    }
  } break;

  default:
    // noop
    break;
  }

  return 0;
}

static sax_event_t sax_parser_state_in_end_tag(sax_parser_t *restrict parser, const sax_str_t glyph) {

  switch (glyph.value[0]) {
  case '>':
    sax_parser_state(parser, SAX_STATE_IN_CONTENT);
    return SAX_EVENT_END_ELEMENT;

  case SAXAMAPHONE_SPACE:
    sax_parser_error_unexpected_glyph(parser, glyph);
    break;

  default:
    if (parser->tag.size == 0) {
      parser->tag.value = strrchr((char *)parser->alloc.arena, '/') + 1;
    }
    parser->tag.size += glyph.size;
    break;
  }

  return 0;
}

static sax_event_t sax_parser_state_in_attr_name(sax_parser_t *restrict parser, const sax_str_t glyph) {

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
      .config = *config,
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
  parser->tag.size = 0;
  parser->content.size = 0;
  parser->attr_offset = 0;
  parser->alloc.offset = 0;
  memset(parser->attrs, 0, sizeof(parser->attrs));

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

sax_str_t sax_str_substr(
    const sax_str_t src,
    sax_size_t start,
    sax_size_t len) {

  (void)len;

  sax_size_t byte_offset = 0;
  sax_size_t char_offset = 0;

  for (;
       byte_offset < src.size && char_offset < start;
       byte_offset += sax_code_pt_size(src.value[byte_offset]), ++char_offset) {
  }

  sax_str_t res = (sax_str_t){
      .value = src.value + byte_offset,
      .size = src.size - byte_offset,
  };

  for (byte_offset = 0, char_offset = 0;
       byte_offset < res.size && char_offset < len;
       byte_offset += sax_code_pt_size(res.value[byte_offset]), ++char_offset) {
  }

  res.size = byte_offset;

  return res;
}

sax_str_t sax_str(const char *restrict value) {

  if (!value) {
    return SAXAMAPHONE_EMPTY_STRING;
  }

  return (sax_str_t){
      .size = strlen(value),
      .value = value,
  };
}

bool sax_str_equals(const sax_str_t lhs, const sax_str_t rhs) {

  if (lhs.size != rhs.size) {
    return false;
  }

  return strncmp(lhs.value, rhs.value, lhs.size) == 0;
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