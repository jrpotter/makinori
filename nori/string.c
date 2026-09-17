#include <stdlib.h>

#include "nori/string.h"

// ================================================================
// String
// ================================================================

struct nori_status nori_str_create(size_t capacity, struct nori_str out[const static 1])
{
  memset(out, 0, sizeof(*out));

  uint8_t *buffer = nullptr;
  if (capacity > 0) {
    buffer = malloc(sizeof(*buffer) * capacity);
    if (!buffer) {
      return NORI_FAILURE_ERROR("Could not allocate buffer");
    }
  }

  out->ss = buffer;
  out->len = 0;
  out->capacity = capacity;

  return NORI_SUCCESS;
}

void nori_str_destroy(struct nori_str str[const static 1])
{
  free(str->ss);
  str->ss = nullptr;
  str->len = 0;
  str->capacity = 0;
}

// ================================================================
// Bytes
// ================================================================

struct nori_status nori_buf_create(size_t capacity, struct nori_buf out[const static 1])
{
  memset(out, 0, sizeof(*out));

  uint8_t *buffer = nullptr;
  if (capacity > 0) {
    buffer = malloc(sizeof(*buffer) * capacity);
    if (!buffer) {
      return NORI_FAILURE_ERROR("Could not allocate buffer");
    }
  }

  out->bs = buffer;
  out->len = 0;
  out->capacity = capacity;

  return NORI_SUCCESS;
}

void nori_buf_destroy(struct nori_buf buf[const static 1])
{
  free(buf->bs);
  buf->bs = nullptr;
  buf->len = 0;
  buf->capacity = 0;
}
