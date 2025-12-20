#include <assert.h>
#include <saxmapper.h>
#include <test.h>

typedef struct delta_t {
  uint8_t second;
  char *epsilon;
  struct delta_t *next;
} delta_t;

static const sax_field_t delta_schema[] = {
    {.name = "second", .offset = offsetof(delta_t, second), .type = SAX_TYPE_UINT8},
    {.name = "epsilon", .offset = offsetof(delta_t, epsilon), .type = SAX_TYPE_STRING},
    {0}};

typedef struct beta_t {
  bool first;
  uint8_t second;
  char *third;
} beta_t;

static const sax_field_t beta_schema[] = {
    {.name = "first", .offset = offsetof(beta_t, first), .type = SAX_TYPE_BOOL},
    {.name = "second", .offset = offsetof(beta_t, second), .type = SAX_TYPE_UINT8},
    {.name = "third", .offset = offsetof(beta_t, third), .type = SAX_TYPE_STRING},
    {0}};

typedef struct alpha_t {

  bool first;
  uint8_t second;
  char *third;

  beta_t beta;

  delta_t *delta;

} alpha_t;

static void *alpha_delta(void *ctx, void *(*alloc)(void *, void *, size_t), void *parent) {

  alpha_t *restrict alpha = parent;
  delta_t *restrict child = alloc(ctx, NULL, sizeof(delta_t));
  assert(child);
  memset(child, 0, sizeof(delta_t));
  if (alpha->delta == NULL) {
    alpha->delta = child;
  } else {
    for (delta_t *i = alpha->delta; i != NULL; i = i->next) {
      if (i->next == NULL) {
        i->next = child;
        break;
      }
    }
  }
  return child;
}

static const sax_field_t alpha_schema[] = {
    {.name = "first", .offset = offsetof(alpha_t, first), .type = SAX_TYPE_BOOL},
    {.name = "second", .offset = offsetof(alpha_t, second), .type = SAX_TYPE_UINT8},
    {.name = "third", .offset = offsetof(alpha_t, third), .type = SAX_TYPE_STRING},
    {.name = "beta", .offset = offsetof(alpha_t, beta), .sub_schema = beta_schema},
    {.name = "delta", .getter = alpha_delta, .sub_schema = delta_schema},
    {0}};

static void delta_free(arena_t *restrict arena, delta_t *restrict delta) {

  if (delta == NULL) {
    return;
  }

  delta_free(arena, delta->next);
  arena_custom_alloc(arena, delta->epsilon, 0);
  arena_custom_alloc(arena, delta, 0);
}

static void beta_dtor(arena_t *restrict arena, beta_t *restrict beta) {
  arena_custom_alloc(arena, beta->third, 0);
}

static void alpha_dtor(arena_t *restrict arena, alpha_t *restrict alpha) {
  arena_custom_alloc(arena, alpha->third, 0);
  beta_dtor(arena, &alpha->beta);
  delta_free(arena, alpha->delta);
}

TEST(map_alpha) {
  alpha_t alpha = {0};

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .path = "../test/files/alpha.xml",
  });

  char *err = NULL;
  ASSERT(sax_decode(parser, alpha_schema, &alpha, NULL, &err));
  ASSERT_NULL(err);

  sax_parser_free(parser);

  alpha_dtor(arena, &alpha);
}