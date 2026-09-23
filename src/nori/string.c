#include <stdlib.h>
#include <string.h>

#include "nori/string.h"

struct nori_str_view nori_str_view_wrap(char const ss[static 1])
{
  size_t len = 0;
  for (char const *p = ss; *p; ++p) {
    len += 1;
  }
  return nori_str_view_create(ss, len);
}

struct nori_str_view nori_str_view_create(char const ss[static 1], size_t const len)
{
  return (struct nori_str_view){.view = ss, .len = len};
}

struct nori_str_view
nori_str_view_substr(struct nori_str_view const s, size_t const i, size_t const j)
{
  size_t const end = j < s.len ? j : s.len;

  struct nori_str_view substr = {};
  if (i < end) {
    substr.len = end - i;
    substr.view = s.view + i;
  }

  return substr;
}

bool nori_str_view_eq(struct nori_str_view const s1, struct nori_str_view const s2)
{
  if (s1.len != s2.len) {
    return false;
  }

  for (size_t i = 0; i < s1.len; ++i) {
    if (s1.view[i] != s2.view[i]) {
      return false;
    }
  }

  return true;
}

bool nori_str_view_ieq(struct nori_str_view const s1, struct nori_str_view const s2)
{
  if (s1.len != s2.len) {
    return false;
  }

  for (size_t i = 0; i < s1.len; ++i) {
    char a = s1.view[i];
    char b = s2.view[i];
    a += (a >= 'A' && a <= 'Z') ? 'a' - 'A' : 0;
    b += (b >= 'A' && b <= 'Z') ? 'a' - 'A' : 0;
    if (a != b) {
      return false;
    }
  }

  return true;
}
