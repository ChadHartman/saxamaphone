#include <saxmapper.h>
#include <test.h>

#define TEST_SAX_DECODE_INT(type_pfx, overflow)              \
  void test_sax_decode_##type_pfx(arena_t *restrict arena) { \
    type_pfx##_t res;                                        \
    sax_decode_ctx_t ctx;                                    \
    ctx = sax_decode_ctx_create(arena, "");                  \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
    ctx = sax_decode_ctx_create(arena, "0");                 \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(0, res);                                       \
    ctx = sax_decode_ctx_create(arena, "-0");                \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(0, res);                                       \
    ctx = sax_decode_ctx_create(arena, "1");                 \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(1, res);                                       \
    ctx = sax_decode_ctx_create(arena, "-1");                \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(-1, res);                                      \
    ctx = sax_decode_ctx_create(arena, overflow);            \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
    ctx = sax_decode_ctx_create(arena, "foo");               \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
  }

#define TEST_SAX_DECODE_UINT(type_pfx, overflow)             \
  void test_sax_decode_##type_pfx(arena_t *restrict arena) { \
    type_pfx##_t res;                                        \
    sax_decode_ctx_t ctx;                                    \
    ctx = sax_decode_ctx_create(arena, "");                  \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
    ctx = sax_decode_ctx_create(arena, "0");                 \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(0, res);                                       \
    ctx = sax_decode_ctx_create(arena, "1");                 \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(1, res);                                       \
    ctx = sax_decode_ctx_create(arena, overflow);            \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
    ctx = sax_decode_ctx_create(arena, "foo");               \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
  }

#define TEST_SAX_DECODE_UINT64(type_pfx)                     \
  void test_sax_decode_##type_pfx(arena_t *restrict arena) { \
    type_pfx##_t res;                                        \
    sax_decode_ctx_t ctx;                                    \
    ctx = sax_decode_ctx_create(arena, "");                  \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
    ctx = sax_decode_ctx_create(arena, "0");                 \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(0, res);                                       \
    ctx = sax_decode_ctx_create(arena, "1");                 \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(1, res);                                       \
    ctx = sax_decode_ctx_create(arena, "foo");               \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
  }

static void *null_allocator(void *ctx, void *ptr, size_t size) {
  (void)ctx;
  (void)ptr;
  (void)size;
  return NULL;
}

static sax_decode_ctx_t sax_decode_ctx_create(
    arena_t *restrict arena,
    const char *restrict encoded) {
  return (sax_decode_ctx_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .encoded = encoded,
  };
}

void test_sax_decode_bool(arena_t *restrict arena) {

  bool res = false;
  sax_decode_ctx_t ctx;

  ctx = sax_decode_ctx_create(arena, "true");
  ASSERT(sax_decode_bool(&ctx, &res));
  ASSERT(res);

  ctx = sax_decode_ctx_create(arena, "True");
  ASSERT(sax_decode_bool(&ctx, &res));
  ASSERT(res);

  ctx = sax_decode_ctx_create(arena, "tRue");
  ASSERT(sax_decode_bool(&ctx, &res));
  ASSERT(res);

  ctx = sax_decode_ctx_create(arena, "truE");
  ASSERT(sax_decode_bool(&ctx, &res));
  ASSERT(res);

  ctx = sax_decode_ctx_create(arena, "TRUE");
  ASSERT(sax_decode_bool(&ctx, &res));
  ASSERT(res);

  ctx = sax_decode_ctx_create(arena, "trueish");
  ASSERT(sax_decode_bool(&ctx, &res));
  ASSERT_FALSE(res);

  ctx = sax_decode_ctx_create(arena, "");
  ASSERT(sax_decode_bool(&ctx, &res));
  ASSERT_FALSE(res);
}

