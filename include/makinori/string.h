#pragma once

#include <stddef.h>
#include <stdint.h>

#include "makinori/macro.h"

// =================================================================================
// Views

struct mn_view {
  char const *ss_;
  size_t len;
};

#define PRImnv "%.*s"
#define mn_view_pri(X) (int){(X).len}, (X).ss_

struct mn_view const mn_view_ref(char const ss[static 1], size_t const len);

struct mn_view const
mn_view_substr(struct mn_view const v, size_t const i, size_t const j);

size_t mn_view_cpy(
    char *const dst,
    struct mn_view const src,
    /* OPTIONAL */ size_t const count);

#define mn_view_cpy(dst, src, ...)                                                     \
  mn_view_cpyI##__VA_OPT__(I)((dst), (src)__VA_OPT__(, ) __VA_ARGS__)
#define mn_view_cpyI(dst, src) mn_view_cpy((dst), (src), SIZE_MAX)
#define mn_view_cpyII(dst, src, count) mn_view_cpy((dst), (src), (count))

size_t mn_view_find(struct mn_view const haystack, struct mn_view const needle);

bool mn_view_eq(struct mn_view const, struct mn_view const);
bool mn_view_ieq(struct mn_view const, struct mn_view const);

// =================================================================================
// Strings

struct mn_str {
  char const *ss;
  size_t len;
  struct mn_view view;
};

#define mn_str_lit(X)                                                                  \
  ((struct mn_str){                                                                    \
      .ss = ("" X ""),                                                                 \
      .len = MN_STR_LEN(X),                                                            \
      .view = (struct mn_view){.ss_ = ("" X ""), .len = MN_STR_LEN(X)},                \
  })

struct mn_str const mn_str_ref(char const ss[static 1], size_t const len);
