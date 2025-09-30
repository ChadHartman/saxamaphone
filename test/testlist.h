#pragma once

#include "arena.h"
#include "test.h"

#define TEST_REG(test_name) {.name = #test_name, .func = test_##test_name}

typedef struct test_t {
  const char *name;
  void (*func)(arena_t *restrict);
} test_t;

TEST(alloc);
TEST(attr_val);
TEST(cdata);
TEST(cdata_malformed);
TEST(code_pt_size);
TEST(comment);
TEST(content);
TEST(file_buf_size);
TEST(obj_map);
TEST(proc_inst);
TEST(start_tag);
TEST(start_tag_closed);
TEST(trim);
TEST(unescape);
TEST(unescape10);
TEST(unescape16);

const test_t tests[] = {
    TEST_REG(alloc),
    TEST_REG(attr_val),
    TEST_REG(cdata),
    TEST_REG(cdata_malformed),
    TEST_REG(code_pt_size),
    TEST_REG(comment),
    TEST_REG(content),
    TEST_REG(file_buf_size),
    TEST_REG(obj_map),
    TEST_REG(proc_inst),
    TEST_REG(start_tag),
    TEST_REG(start_tag_closed),
    TEST_REG(trim),
    TEST_REG(unescape),
    TEST_REG(unescape10),
    TEST_REG(unescape16),
};