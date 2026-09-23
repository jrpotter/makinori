#pragma once

#ifndef NDEBUG
#include <stdlib.h> // IWYU pragma: keep, needed for exit call
#endif

#include "nori/logger.h"
#include "nori/string.h"

enum nori_error {
  NORI_ERROR_NONE = 0,

  // User facing errors. These happen because of user error.
  NORI_ERROR_CONFIG = 1,
  NORI_ERROR_INVALID_ARG,
  NORI_ERROR_IMMUTABLE,
  NORI_ERROR_DUPLICATE,

  // System errors. These happen because of an internal error.
  NORI_ERROR_SYSTEM = 900,
  NORI_ERROR_NOMEM,
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
  ({                                                                                   \
    static_assert(err > NORI_ERROR_NONE, "Did you mean to use NORI_SUCCESS?");         \
    ((struct nori_status){.ns_error = err,                                             \
                          .ns_file = NSV(__FILE__),                                    \
                          .ns_line = NSV(CPP_STR(__LINE__))});                         \
  })

#define NORI_FAILURE_EMIT(err, lvl, msg, ...)                                          \
  ({                                                                                   \
    auto status = NORI_FAILURE(err);                                                   \
    nori_log(lvl, msg __VA_OPT__(, ) __VA_ARGS__);                                     \
    status;                                                                            \
  })

#define NORI_WARN_EMIT(err, msg, ...)                                                  \
  NORI_FAILURE_EMIT(err, NORI_LOG_LEVEL_WARN, msg __VA_OPT__(, ) __VA_ARGS__)

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
