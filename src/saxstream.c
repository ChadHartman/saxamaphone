#include <stdarg.h>

#include "saxalloc.h"
#include "saxstream.h"

#ifdef SAXAMAPHONE_DEBUG
#include <assert.h>
#include <string.h> // strrchr

#define SAXAMAPHONE_LOG(...)                      \
  printf("\x1b[36m"                               \
         "[SAXAMAPHONE] %s:%d "                   \
         "\x1b[0m",                               \
         (strrchr(__FILE__, '/') + 1), __LINE__); \
  printf(__VA_ARGS__);                            \
  printf("\n")
#define SAXAMAPHONE_ASSERT(...) assert(__VA_ARGS__)
#else
#define SAXAMAPHONE_LOG(...) ((void)0)
#define SAXAMAPHONE_ASSERT(...) ((void)0)
#endif

struct sax_stream_t {
  sax_stream_config_t config;
  char *buf;
  size_t buf_size;
  size_t buf_offset;
};

static const sax_stream_config_t sax_stream_default_config = {
    .alloc = sax_system_alloc,
};

static bool sax_stream_append_string(
    sax_stream_t *restrict stream,
    const char *restrict fmt,
    va_list args) {

  const int size = (int)stream->buf_size;
  const int res = vsnprintf(
      stream->buf + stream->buf_offset,
      stream->buf_size - stream->buf_offset,
      fmt, args);

  SAXAMAPHONE_ASSERT(res >= 0);

  if (res < size) {
    stream->buf_offset += (size_t)res;
    return true;
  }

  stream->buf_size = stream->buf_size == 0 ? 4096 : stream->buf_size * 2;
  stream->buf = stream->config.alloc(stream->config.alloc_ctx, stream->buf, stream->buf_size);
  if (stream->buf == NULL) {
    return false;
  }

  return sax_stream_append_string(stream, fmt, args);
}

sax_stream_t *sax_stream(const sax_stream_config_t *restrict config) {

  config = config == NULL ? &sax_stream_default_config : config;
  void *(*alloc)(void *, void *, size_t) = config->alloc == NULL ? sax_system_alloc : config->alloc;
  
  sax_stream_t *restrict stream = alloc(config->alloc_ctx, NULL, sizeof(sax_stream_t));
  if (stream == NULL) {
    SAXAMAPHONE_LOG("Allocator returned NULL in creating sax_stream_t sized %zu", sizeof(sax_stream_t));
    return NULL;
  }

  *stream = (sax_stream_t){
      .config = *config,
  };
  stream->config.alloc = alloc;

  return stream;
}

bool sax_stream_append(sax_stream_t *restrict stream, const char *restrict fmt, ...) {

  if (stream == NULL || fmt == NULL) {
    return false;
  }

  bool res = false;
  va_list args;
  va_start(args, fmt);

  if (stream->config.file) {
    res = vfprintf(stream->config.file, fmt, args) > 0;
  } else {
    res = sax_stream_append_string(stream, fmt, args);
  }

  va_end(args);
  return res;
}

const char *sax_stream_str(const sax_stream_t *restrict stream) {
  return stream == NULL ? NULL : stream->buf;
}

void sax_stream_free(sax_stream_t *restrict stream) {

  if (stream == NULL) {
    return;
  }

  stream->config.alloc(stream->config.alloc_ctx, stream->buf, 0);
  stream->config.alloc(stream->config.alloc_ctx, stream, 0);
}