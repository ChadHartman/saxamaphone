#include <saxmapper.h>
#include <test.h>

typedef struct view_t {

  char *name;
  bool visible;

  float x, y, w, h;

  struct view_t *children;
  size_t child_count;

} view_t;

bool sax_map_view(
    sax_mapper_t *restrict mapper,
    const char *restrict name,
    sax_map_field_t field,
    view_t *restrict view) {

  return sax_map_string(mapper, "name", SAX_FIELD_ATTR, &view->name) &&
         sax_map_bool(mapper, "visible", SAX_FIELD_ATTR, &view->visible) &&
         sax_map_float(mapper, "x", SAX_FIELD_ATTR, &view->x) &&
         sax_map_float(mapper, "y", SAX_FIELD_ATTR, &view->y) &&
         sax_map_float(mapper, "w", SAX_FIELD_ATTR, &view->w) &&
         sax_map_float(mapper, "h", SAX_FIELD_ATTR, &view->h);
}

TEST(map_struct) {
  (void)arena;
}