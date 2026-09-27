#include <stdlib.h>
#include <string.h>

#include "makinori/string.h"

// =================================================================================
// Views

struct mn_view const mn_view_ref(char const ss[static 1], size_t const len)
{
  size_t count = len;
  if (len == 0) {
    for (char const *p = ss; *p; ++p) {
      count += 1;
    }
  }
  return (struct mn_view){.ss_ = ss, .len = count};
}

struct mn_view const
mn_view_substr(struct mn_view const v, size_t const i, size_t const j)
{
  size_t const end = j < v.len ? j : v.len;

  struct mn_view substr = {};
  if (i < end) {
    substr.len = end - i;
    substr.ss_ = v.ss_ + i;
  }

  return substr;
}

size_t mn_view_cpy(char *const dst, struct mn_view src)
{
  for (size_t i = 0; i < src.len; ++i) {
    dst[i] = src.ss_[i];
  }
  return src.len;
}

bool mn_view_eq(struct mn_view const v1, struct mn_view const v2)
{
  if (v1.len != v2.len) {
    return false;
  }

  for (size_t i = 0; i < v1.len; ++i) {
    if (v1.ss_[i] != v2.ss_[i]) {
      return false;
    }
  }

  return true;
}

bool mn_view_ieq(struct mn_view const v1, struct mn_view const v2)
{
  if (v1.len != v2.len) {
    return false;
  }

  for (size_t i = 0; i < v1.len; ++i) {
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

struct mn_str const mn_str_ref(char const ss[static 1], size_t const len)
{
  size_t count = len;
  if (len == 0) {
    for (char const *p = ss; *p; ++p) {
      count += 1;
    }
  }
  return (struct mn_str){.ss = ss, .len = count};
}

// --- Emit inlines ----------------------------------------------------------------

struct mn_view const mn_str_to_view(struct mn_str const);
struct mn_view const mn_str_substr(struct mn_str const, size_t const, size_t const);
size_t mn_str_cpy(char *const, struct mn_str);
bool mn_str_eq(struct mn_str const, struct mn_str const);
bool mn_str_ieq(struct mn_str const, struct mn_str const);
