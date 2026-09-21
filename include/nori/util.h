#pragma once

#ifndef NDEBUG
#include <stdlib.h> // IWYU pragma: keep, needed for exit call
#endif

#include "nori/logger.h"
#include "nori/string.h"

enum nori_error {
  NORI_ERROR_NONE = 0,
  NORI_ERROR_NOMEM,
  NORI_ERROR_GENERIC = 999,
};

/// A representation of a success or failure.
struct nori_status {
  enum nori_error ns_error;
  struct nori_str_view ns_file;
  struct nori_str_view ns_line;
};

#define NORI_SUCCESS                                                                   \
  ((struct nori_status){.ns_error = NORI_ERROR_NONE,                                   \
                        .ns_file = NSV(__FILE__),                                      \
                        .ns_line = NSV(CPP_STR(__LINE__))})

#define NORI_FAILURE(err)                                                              \
  ((struct nori_status){                                                               \
      .ns_error = err, .ns_file = NSV(__FILE__), .ns_line = NSV(CPP_STR(__LINE__))})

#define NORI_FAILURE_EMIT(err, lvl, msg, ...)                                          \
  ({                                                                                   \
    auto status = NORI_FAILURE(err);                                                   \
    nori_log(lvl, msg __VA_OPT__(, ) __VA_ARGS__);                                     \
    status;                                                                            \
  })

#define NORI_ERROR_EMIT(err, msg, ...)                                                 \
  NORI_FAILURE_EMIT(err, NORI_LOG_LEVEL_ERROR, msg __VA_OPT__(, ) __VA_ARGS__)

#ifdef NDEBUG
#define nori_assert(condition, ...)
#else
#define nori_assert(condition, ...)                                                    \
  ({                                                                                   \
    if (!(condition)) {                                                                \
      nori_log_error(#condition " assertion failed");                                  \
      exit(1);                                                                         \
    }                                                                                  \
  })
#endif
