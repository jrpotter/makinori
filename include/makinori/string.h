#pragma once

#include <stddef.h>
#include <stdint.h>

// =================================================================================
// Views

/// A pointer to a portion of an existing NUL-terminated string.
///
/// Keep public to permit pass by value, but avoid accessing these fields directly.
struct mn_view {
  char const *ss_;
  size_t len_;
};

// Utilities for use in printf-like statements.
#define PRInv "%.*s"
#define mn_view_arg(nv) (int){(nv).len_}, (nv).ss_

/// Create a mn_view pointing to ss.
///
/// @param ss - The string to reference. Must remain in memory as long as the view
///             is being used.
/// @param len - The length of the view. Pass 0 to indicate up to '\0' character.
struct mn_view const mn_view_ref(char const ss[static 1], size_t const len);

/// Return whether the view is of an empty string.
inline bool mn_view_empty(struct mn_view const v)
{
  return v.len_ == 0;
}

/// Create a new string view corresponding to the substring [i, j) of another.
struct mn_view const
mn_view_substr(struct mn_view const, size_t const i, size_t const j);

/// Check if two views are case-sensitive equal.
bool mn_view_eq(struct mn_view const, struct mn_view const);

/// Check if two views are case-insensitive equal.
bool mn_view_ieq(struct mn_view const, struct mn_view const);

// =================================================================================
// Strings

/// A wrapper around a NUL-terminated string.
///
/// Unlike the mn_view, this always corresponds to the entirety of a string. As
/// such, it is safe to read the fields directly.
struct mn_str {
  char const *ss;
  size_t len;
};

struct mn_str const mn_str_ref(char const ss[static 1], size_t const len);

#define mn_str_lit(X)                                                                  \
  (struct mn_str){.ss = ("" X ""), .len = (sizeof(X) / sizeof(X[0])) - 1}

inline struct mn_view const mn_str_to_view(struct mn_str const s)
{
  return (struct mn_view){.ss_ = s.ss, .len_ = s.len};
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
