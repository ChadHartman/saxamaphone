#ifndef TEST_H
#define TEST_H

#include <inttypes.h> // PRId64
#include <saxamaphone.h>
#include <stdbool.h>
#include <stdio.h>  // printf
#include <string.h> // strrchr

#include "arena.h"

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_RESET "\x1b[0m"

#define TEST(name) void test_##name(arena_t *restrict arena)

#if 1
#define TEST_LOG(...)                                         \
  printf("%s:%d - ", (strrchr(__FILE__, '/') + 1), __LINE__); \
  printf(__VA_ARGS__);                                        \
  printf("\n")
#else
#define TEST_LOG(...) (void)0
#endif

#define FAIL(...)                        \
  {                                      \
    printf("%s:%d: %s" COLOR_RESET "\n", \
           (strrchr(__FILE__, '/') + 1), \
           __LINE__,                     \
           COLOR_RED "FAILED");          \
    printf(COLOR_CYAN "  FAIL(");        \
    printf(__VA_ARGS__);                 \
    printf(")\n" COLOR_RESET);           \
    exit(EXIT_FAILURE);                  \
  }

#define ASSERT(computed)                                                      \
  {                                                                           \
    const bool passed = (computed);                                           \
    printf("%s:%d: %s" COLOR_RESET "\n",                                      \
           (strrchr(__FILE__, '/') + 1),                                      \
           __LINE__,                                                          \
           passed ? COLOR_GREEN "PASSED" : COLOR_RED "FAILED");               \
    printf(COLOR_CYAN "  ASSERT(" #computed ")\n");                           \
    printf(COLOR_YELLOW "    %s\n\n" COLOR_RESET, passed ? "true" : "false"); \
    if (!passed) {                                                            \
      exit(EXIT_FAILURE);                                                     \
    }                                                                         \
  }

#define ASSERT_ALIGNED(computed)                                              \
  {                                                                           \
    const bool passed = (((uintptr_t)(computed)) % sizeof(uint8_t *)) == 0;   \
    printf("%s:%d: %s" COLOR_RESET "\n",                                      \
           (strrchr(__FILE__, '/') + 1),                                      \
           __LINE__,                                                          \
           passed ? COLOR_GREEN "PASSED" : COLOR_RED "FAILED");               \
    printf(COLOR_CYAN "  ASSERT_ALIGNED(" #computed ")\n");                   \
    printf(COLOR_YELLOW "    %s\n\n" COLOR_RESET, passed ? "true" : "false"); \
    if (!passed) {                                                            \
      exit(EXIT_FAILURE);                                                     \
    }                                                                         \
  }

#define ASSERT_FALSE(computed)                                                \
  {                                                                           \
    const bool passed = !(computed);                                          \
    printf("%s:%d: %s" COLOR_RESET "\n",                                      \
           (strrchr(__FILE__, '/') + 1),                                      \
           __LINE__,                                                          \
           passed ? COLOR_GREEN "PASSED" : COLOR_RED "FAILED");               \
    printf(COLOR_CYAN "  ASSERT(" #computed ")\n");                           \
    printf(COLOR_YELLOW "    %s\n\n" COLOR_RESET, passed ? "true" : "false"); \
    if (!passed) {                                                            \
      exit(EXIT_FAILURE);                                                     \
    }                                                                         \
  }

#define ASSERT_NULL(computed)                                   \
  {                                                             \
    const void *result = computed;                              \
    const bool passed = result == NULL;                         \
    printf("%s:%d: %s" COLOR_RESET "\n",                        \
           (strrchr(__FILE__, '/') + 1),                        \
           __LINE__,                                            \
           passed ? COLOR_GREEN "PASSED" : COLOR_RED "FAILED"); \
    printf(COLOR_CYAN "  ASSERT_NULL(" #computed ")\n");        \
    printf(COLOR_YELLOW "    %p\n\n" COLOR_RESET, result);      \
    if (!passed) {                                              \
      exit(EXIT_FAILURE);                                       \
    }                                                           \
  }

#define ASSERT_NON_NULL(computed)                               \
  {                                                             \
    const void *result = computed;                              \
    const bool passed = result != NULL;                         \
    printf("%s:%d: %s" COLOR_RESET "\n",                        \
           (strrchr(__FILE__, '/') + 1),                        \
           __LINE__,                                            \
           passed ? COLOR_GREEN "PASSED" : COLOR_RED "FAILED"); \
    printf(COLOR_CYAN "  ASSERT_NON_NULL(" #computed ")\n");    \
    printf(COLOR_YELLOW "    %p\n\n" COLOR_RESET, result);      \
    if (!passed) {                                              \
      exit(EXIT_FAILURE);                                       \
    }                                                           \
  }

#define ASSERT_EQ(expected, computed)                                     \
  {                                                                       \
    const int64_t lhs = (int64_t)expected;                                \
    const int64_t rhs = (int64_t)(computed);                              \
    const bool passed = lhs == rhs;                                       \
    printf("%s:%d: %s" COLOR_RESET "\n",                                  \
           (strrchr(__FILE__, '/') + 1),                                  \
           __LINE__,                                                      \
           (passed ? COLOR_GREEN "PASSED" : COLOR_RED "FAILED"));         \
    printf(COLOR_CYAN "  ASSERT_EQ(" #expected ", " #computed ")\n");     \
    printf(COLOR_YELLOW "    %" PRId64 " == %" PRId64 "\n\n" COLOR_RESET, \
           lhs,                                                           \
           rhs);                                                          \
    if (!passed) {                                                        \
      exit(EXIT_FAILURE);                                                 \
    }                                                                     \
  }

#define ASSERT_STR_EQ(expected, computed)                                  \
  {                                                                        \
    bool passed = false;                                                   \
    const char *restrict lhs = expected;                                   \
    const char *restrict rhs = computed;                                   \
    if (lhs == NULL || rhs == NULL) {                                      \
      passed = lhs == NULL && rhs == NULL;                                 \
    } else {                                                               \
      passed = strcmp(lhs, rhs) == 0;                                      \
    }                                                                      \
    printf("%s:%d: %s" COLOR_RESET "\n",                                   \
           (strrchr(__FILE__, '/') + 1),                                   \
           __LINE__,                                                       \
           passed ? COLOR_GREEN "PASSED" : COLOR_RED "FAILED");            \
    printf(COLOR_CYAN "  ASSERT_STR_EQ(" #expected ", " #computed ")\n");  \
    printf(COLOR_YELLOW "    \"%s\" == \"%s\"\n\n" COLOR_RESET, lhs, rhs); \
    if (!passed) {                                                         \
      exit(EXIT_FAILURE);                                                  \
    }                                                                      \
  }

#define ASSERT_XML_ERR(arena, xmlstr, expected)                 \
  {                                                             \
    arena_reset(arena);                                         \
    sax_parser_t *restrict parser = sax_parser(&(sax_config_t){ \
        .xml = xmlstr,                                          \
        .alloc = arena_custom_alloc,                            \
        .alloc_ctx = arena,                                     \
    });                                                         \
    ASSERT_EQ(SAX_EVENT_ERROR, sax_next(parser));               \
    ASSERT_STR_EQ(expected, sax_error(parser));                 \
    sax_parser_free(parser);                                    \
  }

#define ASSERT_XML_SEQ(arena, xmlstr, ...)                                         \
  {                                                                                \
    bool passed = true;                                                            \
    arena_reset(arena);                                                            \
    sax_parser_t *restrict parser = sax_parser(&(sax_config_t){                    \
        .xml = xmlstr,                                                             \
        .alloc = arena_custom_alloc,                                               \
        .alloc_ctx = arena,                                                        \
    });                                                                            \
    const sax_event_t expected[] = {__VA_ARGS__, 0};                               \
    const sax_event_t *ev = expected;                                              \
    sax_event_t actual = SAX_EVENT_END_DOCUMENT;                                   \
    for (; *ev != 0; ++ev) {                                                       \
      actual = sax_next(parser);                                                   \
      if (*ev != actual) {                                                         \
        passed = false;                                                            \
        break;                                                                     \
      }                                                                            \
    }                                                                              \
    sax_parser_free(parser);                                                       \
    printf("%s:%d: %s" COLOR_RESET "\n",                                           \
           (strrchr(__FILE__, '/') + 1),                                           \
           __LINE__,                                                               \
           passed ? COLOR_GREEN "PASSED" : COLOR_RED "FAILED");                    \
    printf(COLOR_CYAN "  ASSERT_XML_SEQ(arena, " #xmlstr ", " #__VA_ARGS__ ")\n"); \
    if (!passed) {                                                                 \
      printf(COLOR_YELLOW "    %d != %d\n\n" COLOR_RESET, *ev, actual);            \
      exit(EXIT_FAILURE);                                                          \
    }                                                                              \
  }

void *test_null_alloc(void *ctx, void *ptr, size_t size);

#endif