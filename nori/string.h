#pragma once

#include <stddef.h>

#define CPP_PROXY(X) #X
#define CPP_STR(X) CPP_PROXY(X)

struct nori_str {
  const char *ss;
  size_t len;
  size_t capacity;
};

#define NORI_STR(X)                                                                    \
  ((struct nori_str){.ss = ("" X ""),                                                  \
                     .len = (sizeof(X) / sizeof(X[0])) - 1,                            \
                     .capacity = (sizeof(X) / sizeof(X[0]))})

/**
 * Wrapper around a C string.
 *
 * Ensure the pointer remains valid while the result is in use.
 */
struct nori_str nori_str_wrap(const char ss[static 1]);

/**
 * Check if two strings are equal by value.
 *
 * @return true if s1 and s2 are equal.
 */
bool nori_str_eq(const struct nori_str s1, const struct nori_str s2);