void test_sax_decode_double(arena_t *restrict arena) {

  double res = false;
  sax_decode_ctx_t ctx;

  ctx = sax_decode_ctx_create(arena, "");
  ASSERT_FALSE(sax_decode_double(&ctx, &res));

  ctx = sax_decode_ctx_create(arena, "0");
  ASSERT(sax_decode_double(&ctx, &res));
  ASSERT_EQ(0, res);

  ctx = sax_decode_ctx_create(arena, "-0");
  ASSERT(sax_decode_double(&ctx, &res));
  ASSERT_EQ(0, res);

  ctx = sax_decode_ctx_create(arena, "1.5");
  ASSERT(sax_decode_double(&ctx, &res));
  ASSERT_EQ(15, res * 10.0);

  ctx = sax_decode_ctx_create(arena, "-1.5");
  ASSERT(sax_decode_double(&ctx, &res));
  ASSERT_EQ(-15, res * 10.0);

  ctx = sax_decode_ctx_create(arena, "50%%");
  ASSERT(sax_decode_double(&ctx, &res));
  ASSERT_EQ(50, res * 100.0);

  ctx = sax_decode_ctx_create(arena, "-50%%");
  ASSERT(sax_decode_double(&ctx, &res));
  ASSERT_EQ(-50, res * 100.0);

  ctx = sax_decode_ctx_create(arena, "%%");
  ASSERT_FALSE(sax_decode_double(&ctx, &res));

  ctx = sax_decode_ctx_create(arena, "foo");
  ASSERT_FALSE(sax_decode_double(&ctx, &res));
}

void test_sax_decode_float(arena_t *restrict arena) {

  float res = false;
  sax_decode_ctx_t ctx;

  ctx = sax_decode_ctx_create(arena, "");
  ASSERT_FALSE(sax_decode_float(&ctx, &res));

  ctx = sax_decode_ctx_create(arena, "0");
  ASSERT(sax_decode_float(&ctx, &res));
  ASSERT_EQ(0, res);

  ctx = sax_decode_ctx_create(arena, "-0");
  ASSERT(sax_decode_float(&ctx, &res));
  ASSERT_EQ(0, res);

  ctx = sax_decode_ctx_create(arena, "1.5");
  ASSERT(sax_decode_float(&ctx, &res));
  ASSERT_EQ(15, res * 10.0f);

  ctx = sax_decode_ctx_create(arena, "-1.5");
  ASSERT(sax_decode_float(&ctx, &res));
  ASSERT_EQ(-15, res * 10.0f);

  ctx = sax_decode_ctx_create(arena, "50%%");
  ASSERT(sax_decode_float(&ctx, &res));
  ASSERT_EQ(50, res * 100.0f);

  ctx = sax_decode_ctx_create(arena, "-50%%");
  ASSERT(sax_decode_float(&ctx, &res));
  ASSERT_EQ(-50, res * 100.0f);

  ctx = sax_decode_ctx_create(arena, "%%");
  ASSERT_FALSE(sax_decode_float(&ctx, &res));

  ctx = sax_decode_ctx_create(arena, "foo");
  ASSERT_FALSE(sax_decode_float(&ctx, &res));
}

TEST_SAX_DECODE_INT(int8, "128")
TEST_SAX_DECODE_INT(int16, "32768")
TEST_SAX_DECODE_INT(int32, "2147483648")

void test_sax_decode_int64(arena_t *restrict arena) {

  int64_t res;
  sax_decode_ctx_t ctx;

  ctx = sax_decode_ctx_create(arena, "");
  ASSERT_FALSE(sax_decode_int64(&ctx, &res));

  ctx = sax_decode_ctx_create(arena, "0");
  ASSERT(sax_decode_int64(&ctx, &res));
  ASSERT_EQ(0, res);

  ctx = sax_decode_ctx_create(arena, "-0");
  ASSERT(sax_decode_int64(&ctx, &res));
  ASSERT_EQ(0, res);

  ctx = sax_decode_ctx_create(arena, "1");
  ASSERT(sax_decode_int64(&ctx, &res));
  ASSERT_EQ(1, res);

  ctx = sax_decode_ctx_create(arena, "-1");
  ASSERT(sax_decode_int64(&ctx, &res));
  ASSERT_EQ(-1, res);

  ctx = sax_decode_ctx_create(arena, "foo");
  ASSERT_FALSE(sax_decode_int64(&ctx, &res));
}

TEST_SAX_DECODE_UINT64(size)

