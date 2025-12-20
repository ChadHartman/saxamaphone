#include <test.h>

typedef struct delta_t {
  uint8_t second;
  char *epsilon;
  struct delta_t *next;
} delta_t;

typedef struct beta_t {
  bool first;
  uint8_t second;
  char *third;
} beta_t;

typedef struct alpha_t {

  bool first;
  uint8_t second;
  char *third;

  beta_t beta;

  delta_t *delta;

} alpha_t;

static void delta_free(arena_t *restrict arena, delta_t *restrict delta) {

  if (delta == NULL) {
    return;
  }

  delta_free(arena, delta->next);
  arena_custom_alloc(arena, delta->epsilon, 0);
  arena_custom_alloc(arena, delta, 0);
}

static void beta_dtor(arena_t *restrict arena, beta_t *restrict beta) {
  arena_custom_alloc(arena, beta->third, 0);
}

static void alpha_dtor(arena_t *restrict arena, alpha_t *restrict alpha) {
  arena_custom_alloc(arena, alpha->third, 0);
  beta_dtor(arena, &alpha->beta);
  delta_free(arena, alpha->delta);
}

TEST(map_alpha) {
  alpha_t alpha = {0};
  alpha_dtor(arena, &alpha);
}