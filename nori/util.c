#include "nori/util.h"

// ================================================================
// Views
// ================================================================

struct nori_view nori_view_create(const char ss[static 1])
{
  size_t len = 0;
  for (const char *p = ss; *p; ++p) {
    len += 1;
  }
  return (struct nori_view){.ss = ss, .len = len};
}

bool nori_view_eq(const struct nori_view s1, const struct nori_view s2)
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
