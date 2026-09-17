#include "nori/slice.h"

struct nori_slice nori_slice_wrap(const char ss[static 1])
{
  size_t len = 0;
  for (const char *p = ss; *p; ++p) {
    len += 1;
  }
  return (struct nori_slice){.ss = ss, .len = len};
}

struct nori_slice nori_slice_suffix(const struct nori_slice s, const size_t offset)
{
  struct nori_slice suffix = {};
  if (offset < s.len) {
    suffix.ss = s.ss + offset;
    suffix.len = s.len - offset;
  }
  return suffix;
}

bool nori_slice_eq(const struct nori_slice s1, const struct nori_slice s2)
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
