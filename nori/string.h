#pragma once

#include <stddef.h>
#include <stdint.h>

#include "nori/util.h"

// ================================================================
// Strings
// ================================================================

/**
 * A dynamically allocated string.
 */
struct nori_str {
  uint8_t *ss;
  size_t len;
  size_t capacity;
};

struct nori_status
nori_str_create(size_t capacity, struct nori_str out[const static 1]);

void nori_str_destroy(struct nori_str[const static 1]);

// ================================================================
// Bytes
// ================================================================

/**
 * A dynamically allocated buffer.
 */
struct nori_buf {
  uint8_t *bs;
  size_t len;
  size_t capacity;
};

struct nori_status
nori_bytes_create(size_t capacity, struct nori_buf out[const static 1]);

void nori_bytes_destroy(struct nori_buf[const static 1]);
