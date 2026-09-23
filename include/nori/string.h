#pragma once

#include <stddef.h>
#include <stdint.h>

// =================================================================================
// Views

/// A pointer to a portion of an existing NUL-terminated string.
///
/// Keep public to permit pass by value, but avoid accessing these fields directly.
struct nori_view {
  char const *ss_;
  size_t len_;
};

// Utilities for use in printf-like statements.
#define PRInv "%.*s"
#define nori_view_arg(nv) (int){(nv).len_}, (nv).ss_

/// Create a nori_view pointing to ss.
///
/// @param ss - The string to reference. Must remain in memory as long as the view
///             is being used.
/// @param len - The length of the view. Pass 0 to indicate up to '\0' character.
struct nori_view const nori_view_ref(char const ss[static 1], size_t const len);

/// Return whether the view is of an empty string.
inline bool nori_view_empty(struct nori_view const v)
{
  return v.len_ == 0;
}

/// Create a new string view corresponding to the substring [i, j) of another.
struct nori_view const
nori_view_substr(struct nori_view const, size_t const i, size_t const j);

/// Check if two views are case-sensitive equal.
bool nori_view_eq(struct nori_view const, struct nori_view const);

/// Check if two views are case-insensitive equal.
bool nori_view_ieq(struct nori_view const, struct nori_view const);

// =================================================================================
// Strings

/// A wrapper around a NUL-terminated string.
///
/// Unlike the nori_view, this always corresponds to the entirety of a string. As
/// such, it is safe to read the fields directly.
struct nori_str {
  char const *ss;
  size_t len;
};

struct nori_str const nori_str_ref(char const ss[static 1], size_t const len);

#define nori_str_lit(X)                                                                \
  (struct nori_str){.ss = ("" X ""), .len = (sizeof(X) / sizeof(X[0])) - 1}

inline struct nori_view const nori_str_to_view(struct nori_str const s)
{
  return (struct nori_view){.ss_ = s.ss, .len_ = s.len};
}

inline struct nori_view const
nori_str_substr(struct nori_str const s, size_t const i, size_t const j)
{
  return nori_view_substr(nori_str_to_view(s), i, j);
}

inline bool nori_str_eq(struct nori_str const s1, struct nori_str const s2)
{
  return nori_view_eq(nori_str_to_view(s1), nori_str_to_view(s2));
}

inline bool nori_str_ieq(struct nori_str const s1, struct nori_str const s2)
{
  return nori_view_ieq(nori_str_to_view(s1), nori_str_to_view(s2));
}
