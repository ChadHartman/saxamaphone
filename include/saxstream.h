#ifndef SAX_STREAM_H
#define SAX_STREAM_H

#include <stdbool.h>
#include <stddef.h> // size_t
#include <stdio.h>  // FILE

typedef struct sax_stream_t sax_stream_t;

typedef struct sax_stream_config_t {
  void *(*alloc)(void *, void *, size_t);
  void *alloc_ctx;
  FILE *file;
} sax_stream_config_t;

sax_stream_t *sax_stream(const sax_stream_config_t *restrict config);

bool sax_stream_append(sax_stream_t *restrict stream, const char *restrict fmt, ...);

void sax_stream_free(sax_stream_t *restrict stream);

#endif