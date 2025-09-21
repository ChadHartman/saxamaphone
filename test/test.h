#ifndef TEST_H
#define TEST_H

#include <saxamaphone.h>
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
  }

#define ASSERT_NON_NULL(computed)                               \
  {                                                             \
    const void *result = computed;                              \
    const bool passed = result != NULL;                         \
    printf("%s:%d: %s" COLOR_RESET "\n",                        \
           (strrchr(__FILE__, '/') + 1),                        \
           __LINE__,                                            \
           passed ? COLOR_GREEN "PASSED" : COLOR_RED "FAILED"); \
    printf(COLOR_CYAN "  ASSERT_NON_NULL(" #computed ")\n");        \
    printf(COLOR_YELLOW "    %p\n\n" COLOR_RESET, result);      \
  }

#define ASSERT_STR_EQ(expected, computed)                                 \
  {                                                                       \
    const sax_str_t lhs = sax_str(expected);                              \
    const sax_str_t rhs = computed;                                       \
    const bool passed = sax_str_equals(lhs, rhs);                         \
    printf("%s:%d: %s" COLOR_RESET "\n",                                  \
           (strrchr(__FILE__, '/') + 1),                                  \
           __LINE__,                                                      \
           passed ? COLOR_GREEN "PASSED" : COLOR_RED "FAILED");           \
    printf(COLOR_CYAN "  ASSERT_STR_EQ(" #expected ", " #computed ")\n"); \
    printf(COLOR_YELLOW "    \"%.*s\" == \"%.*s\"\n\n" COLOR_RESET,       \
           lhs.size,                                                      \
           lhs.value,                                                     \
           rhs.size,                                                      \
           rhs.value);                                                    \
    if (!passed) {                                                        \
      exit(EXIT_FAILURE);                                                 \
    }                                                                     \
  }

sax_str_t sax_str_substr(
    const sax_str_t src,
    sax_size_t start,
    sax_size_t len);

sax_str_t sax_str(const char *restrict value);

bool sax_str_equals(const sax_str_t lhs, const sax_str_t rhs);

sax_str_t sax_str_unescaped(const sax_str_t src);

#endif