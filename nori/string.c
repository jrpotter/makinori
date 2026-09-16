#include "nori/string.h"

bool nori_str_eq(const struct nori_str s1, const struct nori_str s2)
{
  if (s1.len != s2.len) {
    return false;
  }

  const char *p = s1.ss, *q = s2.ss;
  for (; *p && *q; ++p, ++q) {
    if (*p != *q) {
      return false;
    }
  }

  if (*p || *q) {
    return false;
  }

  return true;
}
