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

#define PRInv "%.*s"
#define mn_view_pri(X) (int){(X).len}, (X).ss_

struct mn_view const mn_view_ref(char const ss[static 1], size_t const len);

struct mn_view const
mn_view_substr(struct mn_view const v, size_t const i, size_t const j);

bool mn_view_eq(struct mn_view const, struct mn_view const);

bool mn_view_ieq(struct mn_view const, struct mn_view const);

// =================================================================================
// Strings

struct mn_str {
  char const *ss;
  size_t len;
};

#define mn_str_lit(X) ((struct mn_str){.ss = ("" X ""), .len = MN_STR_LEN(X)})

struct mn_str const mn_str_ref(char const ss[static 1], size_t const len);

inline struct mn_view const mn_str_to_view(struct mn_str const s)
{
  return (struct mn_view){.ss_ = s.ss, .len = s.len};
}

inline struct mn_view const
mn_str_substr(struct mn_str const s, size_t const i, size_t const j)
{
  return mn_view_substr(mn_str_to_view(s), i, j);
}

inline bool mn_str_eq(struct mn_str const s1, struct mn_str const s2)
{
  return mn_view_eq(mn_str_to_view(s1), mn_str_to_view(s2));
}

inline bool mn_str_ieq(struct mn_str const s1, struct mn_str const s2)
{
  return mn_view_ieq(mn_str_to_view(s1), mn_str_to_view(s2));
}
