#include "../test.h"

MN_TEST_CASE(MN_ARR_SIZE, {
  int arr1[] = {};
  int arr2[] = {1, 2, 3};
  mn_assert_eq(MN_ARR_SIZE(arr1), 0);
  mn_assert_eq(MN_ARR_SIZE(arr2), 3);
})

MN_TEST_CASE(MN_MAX, {
  int x = 1;
  int y = 2;
  mn_assert_eq(MN_MAX(x, y), y);
})

MN_TEST_CASE(MN_MIN, {
  int x = 1;
  int y = 2;
  mn_assert_eq(MN_MIN(x, y), x);
})

MN_TEST_CASE(MN_STR_LEN, {
  char const str1[] = "";
  char const str2[] = "abc";
  mn_assert_eq(MN_STR_LEN(str1), 0);
  mn_assert_eq(MN_STR_LEN(str2), 3);
})
