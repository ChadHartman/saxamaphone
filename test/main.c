#include <stdio.h>  // printf
#include <stdlib.h> // EXIT_SUCCESS

#include "arena.h"
#include "test.h" // COLOR_MAGENTA
#include "testlist.h"

static void print_test(const char *restrict header) {
  const size_t len = strlen(header);
  putchar('+');
  for (size_t i = 0; i < len + 4; ++i) {
    putchar('-');
  }
  printf("+\n| " COLOR_MAGENTA " %s " COLOR_RESET " |\n+", header);
  for (size_t i = 0; i < len + 4; ++i) {
    putchar('-');
  }
  printf("+\n");
}

void *test_null_alloc(void *ctx, void *ptr, size_t size) {
  (void)ctx;
  (void)ptr;
  (void)size;
  return NULL;
}

// === main === //

int main(int argc, char **args) {

  const size_t count = sizeof(tests) / sizeof(test_t);

  // Run all tests
  if (argc == 1) {

    arena_t *arena = arena_create();

    for (size_t i = 0; i < count; ++i) {
      print_test(tests[i].name);
      tests[i].func(arena);
      arena_reset(arena);
    }

    TEST_LOG("Arena managed %zu bytes", arena_size(arena));
    arena_free(arena);
    return EXIT_SUCCESS;
  }

  // Help
  if (strcmp("-h", args[1]) == 0) {
    printf("Usage: %s [-h] [-l] [<test-name>]\n", args[0]);
    return EXIT_SUCCESS;
  }

  // List tests
  if (strcmp("-l", args[1]) == 0) {

    for (size_t i = 0; i < count; ++i) {
      printf("%s\n", tests[i].name);
    }
    return EXIT_SUCCESS;
  }

  // Run specific test
  for (size_t i = 0; i < count; ++i) {
    if (strcmp(tests[i].name, args[1]) == 0) {
      print_test(tests[i].name);
      arena_t *restrict arena = arena_create();
      tests[i].func(arena);
      TEST_LOG("Arena managed %zu bytes", arena_size(arena));
      arena_free(arena);
      return EXIT_SUCCESS;
    }
  }

  printf("Unknown test \"%s\"\n", args[1]);
  return EXIT_FAILURE;
}
