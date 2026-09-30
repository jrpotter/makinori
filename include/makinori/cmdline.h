#pragma once

#include "makinori/config.h"

struct mn_flag {
  struct mn_str sflag;
  struct mn_str lflag;
  unsigned int arity;
  bool set;
  struct mn_str vals[MN_CMDLINE_MAX_ARITY];
};

struct mn_cmdline {
  struct mn_str action;
  struct mn_flag *flags[MN_CMDLINE_MAX_FLAGS];
};

struct mn_status mn_cmdline_parse(
    int const argc,
    char const *argv[const argc],
    struct mn_cmdline cl[static 1]);
