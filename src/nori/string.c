#include <stdlib.h>
#include <string.h>

#include "nori/string.h"

// =================================================================================
// Views

struct nori_view const nori_view_ref(char const ss[static 1], size_t const len)
{
  size_t count = len;
  if (len == 0) {
    for (char const *p = ss; *p; ++p) {
      count += 1;
    }
  }
  return (struct nori_view){.ss_ = ss, .len_ = count};
}

bool nori_view_empty(struct nori_view const); // Emit inline

struct nori_view const
nori_view_substr(struct nori_view const v, size_t const i, size_t const j)
{
  size_t const end = j < v.len_ ? j : v.len_;

  struct nori_view substr = {};
  if (i < end) {
    substr.len_ = end - i;
    substr.ss_ = v.ss_ + i;
  }

  return substr;
}

bool nori_view_eq(struct nori_view const v1, struct nori_view const v2)
{
  if (v1.len_ != v2.len_) {
    return false;
  }

  for (size_t i = 0; i < v1.len_; ++i) {
    if (v1.ss_[i] != v2.ss_[i]) {
      return false;
    }
  }

  return true;
}

bool nori_view_ieq(struct nori_view const v1, struct nori_view const v2)
{
  if (v1.len_ != v2.len_) {
    return false;
  }

  for (size_t i = 0; i < v1.len_; ++i) {
    char a = v1.ss_[i];
    char b = v2.ss_[i];
    a += (a >= 'A' && a <= 'Z') ? 'a' - 'A' : 0;
    b += (b >= 'A' && b <= 'Z') ? 'a' - 'A' : 0;
    if (a != b) {
      return false;
    }
  }

  return true;
}

// =================================================================================
// Strings

struct nori_str const nori_str_ref(char const ss[static 1], size_t const len)
{
  size_t count = len;
  if (len == 0) {
    for (char const *p = ss; *p; ++p) {
      count += 1;
    }
  }
  return (struct nori_str){.ss = ss, .len = count};
}

struct nori_view const nori_str_to_view(struct nori_str const); // Emit inline

struct nori_view const
nori_str_substr(struct nori_str const, size_t const, size_t const); // Emit inline

bool nori_str_eq(struct nori_str const, struct nori_str const); // Emit inline

bool nori_str_ieq(struct nori_str const, struct nori_str const); // Emit inline
