#pragma once

#include "arena.h"
#include "test.h"

#define TEST_REG(test_name) {.name = #test_name, .func = test_##test_name}

typedef struct test_t {
  const char *name;
  void (*func)(arena_t *restrict);
} test_t;

TEST(arena_alloc);
TEST(attr);
TEST(attr_val);
TEST(cdata);
TEST(cdata_malformed);
TEST(code_pt_size);
TEST(comment);
TEST(content);
TEST(default_alloc);
TEST(end_tag);
TEST(endswith);
TEST(getter);
TEST(long_to_code_pt);
TEST(ltrim);
TEST(map_alpha);
TEST(map_encode);
TEST(map_malformed);
TEST(map_view);
TEST(mapper);
TEST(obj_map);
TEST(parser);
TEST(proc_inst);
TEST(rtrim);
TEST(startswith);
TEST(start_tag);
TEST(start_tag_closed);
TEST(start_tag_oom);
TEST(str_eq);
TEST(trim);
TEST(unescape);
TEST(unescape10);
TEST(unescape16);

const test_t tests[] = {
    TEST_REG(arena_alloc),
    TEST_REG(attr),
    TEST_REG(attr_val),
    TEST_REG(cdata),
    TEST_REG(cdata_malformed),
    TEST_REG(code_pt_size),
    TEST_REG(comment),
    TEST_REG(content),
    TEST_REG(default_alloc),
    TEST_REG(end_tag),
    TEST_REG(endswith),
    TEST_REG(getter),
    TEST_REG(long_to_code_pt),
    TEST_REG(ltrim),
    TEST_REG(map_alpha),
    TEST_REG(map_encode),
    TEST_REG(map_malformed),
    TEST_REG(map_view),
    TEST_REG(mapper),
    TEST_REG(obj_map),
    TEST_REG(parser),
    TEST_REG(proc_inst),
    TEST_REG(rtrim),
    TEST_REG(startswith),
    TEST_REG(start_tag),
    TEST_REG(start_tag_closed),
    TEST_REG(start_tag_oom),
    TEST_REG(str_eq),
    TEST_REG(trim),
    TEST_REG(unescape),
    TEST_REG(unescape10),
    TEST_REG(unescape16),
};