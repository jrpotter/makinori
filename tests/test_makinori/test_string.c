#include "../test.h"

MN_TEST_CASE(mn_view_eq, {
  auto v1 = mn_str_lit("abc123 .*").view;
  auto v2 = mn_str_lit("abc123 .*").view;
  auto v3 = mn_str_lit("abc123 .&").view;
  auto v4 = mn_str_lit("abC123 .*").view;

  mn_assert_true(mn_view_eq(v1, v2));
  mn_assert_false(mn_view_eq(v1, v3));
  mn_assert_false(mn_view_eq(v1, v4));
})

MN_TEST_CASE(mn_view_ieq, {
  auto v1 = mn_str_lit("abc123 .*").view;
  auto v2 = mn_str_lit("abc123 .*").view;
  auto v3 = mn_str_lit("abc123 .&").view;
  auto v4 = mn_str_lit("abC123 .*").view;

  mn_assert_true(mn_view_ieq(v1, v2));
  mn_assert_false(mn_view_ieq(v1, v3));
  mn_assert_true(mn_view_ieq(v1, v4));
})

MN_TEST_CASE(mn_view_substr, {
  auto vv = mn_str_lit("abc123 .*").view;
  auto v1 = mn_view_substr(vv, 0, 3);
  auto v2 = mn_view_substr(vv, 0, SIZE_MAX);
  auto v3 = mn_view_substr(vv, 3, 2);

  mn_assert_true(mn_view_eq(v1, mn_view_ref("abc", 3)));
  mn_assert_true(mn_view_eq(v2, vv));
  mn_assert_true(mn_view_eq(v3, mn_str_lit("").view));
})

MN_TEST_CASE(mn_view_cpy, {
  {
    char buffer[128] = {};
    auto v1 = mn_str_lit("abc123 .*").view;
    size_t n = mn_view_cpy(buffer, v1);
    mn_assert_eq(n, v1.len);

    auto v2 = mn_view_ref(buffer, n);
    mn_assert_true(mn_view_eq(v1, v2));
}

{
  char buffer[128] = {};
  auto v1 = mn_str_lit("abc123 .*").view;
  size_t n = mn_view_cpy(buffer, v1, 3);
  mn_assert_eq(n, 3);

  auto v2 = mn_str_lit("abc").view;
  auto v3 = mn_view_ref(buffer, n);
  mn_assert_true(mn_view_eq(v2, v3));
}
})

MN_TEST_CASE(mn_view_find, {
  auto v1 = mn_str_lit("abc123 .*").view;

  size_t n1 = mn_view_find(v1, v1);
  mn_assert_eq(n1, 0);

  size_t n2 = mn_view_find(v1, mn_str_lit("abc").view);
  mn_assert_eq(n2, 0);

  size_t n3 = mn_view_find(v1, mn_str_lit("123").view);
  mn_assert_eq(n3, 3);

  size_t n4 = mn_view_find(v1, mn_str_lit(" .*").view);
  mn_assert_eq(n4, 6);
})
