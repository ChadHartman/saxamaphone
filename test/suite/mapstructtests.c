#include <saxmapper.h>
#include <test.h>

typedef struct view_t {

  char *name;
  bool visible;

  float x, y, w, h;

  struct view_t *children;

} view_t;

extern const sax_field_t view_schema[];

const sax_field_t view_schema[] = {
    {.name = "name", .type = SAX_TYPE_STRING, .offset = offsetof(view_t, name), .optional = true},
    {.name = "visible", .type = SAX_TYPE_BOOL, .offset = offsetof(view_t, visible), .optional = true},
    {.name = "x", .type = SAX_TYPE_FLOAT, .offset = offsetof(view_t, x), .optional = true},
    {.name = "y", .type = SAX_TYPE_FLOAT, .offset = offsetof(view_t, y), .optional = true},
    {.name = "w", .type = SAX_TYPE_FLOAT, .offset = offsetof(view_t, w), .optional = true},
    {.name = "h", .type = SAX_TYPE_FLOAT, .offset = offsetof(view_t, h), .optional = true},
    {.name = "children", .type = SAX_TYPE_ARRAY, .offset = offsetof(view_t, children), .optional = true, .sub_schema = view_schema},
    {0}};

// bool sax_map_view(
//     sax_mapper_t *restrict mapper,
//     const char *restrict name,
//     sax_map_field_t field,
//     view_t *restrict view) {

//   (void)name;
//   (void)field;

//   return sax_map_string(mapper, "name", SAX_FIELD_ATTR, &view->name) &&
//          sax_map_bool(mapper, "visible", SAX_FIELD_ATTR, &view->visible) &&
//          sax_map_float(mapper, "x", SAX_FIELD_ATTR, &view->x) &&
//          sax_map_float(mapper, "y", SAX_FIELD_ATTR, &view->y) &&
//          sax_map_float(mapper, "w", SAX_FIELD_ATTR, &view->w) &&
//          sax_map_float(mapper, "h", SAX_FIELD_ATTR, &view->h);
// }

TEST(map_struct) {

  view_t view = {0};

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .xml = "<view/>",
  });

  char *errmsg = NULL;
  ASSERT(sax_deserialize(parser, view_schema, &view, &errmsg));
  if (errmsg) {
    TEST_LOG("%s", errmsg);
  }
  ASSERT_NULL(errmsg);

  sax_parser_free(parser);
}