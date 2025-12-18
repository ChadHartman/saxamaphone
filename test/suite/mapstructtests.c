#include <assert.h>
#include <saxmapper.h>
#include <test.h>

typedef struct view_t {

  char *name;
  bool hidden;

  float x, y, w, h;

  struct view_t *children;
  size_t child_count;
  size_t child_cap;

} view_t;

static void view_dtor(arena_t *restrict arena, view_t *restrict view) {

  for (size_t i = 0; i < view->child_count; ++i) {
    view_dtor(arena, &view->children[i]);
  }

  arena_custom_alloc(arena, view->children, 0);
  arena_custom_alloc(arena, view->name, 0);
}

static void *view_append(void *alloc_ctx, void *(*alloc)(void *, void *, size_t), void *value) {
  view_t *parent = value;
  if (parent->child_cap == parent->child_count) {
    parent->child_cap = parent->child_cap == 0 ? 8 : parent->child_cap * 2;
    parent->children = alloc(alloc_ctx, parent->children, sizeof(view_t) * parent->child_cap);
    assert(parent->children);
  }

  return &parent->children[parent->child_count++];
}

extern const sax_field_t view_schema[];

const sax_field_t view_schema[] = {
    {.name = "name", .type = SAX_TYPE_STRING, .offset = offsetof(view_t, name)},
    {.name = "hidden", .type = SAX_TYPE_BOOL, .offset = offsetof(view_t, hidden)},
    {.name = "x", .type = SAX_TYPE_FLOAT, .offset = offsetof(view_t, x)},
    {.name = "y", .type = SAX_TYPE_FLOAT, .offset = offsetof(view_t, y)},
    {.name = "w", .type = SAX_TYPE_FLOAT, .offset = offsetof(view_t, w)},
    {.name = "h", .type = SAX_TYPE_FLOAT, .offset = offsetof(view_t, h)},
    {.name = "view", .type = SAX_TYPE_ARRAY, .sub_schema = view_schema, .arr_append = view_append},
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
  ASSERT(sax_decode(parser, doc_schema, &view, NULL, &errmsg));
  if (errmsg) {
    TEST_LOG("%s", errmsg);
  }
  ASSERT_NULL(errmsg);
  ASSERT_STR_EQ("root", view.name);
  ASSERT_FALSE(view.hidden);
  ASSERT_EQ(640, view.w);
  ASSERT_EQ(480, view.h);
  ASSERT_EQ(2, view.child_count);

  view_dtor(arena, &view);

  sax_parser_free(parser);
}