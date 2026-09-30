#pragma once

#include <stdio.h>  // IWYU pragma: keep
#include <string.h> // IWYU pragma: keep

#include "makinori.h"

size_t constexpr MN_FIELD_SIZE = 1024;
size_t constexpr MN_MESSAGE_SIZE = MN_FIELD_SIZE * 2;

struct mn_test_case {
  char const *name;
  void (*func)(struct mn_test_case *);
  char file[MN_FIELD_SIZE];
  char line[MN_FIELD_SIZE];
  char error[MN_MESSAGE_SIZE];
};

extern struct mn_test_case MN_TEST_SUITE[];

#define MN_TEST_CASE(name_, body_)                                                     \
  void test_##name_(struct mn_test_case *test_case_) body_;                            \
                                                                                       \
  [[gnu::constructor]]                                                                 \
  void mn_constructor_test_##name_(void)                                               \
  {                                                                                    \
    MN_TEST_SUITE[__COUNTER__] =                                                       \
        (struct mn_test_case){.name = MN_STR_TO(test_##name_),                         \
                              .func = test_##name_,                                    \
                              .file = __FILE__,                                        \
                              .line = MN_STR_TO(__LINE__)};                            \
  }

#define mn_assert_generic_(test_case, expr, msg, ...)                                  \
  ({                                                                                   \
    auto result = !!(expr);                                                            \
    if (!result) {                                                                     \
      memset(test_case->line, 0, MN_FIELD_SIZE);                                       \
      strcpy(test_case->line, MN_STR_TO(__LINE__));                                    \
      sprintf(test_case->error, msg __VA_OPT__(, ) __VA_ARGS__);                       \
      return;                                                                          \
    }                                                                                  \
  })

#define mn_assert_true(expr)                                                           \
  mn_assert_generic_(test_case_, (expr), "should be: %s", #expr)

#define mn_assert_false(expr)                                                          \
  mn_assert_generic_(test_case_, !(expr), "should be: !%s", #expr)

#define mn_assert_eq(e1, e2)                                                           \
  mn_assert_generic_(test_case_, (e1) == (e2), "should be: %s == %s", #e1, #e2)

#define mn_assert_neq(e1, e2)                                                          \
  mn_assert_generic_(test_case_, (e1) != (e2), "should be: %s != %s", #e1, #e2)
