#include <saxmapper.h>
#include <test.h>

// static void test_map_bool_attr(arena_t *restrict arena) {

//   sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
//       .alloc = arena_custom_alloc,
//       .alloc_ctx = arena,
//       .xml = "<bool value=\"true\"/>",
//   });

//   sax_mapper_t *restrict mapper = sax_mapper(parser);
//   bool result = false;
//   ASSERT(sax_map_bool(mapper, "value", SAX_FIELD_ATTR, true, &result));
//   ASSERT(result);

//   sax_parser_free(parser);
// }

// static void test_map_bool_content(arena_t *restrict arena) {

//   sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
//       .alloc = arena_custom_alloc,
//       .alloc_ctx = arena,
//       .xml = "<bool>true</bool>",
//   });

//   sax_mapper_t *restrict mapper = sax_mapper(parser);
//   bool result = false;
//   ASSERT(sax_map_bool(mapper, "bool", SAX_FIELD_CONTENT, true, &result));
//   ASSERT(result);

//   sax_mapper_free(mapper);
//   sax_parser_free(parser);
// }

TEST(mapper) {
  // test_map_bool_attr(arena);
  // test_map_bool_content(arena);
  (void)arena;
}