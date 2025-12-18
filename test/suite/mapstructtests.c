#include <saxmapper.h>
#include <test.h>

typedef struct view_t {

  char *name;
  bool visible;

  float x, y, w, h;

  struct view_t *children;

} view_t;

static void view_dtor(arena_t *restrict arena, view_t *restrict view) {
  arena_custom_alloc(arena, view->name, 0);
}

extern const sax_field_t view_schema[];

const sax_field_t view_schema[] = {
    {.name = "name", .type = SAX_TYPE_STRING, .offset = offsetof(view_t, name)},
    {.name = "visible", .type = SAX_TYPE_BOOL, .offset = offsetof(view_t, visible)},
    {.name = "x", .type = SAX_TYPE_FLOAT, .offset = offsetof(view_t, x)},
    {.name = "y", .type = SAX_TYPE_FLOAT, .offset = offsetof(view_t, y)},
    {.name = "w", .type = SAX_TYPE_FLOAT, .offset = offsetof(view_t, w)},
    {.name = "h", .type = SAX_TYPE_FLOAT, .offset = offsetof(view_t, h)},
    {.name = "children", .type = SAX_TYPE_ARRAY, .offset = offsetof(view_t, children), .sub_schema = view_schema},
    {0}};

static const sax_field_t doc_schema[] = {
    {.name = "view", .type = SAX_TYPE_STRUCT, .sub_schema = view_schema},
    {0}};

TEST(map_struct) {

  view_t view = {0};

  sax_parser_t *restrict parser = sax_parser(&(sax_config_t){
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
      .path = "../test/files/view.xml",
  });

  char *errmsg = NULL;
  ASSERT(sax_deserialize(parser, doc_schema, &view, &errmsg));
  if (errmsg) {
    TEST_LOG("%s", errmsg);
  }
  ASSERT_NULL(errmsg);
  ASSERT_STR_EQ("root", view.name);
  ASSERT_EQ(640, view.w);
  ASSERT_EQ(480, view.h);

  view_dtor(arena, &view);

  sax_parser_free(parser);
}