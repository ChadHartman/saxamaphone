#ifndef TEST_H
#define TEST_H

#include <saxamaphone.h>
#include <stdbool.h>

sax_str_t sax_str_substr(
    const sax_str_t src,
    sax_size_t start,
    sax_size_t len);

sax_str_t sax_str(const char *restrict value);

bool sax_str_equals(const sax_str_t lhs, const sax_str_t rhs);

sax_str_t sax_str_unescaped(const sax_str_t src);

#endif