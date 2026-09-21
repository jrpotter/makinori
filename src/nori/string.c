#include <stdlib.h>
#include <string.h>

#include "nori/string.h"
#include "nori/util.h"

struct nori_str_view nori_str_view_of(char const ss[static 1])
{
  size_t len = 0;
  for (char const *p = ss; *p; ++p) {
    len += 1;
  }
  return (struct nori_str_view){.view = ss, .len = len};
}

struct nori_str_view
nori_str_view_suffix(struct nori_str_view const s, size_t const offset)
{
  struct nori_str_view suffix = {};
  if (offset < s.len) {
    suffix.view = s.view + offset;
    suffix.len = s.len - offset;
  }
  return suffix;
}

bool nori_str_view_eq(struct nori_str_view const s1, struct nori_str_view const s2)
{
  if (s1.len != s2.len) {
    return false;
  }

  char const *p = s1.view, *q = s2.view;
  for (; *p && *q; ++p, ++q) {
    if (*p != *q) {
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

  char const *p = s1.view, *q = s2.view;
  for (; *p && *q; ++p, ++q) {
    char a = *p, b = *q;
    a += (a >= 'A' && a <= 'Z') ? 'a' - 'A' : 0;
    b += (b >= 'A' && b <= 'Z') ? 'a' - 'A' : 0;
    if (a != b) {
      return false;
    }
  }

  return true;
}

struct nori_status
nori_str_create(size_t const capacity, struct nori_str out[const static 1])
{
  memset(out, 0, sizeof(*out));

  char *buffer = nullptr;
  if (capacity > 0) {
    buffer = malloc(sizeof(*buffer) * capacity);
    if (!buffer) {
      return NORI_FAILURE_ERROR("Could not allocate buffer");
    }
  }

  out->buffer = buffer;
  out->len = 0;
  out->capacity = capacity;

  return NORI_SUCCESS;
}

struct nori_status(nori_str_copy)(
    struct nori_str dst[const static 1],
    struct nori_str_view src,
    size_t offset)
{
  if (offset + src.len > dst->capacity) {
    char *new_buffer = realloc(dst->buffer, dst->capacity * 2);
    if (!new_buffer) {
      return NORI_FAILURE_ERROR("Could not reallocate buffer");
    }
    dst->buffer = new_buffer;
    dst->capacity = dst->capacity * 2;
  }

  for (size_t i = offset; i < offset + src.len; ++i) {
    dst->buffer[i] = src.view[i - offset];
  }
  dst->buffer[offset + src.len] = '\0';
  dst->len = offset + src.len - 1;
  return NORI_SUCCESS;
}

void nori_str_destroy(struct nori_str str[const static 1])
{
  free(str->buffer);
  memset(str, 0, sizeof(*str));
}
