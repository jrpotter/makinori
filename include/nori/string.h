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

/// Construct a `struct nori_str_view` instance pointing to @ss.
///
/// In general, prefer using the `NSV` macro for compile-time construction.
struct nori_str_view nori_str_view_of(char const ss[static 1]);

/// Create a new string view corresponding to the suffix of another.
struct nori_str_view
nori_str_view_suffix(struct nori_str_view const s, size_t const offset);

/// Check if two string views are equal.
bool nori_str_view_eq(struct nori_str_view const s1, struct nori_str_view const s2);

/// Check if two string views are equal case-sensitively.
bool nori_str_view_ieq(struct nori_str_view const s1, struct nori_str_view const s2);

/// A dynamically allocated string.
///
/// Generally speaking, we want to avoid dynamic allocation. But, when processing
/// possibly dynamic HTTP responses, this is necessary.
struct nori_str {
  char *buffer;
  size_t len;      // Does not include trailing '\0'.
  size_t capacity; // Must accommodate trailing '\0'.
};

/// Create a new `struct nori_str` instance with initial @capacity.
struct nori_status
nori_str_create(size_t const capacity, struct nori_str out[const static 1]);

// Copy a `struct nori_str_view` into a `struct nori_str`.
//
// Copying automatically handles reallocating the size of the buffer if necessary.
// Macros are provided to allow omitting the @offset parameter.
//
// @param dst - The buffer we are copying into.
// @param src - The string view we are copying from.
// @param offset - The position in @dst to start copying at. If this value is larger
//                 than the length of the string, start copying at the end. A value
//                 of SIZE_MAX is a safe means of always copying at the end.
struct nori_status nori_str_copy(
    struct nori_str dst[const static 1],
    struct nori_str_view src,
    size_t offset);

#define nori_str_copy(in, sl, ...)                                                     \
  nori_str_copyI##__VA_OPT__(I)((in)__VA_OPT__(, ) __VA_ARGS__)
#define nori_str_copyI(in, sl) nori_str_copy((in), (sl), SIZE_MAX)
#define nori_str_copyII(in, sl, offset) nori_str_copy((in), (sl), (offset))

/// Destroy a previously allocated `struct nori_str`.
void nori_str_destroy(struct nori_str str[const static 1]);
