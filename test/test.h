#ifndef TEST_H
#define TEST_H

#include <inttypes.h> // PRId64
#include <stdbool.h>
#include <stdio.h>
#include <string.h> // strrchr

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_RESET "\x1b[0m"

#define TEST(header)                                                  \
  {                                                                   \
    const size_t len = strlen(header);                                \
    putchar('+');                                                     \
    for (size_t i = 0; i < len + 4; ++i) {                            \
      putchar('-');                                                   \
    }                                                                 \
    printf("+\n| " COLOR_MAGENTA " %s " COLOR_RESET " |\n+", header); \
    for (size_t i = 0; i < len + 4; ++i) {                            \
      putchar('-');                                                   \
    }                                                                 \
    printf("+\n");                                                    \
  }

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

#define ASSERT_EQ(expected, computed)                                             \
  {                                                                               \
    const int64_t lhs = (int64_t)expected;                                        \
    const int64_t rhs = (int64_t)(computed);                                      \
    const bool passed = lhs == rhs;                                               \
    printf("%s:%d: %s" COLOR_RESET "\n",                                          \
           (strrchr(__FILE__, '/') + 1),                                          \
           __LINE__,                                                              \
           passed ? COLOR_GREEN "PASSED" : COLOR_RED "FAILED");                   \
    printf(COLOR_CYAN "  ASSERT_EQ(" #expected ", " #computed ")\n");             \
    printf(COLOR_YELLOW "    \"%" PRId64 "\" == \"%" PRId64 "\"\n\n" COLOR_RESET, \
           lhs,                                                                   \
           rhs);                                                                  \
    if (!passed) {                                                                \
      exit(EXIT_FAILURE);                                                         \
    }                                                                             \
  }

#define ASSERT_STR_EQ(expected, computed)                                  \
  {                                                                        \
    const char *lhs = expected;                                            \
    const char *rhs = computed;                                            \
    const bool passed = strcmp(lhs, rhs) == 0;                             \
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

#define ASSERT_STRN_EQ(expected, computed, len)                           \
  {                                                                       \
    const char *lhs = expected;                                           \
    const char *rhs = computed;                                           \
    const bool passed = strncmp(lhs, rhs, len) == 0;                      \
    printf("%s:%d: %s" COLOR_RESET "\n",                                  \
           (strrchr(__FILE__, '/') + 1),                                  \
           __LINE__,                                                      \
           passed ? COLOR_GREEN "PASSED" : COLOR_RED "FAILED");           \
    printf(COLOR_CYAN "  ASSERT_STR_EQ(" #expected ", " #computed ")\n"); \
    printf(COLOR_YELLOW "    \"%.*s\" == \"%.*s\"\n\n" COLOR_RESET,       \
           (int)len, lhs, (int)len, rhs);                                 \
    if (!passed) {                                                        \
      exit(EXIT_FAILURE);                                                 \
    }                                                                     \
  }

void sax_str_substr(
    const char *src,
    uint_fast32_t start,
    uint_fast32_t len,
    const char **substr,
    size_t *substr_len);

// sax_str_t sax_str_unescaped(const sax_str_t src);

#endif