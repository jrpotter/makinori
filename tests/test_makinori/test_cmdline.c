#include "../test.h"

MN_TEST_CASE(mn_cmdline_empty, {
  int constexpr argc = 1;
  char const *argv[argc] = {"./bin"};

  struct mn_cmdline cl = {};
  mn_assert_success(mn_cmdline_parse(argc, argv, &cl));
  mn_assert_eq(cl.action.len, 0);
})

MN_TEST_CASE(mn_cmdline_single_action, {
  int constexpr argc = 2;
  char const *argv[argc] = {"./bin", "test"};

  struct mn_cmdline cl = {};
  mn_assert_success(mn_cmdline_parse(argc, argv, &cl));
  mn_assert_str_eq(cl.action, mn_str_lit("test"));
})

MN_TEST_CASE(mn_cmdline_arity_zero_short, {
  int constexpr argc = 2;
  char const *argv[argc] = {"./bin", "-h"};

  struct mn_flag flag_help = {
      .sflag = mn_str_lit("h"),
      .lflag = mn_str_lit("help"),
      .arity = 0,
  };

  struct mn_flag flag_other = {
      .sflag = mn_str_lit("o"),
      .lflag = mn_str_lit("other"),
      .arity = 0,
  };

  struct mn_cmdline cl = {.flags = {&flag_help, &flag_other}};
  mn_assert_success(mn_cmdline_parse(argc, argv, &cl));
  mn_assert_true(flag_help.set);
  mn_assert_false(flag_other.set);
})

MN_TEST_CASE(mn_cmdline_arity_zero_long, {
  int constexpr argc = 2;
  char const *argv[argc] = {"./bin", "--help"};

  struct mn_flag flag_help = {
      .sflag = mn_str_lit("h"),
      .lflag = mn_str_lit("help"),
      .arity = 0,
  };

  struct mn_flag flag_other = {
      .sflag = mn_str_lit("o"),
      .lflag = mn_str_lit("other"),
      .arity = 0,
  };

  struct mn_cmdline cl = {.flags = {&flag_help, &flag_other}};
  mn_assert_success(mn_cmdline_parse(argc, argv, &cl));
  mn_assert_true(flag_help.set);
  mn_assert_false(flag_other.set);
})

MN_TEST_CASE(mn_cmdline_arity_one, {
  int constexpr argc = 3;
  char const *argv[argc] = {"./bin", "--port", "8000"};

  struct mn_flag flag_port = {
      .lflag = mn_str_lit("port"),
      .arity = 1,
  };

  struct mn_cmdline cl = {.flags = {&flag_port}};
  mn_assert_success(mn_cmdline_parse(argc, argv, &cl));
  mn_assert_true(flag_port.set);
  mn_assert_str_eq(flag_port.vals[0], mn_str_lit("8000"));
})

MN_TEST_CASE(mn_cmdline_arity_one_missing, {
  int constexpr argc = 2;
  char const *argv[argc] = {"./bin", "--port"};

  struct mn_flag flag_port = {
      .lflag = mn_str_lit("port"),
      .arity = 1,
  };

  struct mn_cmdline cl = {.flags = {&flag_port}};
  mn_assert_status(mn_cmdline_parse(argc, argv, &cl), MN_ERROR_CONFIG);
  mn_assert_false(flag_port.set);
})

MN_TEST_CASE(mn_cmdline_action_and_flag, {
  int constexpr argc = 4;
  char const *argv[argc] = {"./bin", "test", "--port", "8000"};

  struct mn_flag flag_port = {
      .lflag = mn_str_lit("port"),
      .arity = 1,
  };

  struct mn_cmdline cl = {.flags = {&flag_port}};
  mn_assert_success(mn_cmdline_parse(argc, argv, &cl));
  mn_assert_str_eq(cl.action, mn_str_lit("test"));
  mn_assert_true(flag_port.set);
  mn_assert_str_eq(flag_port.vals[0], mn_str_lit("8000"));
})
