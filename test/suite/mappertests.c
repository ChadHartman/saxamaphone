#include <saxmapper.h>
#include <test.h>

#define TEST_SAX_DECODE_INT(type_pfx, overflow)              \
  void test_sax_decode_##type_pfx(arena_t *restrict arena) { \
    type_pfx##_t res;                                        \
    sax_decode_ctx_t ctx;                                    \
    ctx = sax_decode_ctx_create(arena, "", NULL);            \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
    ctx = sax_decode_ctx_create(arena, "0", NULL);           \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(0, res);                                       \
    ctx = sax_decode_ctx_create(arena, "-0", NULL);          \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(0, res);                                       \
    ctx = sax_decode_ctx_create(arena, "1", NULL);           \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(1, res);                                       \
    ctx = sax_decode_ctx_create(arena, "-1", NULL);          \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(-1, res);                                      \
    ctx = sax_decode_ctx_create(arena, overflow, NULL);      \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
    ctx = sax_decode_ctx_create(arena, "foo", NULL);         \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
  }

#define TEST_SAX_DECODE_UINT(type_pfx, overflow)             \
  void test_sax_decode_##type_pfx(arena_t *restrict arena) { \
    type_pfx##_t res;                                        \
    sax_decode_ctx_t ctx;                                    \
    ctx = sax_decode_ctx_create(arena, "", NULL);            \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
    ctx = sax_decode_ctx_create(arena, "0", NULL);           \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(0, res);                                       \
    ctx = sax_decode_ctx_create(arena, "1", NULL);           \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(1, res);                                       \
    ctx = sax_decode_ctx_create(arena, overflow, NULL);      \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
    ctx = sax_decode_ctx_create(arena, "foo", NULL);         \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
  }

