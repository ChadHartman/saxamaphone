#include <saxmapper.h>
#include <test.h>

void test_sax_decode_bool(arena_t *restrict arena) { (void)arena; }

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