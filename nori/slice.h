#pragma once

#include <stdlib.h>

#define SS(X)                                                                          \
  ((struct nori_slice){.ss = ("" X ""), .len = (sizeof(X) / sizeof(X[0])) - 1})

/// An immutable representation of a string.
///
/// The `ss` pointer must remain valid while a slice is in use.
struct nori_slice {
  const char *ss;
  size_t len;
};

/// Construct a struct nori_slice instance pointing to @ss.
///
/// In general, prefer using the `SS` macro for compile-time construction.
struct nori_slice nori_slice_wrap(const char ss[static 1]);

/// Create a new slice corresponding to the suffix of another.
struct nori_slice nori_slice_suffix(const struct nori_slice s, const size_t offset);

/// Check if two slices are equal.
bool nori_slice_eq(const struct nori_slice s1, const struct nori_slice s2);