#define TEST_SAX_DECODE_UINT64(type_pfx)                     \
  void test_sax_decode_##type_pfx(arena_t *restrict arena) { \
    type_pfx##_t res;                                        \
    sax_decode_ctx_t ctx;                                    \
    ctx = sax_decode_ctx_create(arena, "", NULL);            \
    ASSERT_FALSE(sax_decode_##type_pfx(&ctx, &res));         \
    ctx = sax_decode_ctx_create(arena, "0", NULL);           \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(0, res);                                       \
    ctx = sax_decode_ctx_create(arena, "1", NULL);           \
    ASSERT(sax_decode_##type_pfx(&ctx, &res));               \
    ASSERT_EQ(1, res);                                       \
    ctx = sax_decode_ctx_create(arena, "foo", NULL);         \
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
    const char *restrict encoded,
    const sax_field_t *restrict field) {
  return (sax_decode_ctx_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .encoded = encoded,
      .field = field,
  };
}

void test_sax_decode_bool(arena_t *restrict arena) {

  bool res = false;
  sax_decode_ctx_t ctx;

  ctx = sax_decode_ctx_create(arena, "true", NULL);
  ASSERT(sax_decode_bool(&ctx, &res));
  ASSERT(res);

  ctx = sax_decode_ctx_create(arena, "True", NULL);
  ASSERT(sax_decode_bool(&ctx, &res));
  ASSERT(res);

  ctx = sax_decode_ctx_create(arena, "tRue", NULL);
  ASSERT(sax_decode_bool(&ctx, &res));
  ASSERT(res);

  ctx = sax_decode_ctx_create(arena, "truE", NULL);
  ASSERT(sax_decode_bool(&ctx, &res));
  ASSERT(res);

  ctx = sax_decode_ctx_create(arena, "TRUE", NULL);
  ASSERT(sax_decode_bool(&ctx, &res));
  ASSERT(res);

  ctx = sax_decode_ctx_create(arena, "trueish", NULL);
  ASSERT(sax_decode_bool(&ctx, &res));
  ASSERT_FALSE(res);

  ctx = sax_decode_ctx_create(arena, "", NULL);
  ASSERT(sax_decode_bool(&ctx, &res));
  ASSERT_FALSE(res);
}

void test_sax_decode_double(arena_t *restrict arena) {

  double res = false;
  sax_decode_ctx_t ctx;

  ctx = sax_decode_ctx_create(arena, "", NULL);
  ASSERT_FALSE(sax_decode_double(&ctx, &res));

  ctx = sax_decode_ctx_create(arena, "0", NULL);
  ASSERT(sax_decode_double(&ctx, &res));
  ASSERT_EQ(0, res);

  ctx = sax_decode_ctx_create(arena, "-0", NULL);
  ASSERT(sax_decode_double(&ctx, &res));
  ASSERT_EQ(0, res);

  ctx = sax_decode_ctx_create(arena, "1.5", NULL);
  ASSERT(sax_decode_double(&ctx, &res));
  ASSERT_EQ(15, res * 10.0);

  ctx = sax_decode_ctx_create(arena, "-1.5", NULL);
  ASSERT(sax_decode_double(&ctx, &res));
  ASSERT_EQ(-15, res * 10.0);

  ctx = sax_decode_ctx_create(arena, "50%%", NULL);
  ASSERT(sax_decode_double(&ctx, &res));
  ASSERT_EQ(50, res * 100.0);

  ctx = sax_decode_ctx_create(arena, "-50%%", NULL);
  ASSERT(sax_decode_double(&ctx, &res));
  ASSERT_EQ(-50, res * 100.0);

  ctx = sax_decode_ctx_create(arena, "%%", NULL);
  ASSERT_FALSE(sax_decode_double(&ctx, &res));

  ctx = sax_decode_ctx_create(arena, "foo", NULL);
  ASSERT_FALSE(sax_decode_double(&ctx, &res));
}

void test_sax_decode_float(arena_t *restrict arena) {

  float res = false;
  sax_decode_ctx_t ctx;

  ctx = sax_decode_ctx_create(arena, "", NULL);
  ASSERT_FALSE(sax_decode_float(&ctx, &res));

  ctx = sax_decode_ctx_create(arena, "0", NULL);
  ASSERT(sax_decode_float(&ctx, &res));
  ASSERT_EQ(0, res);

  ctx = sax_decode_ctx_create(arena, "-0", NULL);
  ASSERT(sax_decode_float(&ctx, &res));
  ASSERT_EQ(0, res);

  ctx = sax_decode_ctx_create(arena, "1.5", NULL);
  ASSERT(sax_decode_float(&ctx, &res));
  ASSERT_EQ(15, res * 10.0f);

  ctx = sax_decode_ctx_create(arena, "-1.5", NULL);
  ASSERT(sax_decode_float(&ctx, &res));
  ASSERT_EQ(-15, res * 10.0f);

  ctx = sax_decode_ctx_create(arena, "50%%", NULL);
  ASSERT(sax_decode_float(&ctx, &res));
  ASSERT_EQ(50, res * 100.0f);

  ctx = sax_decode_ctx_create(arena, "-50%%", NULL);
  ASSERT(sax_decode_float(&ctx, &res));
  ASSERT_EQ(-50, res * 100.0f);

  ctx = sax_decode_ctx_create(arena, "%%", NULL);
  ASSERT_FALSE(sax_decode_float(&ctx, &res));

  ctx = sax_decode_ctx_create(arena, "foo", NULL);
  ASSERT_FALSE(sax_decode_float(&ctx, &res));
}

TEST_SAX_DECODE_INT(int8, "128")
TEST_SAX_DECODE_INT(int16, "32768")
TEST_SAX_DECODE_INT(int32, "2147483648")

void test_sax_decode_int64(arena_t *restrict arena) {

  int64_t res;
  sax_decode_ctx_t ctx;

  ctx = sax_decode_ctx_create(arena, "", NULL);
  ASSERT_FALSE(sax_decode_int64(&ctx, &res));

  ctx = sax_decode_ctx_create(arena, "0", NULL);
  ASSERT(sax_decode_int64(&ctx, &res));
  ASSERT_EQ(0, res);

  ctx = sax_decode_ctx_create(arena, "-0", NULL);
  ASSERT(sax_decode_int64(&ctx, &res));
  ASSERT_EQ(0, res);

  ctx = sax_decode_ctx_create(arena, "1", NULL);
  ASSERT(sax_decode_int64(&ctx, &res));
  ASSERT_EQ(1, res);

  ctx = sax_decode_ctx_create(arena, "-1", NULL);
  ASSERT(sax_decode_int64(&ctx, &res));
  ASSERT_EQ(-1, res);

  ctx = sax_decode_ctx_create(arena, "foo", NULL);
  ASSERT_FALSE(sax_decode_int64(&ctx, &res));
}

TEST_SAX_DECODE_UINT64(size)

void test_sax_decode_string(arena_t *restrict arena) {

  char *res;
  sax_decode_ctx_t ctx;

  ctx = sax_decode_ctx_create(arena, "", NULL);
  ASSERT(sax_decode_string(&ctx, &res));
  ASSERT_STR_EQ("", res);
  arena_custom_alloc(arena, res, 0);

  ctx = sax_decode_ctx_create(arena, "foo", NULL);
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

TEST(mapper) {
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
}