void test_sax_decode_string(arena_t *restrict arena) {

  char *res;
  sax_decode_ctx_t ctx;

  ctx = sax_decode_ctx_create(arena, "");
  ASSERT(sax_decode_string(&ctx, &res));
  ASSERT_STR_EQ("", res);
  arena_custom_alloc(arena, res, 0);

  ctx = sax_decode_ctx_create(arena, "foo");
  ASSERT(sax_decode_string(&ctx, &res));
  ASSERT_STR_EQ("foo", res);
  arena_custom_alloc(arena, res, 0);

  ctx = (sax_decode_ctx_t){
      .alloc = null_allocator,
      .encoded = "foo",
  };

  ASSERT_FALSE(sax_decode_string(&ctx, &res));
}

TEST_SAX_DECODE_UINT(uint8, "256")
TEST_SAX_DECODE_UINT(uint16, "65536")
TEST_SAX_DECODE_UINT(uint32, "4294967296")
TEST_SAX_DECODE_UINT64(uint64)

static bool decode_int32_inverted(const sax_decode_ctx_t *restrict ctx, void *value) {
  int conv = atoi(ctx->encoded);
  *(int32_t *)value = -(int32_t)(conv);
  return true;
}

static void test_sax_decode_custom(arena_t *restrict arena) {

  typedef struct foo_t {
    int32_t alpha;
    int32_t beta;
  } foo_t;

  foo_t foo = {0};

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = "<foo alpha=\"42\">52</foo>",
  });

  const sax_field_t foo_schema[] = {
      {.name = "alpha", .offset = offsetof(foo_t, alpha), .type = SAX_TYPE_INT32},
      {.name = SAX_CONTENT, .offset = offsetof(foo_t, beta), .type = SAX_TYPE_INT32},
      {0},
  };

  const sax_field_t doc_schema[] = {
      {.name = "foo", .schema = foo_schema},
      {0},
  };

  sax_map_opts_t opts = {0};
  opts.decoders[SAX_TYPE_INT32] = decode_int32_inverted;

  char *err = NULL;
  const bool decode_success = sax_decode(parser, doc_schema, &foo, &opts, &err);
  if (err) {
    fprintf(stderr, "%s\n", err);
  }
  ASSERT(decode_success);
  ASSERT_EQ(foo.alpha, -42);
  ASSERT_EQ(foo.beta, -52);

  sax_parser_free(parser);
}

static void test_sax_decode_null_parser(arena_t *restrict arena) {
  (void)arena;
  ASSERT_FALSE(sax_decode(NULL, NULL, NULL, NULL, NULL));
}

static void test_sax_decode_null_schema(arena_t *restrict arena) {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = "<foo/>",
  });

  ASSERT_FALSE(sax_decode(parser, NULL, NULL, NULL, NULL));

  char *err = NULL;
  ASSERT_FALSE(sax_decode(parser, NULL, NULL, NULL, &err));
  arena_custom_alloc(arena, err, 0);

  sax_parser_free(parser);
}

static void test_sax_decode_null_value(arena_t *restrict arena) {

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = "<foo/>",
  });

  const sax_field_t schema = {0};
  ASSERT_FALSE(sax_decode(parser, &schema, NULL, NULL, NULL));

  char *err = NULL;
  ASSERT_FALSE(sax_decode(parser, &schema, NULL, NULL, &err));
  arena_custom_alloc(arena, err, 0);

  sax_parser_free(parser);
}

TEST(mapper) {
  test_sax_decode_custom(arena);
  test_sax_decode_bool(arena);
  test_sax_decode_double(arena);
  test_sax_decode_float(arena);
  test_sax_decode_int8(arena);
  test_sax_decode_int16(arena);
  test_sax_decode_int32(arena);
  test_sax_decode_int64(arena);
  test_sax_decode_size(arena);
  test_sax_decode_string(arena);
  test_sax_decode_uint8(arena);
  test_sax_decode_uint16(arena);
  test_sax_decode_uint32(arena);
  test_sax_decode_uint64(arena);
  test_sax_decode_null_parser(arena);
  test_sax_decode_null_schema(arena);
  test_sax_decode_null_value(arena);
}