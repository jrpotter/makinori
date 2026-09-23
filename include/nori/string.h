#pragma once

#include <stddef.h>
#include <stdint.h>

/// An immutable pointer to an existing string.
struct nori_str_view {
  char const *view;
  size_t len;
};

#define NSV(X)                                                                         \
  ((struct nori_str_view){.view = ("" X ""), .len = (sizeof(X) / sizeof(X[0])) - 1})

/// Construct a `struct nori_str_view` instance pointing to ss.
struct nori_str_view nori_str_view_wrap(char const ss[static 1]);

/// Construct a `struct nori_str_view` instance directly.
struct nori_str_view nori_str_view_create(char const ss[static 1], size_t const len);

/// Create a new string view corresponding to the substring [i, j) of another.
struct nori_str_view
nori_str_view_substr(struct nori_str_view const s, size_t const i, size_t const j);

/// Check if two string views are equal.
bool nori_str_view_eq(struct nori_str_view const s1, struct nori_str_view const s2);

/// Check if two string views are equal case-sensitively.
bool nori_str_view_ieq(struct nori_str_view const s1, struct nori_str_view const s2);
