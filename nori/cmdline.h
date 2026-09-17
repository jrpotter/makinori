#pragma once

#include "nori/util.h"

enum nori_arity {
  NORI_ARITY_ZERO = 0,
  NORI_ARITY_ONE,
  NORI_ARITY_TWO,
  NORI_ARITY_THREE,
  NORI_ARITY_FOUR,
  NORI_ARITY_FIVE,
  NORI_ARITY_SIX,
  NORI_ARITY_SEVEN,
  NORI_ARITY_EIGHT,
};

struct nori_flag {
  bool nf_set;
  struct nori_view nf_keys[8];
  struct nori_view nf_vals[8];
  enum nori_arity nf_arity;
};

extern struct nori_flag *NORI_FLAG_OPTIONS[];

struct nori_status nori_cmdline_parse(
    const int argc,
    const char *argv[argc],
    struct nori_view *out_action);
