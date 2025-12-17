// MIT License
// Copyright (c) 2025 Chad Hartman
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef SAXAMAPHONE_H
#define SAXAMAPHONE_H

#define SAXAMAPHONE_VERSION_MAJOR 1
#define SAXAMAPHONE_VERSION_MINOR 1
#define SAXAMAPHONE_VERSION_PATCH 0

#define SAXAMAPHONE_VERSION_STRING "1.1.0"

#if (defined(_WIN32) || defined(__CYGWIN__)) && !defined(SAXAMAPHONE_TEST)
#if defined(SAXAMAPHONE_EXPORTS)
#define SAXAMAPHONE_API __declspec(dllexport)

#else
#define SAXAMAPHONE_API __declspec(dllimport)

#endif
#elif defined(__GNUC__)
#define SAXAMAPHONE_API __attribute__((visibility("default")))

#else
#define SAXAMAPHONE_API

#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint8_t

/// @brief Possible Saxamaphone event types
typedef enum {

  /// @brief e.g. `<tag name="value">`
  SAX_EVENT_START_TAG = 1,

  /// @brief For content within tags and CDATA
  SAX_EVENT_CONTENT,

  /// @brief e.g. `</tag>` and `<tag/>` (directly after SAX_EVENT_START_TAG)
  SAX_EVENT_END_TAG,

  /// @brief When the end of the document is reached
  SAX_EVENT_END_DOCUMENT,

  /// @brief e.g. `<?xml version="1.0"?>` or `<?custom name="value"?>`
  SAX_EVENT_PROCESSING_INSTRUCTION,

  /// @brief When unexpected input is reached
  SAX_EVENT_ERROR,
} sax_event_t;

/// @brief Saxamaphone parser instance
typedef struct sax_parser_t sax_parser_t;

/// @brief Saxamaphone parser configuration
typedef struct sax_config_t {

  /// @brief The file path to stream XML from
  const char *path;

  /// @brief The XML string to parse; when path is provided; this field is ignored
  const char *xml;

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

  /// @brief First argument passed into alloc
  void *alloc_ctx;

  /// @brief When non-zero, and buf is NOT provided; this is the initial number
  ///   of bytes allocated for tag & content parsing. 1024 by default
  size_t arena_size;

  /// @brief When non-zero, path is provided, and buf is NOT provided; this is
  ///   the initial number of bytes allocated for streaming in a file. 4096 by
  ///   default
  size_t file_buf_size;

} sax_config_t;

/// @brief Saxamaphone XML Attribute
typedef struct sax_attr_t {

  /// @brief non-NULL Attribute Name
  char *name;

  /// @brief non-NULL Attribute Value
  char *value;

  /// @brief NULLable pointer to next attribute pair
  struct sax_attr_t *next;

} sax_attr_t;

/// @brief Create a Saxamaphone parser instance
/// @param config (non-NULL) configuration to use
/// @return Parser instance; or NULL due to missing configuration or error
SAXAMAPHONE_API sax_parser_t *sax_parser(const sax_config_t *restrict config);

/// @brief Advance document iteration to the next event
/// @param  parser instance
/// @return next event
SAXAMAPHONE_API sax_event_t sax_next(sax_parser_t *restrict parser);

/// @brief Retrieve an error message for a @see SAX_EVENT_ERROR
/// @param parser instance
/// @return Human-readable error string or NULL if not in an ERROR state
SAXAMAPHONE_API const char *sax_error(const sax_parser_t *restrict parser);

/// @brief Retrieve the tag value for events @see SAX_EVENT_START_TAG,
///   @see SAX_EVENT_END_TAG, or @see SAX_EVENT_PROCESSING_INSTRUCTION
/// @param parser instance
/// @return Tag name or possibly NULL for non-specified events
SAXAMAPHONE_API const char *sax_tag(const sax_parser_t *restrict parser);

/// @brief Retrieve the content for the @see SAX_EVENT_CONTENT event
/// @param parser instance
/// @return Content or NULL for non @see SAX_EVENT_CONTENT events
SAXAMAPHONE_API const char *sax_content(const sax_parser_t *restrict parser);

/// @brief Retrieve the XML attributes for the @see SAX_EVENT_START_TAG or
///   @see SAX_EVENT_PROCESSING_INSTRUCTION events
/// @param parser instance
/// @return NULLable attribute linked list
SAXAMAPHONE_API const sax_attr_t *sax_attrs(const sax_parser_t *restrict parser);

/// @brief Retrieve the XML attribute value associated with the provided name
/// @param parser instance
/// @param name to lookup
/// @return paired value or NULL if not found
SAXAMAPHONE_API const char *sax_attr(const sax_parser_t *restrict parser, const char *restrict name);

/// @brief Use to close out and free any resources
/// @param parser instance
SAXAMAPHONE_API void sax_parser_free(sax_parser_t *restrict parser);

#endif // SAXAMAPHONE_H