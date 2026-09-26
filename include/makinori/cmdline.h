#ifdef MN_CMDLINE_SOURCE

#pragma once

#include "makinori/util.h"

enum mn_flag_arity : unsigned int {
  MN_FLAG_ARITY_ZERO = 0,
  MN_FLAG_ARITY_ONE = 1,
  MN_FLAG_ARITY_TWO = 2,
  MN_FLAG_ARITY_THREE = 3,
  MN_FLAG_ARITY_FOUR = 4,
  MN_FLAG_ARITY_FIVE = 5,
  MN_FLAG_ARITY_SIX = 6,
  MN_FLAG_ARITY_SEVEN = 7,
  MN_FLAG_ARITY_MAX,
};

struct mn_flag {
  struct mn_str vals[MN_FLAG_ARITY_MAX];
  struct mn_str sflag;
  struct mn_str lflag;
  enum mn_flag_arity arity;
  bool set;
};

extern struct mn_flag *MN_FLAG_OPTIONS[];

struct mn_status mn_cmdline_parse(
    int const argc,
    char const *argv[const argc],
    struct mn_str action[static 1]);

#endif /* MN_CMDLINE_SOURCE */
