#include "nori/string.h"

struct nori_str nori_str_wrap(const char ss[static 1])
{
  size_t len = 0;
  for (const char *p = ss; *p; ++p) {
    len += 1;
  }

  return (struct nori_str){
      .ss = ss,
      .len = len,
      .capacity = len + 1,
  };
}

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
