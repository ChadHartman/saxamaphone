#include <assert.h>
#include <saxmapper.h>
#include <test.h>

typedef struct delta_t {
  uint8_t second;
  char *epsilon;
  struct delta_t *next;
} delta_t;

const sax_field_t epsilon_schema[] = {
    {.name = SAX_CONTENT, .type = SAX_TYPE_STRING},
    {0}};

const sax_field_t delta_schema[] = {
    {.name = "second", .offset = offsetof(delta_t, second), .type = SAX_TYPE_UINT8},
    {.name = "epsilon", .offset = offsetof(delta_t, epsilon), .schema = epsilon_schema},
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
    {.name = "beta", .offset = offsetof(alpha_t, beta), .schema = beta_schema},
    {.name = "delta", .getter = alpha_delta, .schema = delta_schema},
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

  const sax_field_t doc_schema[] = {
      {.name = "alpha", .schema = alpha_schema},
      {0}};

  char *err = NULL;
  const bool decode_success = sax_decode(parser, doc_schema, &alpha, NULL, &err);
  if (err != NULL) {
    fprintf(stderr, "ERROR: %s\n", err);
  }
  ASSERT(decode_success);
  ASSERT_NULL(err);

  ASSERT(alpha.first);
  ASSERT_EQ(42, alpha.second);
  ASSERT_STR_EQ("foo", alpha.third);

  ASSERT_FALSE(alpha.beta.first);
  ASSERT_EQ(52, alpha.beta.second);
  ASSERT_STR_EQ("bar", alpha.beta.third);

  const delta_t *restrict delta_first = alpha.delta;
  ASSERT_NON_NULL(delta_first)
  ASSERT_EQ(92, delta_first->second);
  ASSERT_NULL(delta_first->epsilon);

  const delta_t *restrict delta_second = delta_first->next;
  ASSERT_NON_NULL(delta_second)
  ASSERT_EQ(102, delta_second->second);
  ASSERT_NULL(delta_second->epsilon);

  const delta_t *restrict delta_third = delta_second->next;
  ASSERT_NON_NULL(delta_third)
  ASSERT_EQ(112, delta_third->second);
  ASSERT_STR_EQ("theta", delta_third->epsilon);

  alpha_dtor(arena, &alpha);

  sax_parser_free(parser);
}