#include <saxmapper.h>
#include <test.h>

typedef struct alpha_t {
  int8_t beta;
} alpha_t;

static const sax_field_t alpha_schema[] = {
    {.name = "beta", .type = SAX_TYPE_INT8, .offset = offsetof(alpha_t, beta)},
    {0},
};

static const sax_field_t doc_schema[] = {
    {.name = "alpha", .schema = alpha_schema},
    {0},
};

TEST(map_encode) {

  alpha_t alpha = {
      .beta = 42,
  };

  sax_map_opts_t opts = {
      .alloc = arena_custom_alloc,
      .alloc_ctx = arena,
  };

  char *xml = NULL;
  ASSERT(sax_encode(doc_schema, &alpha, &opts, &xml));
  arena_custom_alloc(arena, xml, 0);
}