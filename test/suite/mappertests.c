#include <saxmapper.h>
#include <test.h>

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
}

void test_sax_decode_double(arena_t *restrict arena) { (void)arena; }

void test_sax_decode_float(arena_t *restrict arena) { (void)arena; }

void test_sax_decode_int8(arena_t *restrict arena) { (void)arena; }

void test_sax_decode_int16(arena_t *restrict arena) { (void)arena; }

void test_sax_decode_int32(arena_t *restrict arena) { (void)arena; }

void test_sax_decode_int64(arena_t *restrict arena) { (void)arena; }

void test_sax_decode_size(arena_t *restrict arena) { (void)arena; }

void test_sax_decode_string(arena_t *restrict arena) { (void)arena; }

void test_sax_decode_uint8(arena_t *restrict arena) { (void)arena; }

void test_sax_decode_uint16(arena_t *restrict arena) { (void)arena; }

void test_sax_decode_uint32(arena_t *restrict arena) { (void)arena; }

void test_sax_decode_uint64(arena_t *restrict arena) { (void)arena; }

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