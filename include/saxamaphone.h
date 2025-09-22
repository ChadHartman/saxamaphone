#ifndef SAXAMAPHONE
#define SAXAMAPHONE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

typedef void *(*sax_alloc_t)(void *ctx, void *ptr, size_t size);

/// @brief Saxamaphone parser configuration
typedef struct sax_config_t {

  /// @brief The file path to stream XML from
  const char *path;

  /// @brief The XML string to parse; when path is provided; this field is ignored
  const char *string;

  /// @brief Leave content leading and trailing spaces
  bool untrimmed_content;

  /// @brief Provide this buffer if allocation is undesired; 4096 is recommended.
  uint8_t *buf;

  /// @brief Size of buf
  size_t buf_size;

  /// @brief Custom allocator to use
  sax_alloc_t alloc;

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

void *sax_default_alloc(void *ctx, void *ptr, size_t size);

#endif