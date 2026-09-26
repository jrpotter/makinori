#pragma once

#ifndef NDEBUG
#include <stdlib.h> // IWYU pragma: keep, needed for abort
#endif

#include "makinori/logger.h"
#include "makinori/macro.h"
#include "makinori/string.h"

enum mn_error : unsigned int {
  MN_ERROR_NONE = 0,

  // User facing errors
  MN_ERROR_CONFIG = 1,
  MN_ERROR_INVALID_ARG = 2,
  MN_ERROR_IMMUTABLE = 3,
  MN_ERROR_DUPLICATE = 4,

  // System errors
  MN_ERROR_SYSTEM = 900,
  MN_ERROR_NOMEM = 901,
};

struct mn_status {
  enum mn_error error;
  struct mn_str file;
  struct mn_str line;
};

#define MN_SUCCESS                                                                     \
  ((struct mn_status){.error = MN_ERROR_NONE,                                          \
                      .file = mn_str_lit(__FILE__),                                    \
                      .line = mn_str_lit(MN_STR_TO(__LINE__))})

#define MN_FAILURE(err)                                                                \
  ({                                                                                   \
    static_assert(err > MN_ERROR_NONE, "Did you mean to use MN_SUCCESS?");             \
    ((struct mn_status){.error = err,                                                  \
                        .file = mn_str_lit(__FILE__),                                  \
                        .line = mn_str_lit(MN_STR_TO(__LINE__))});                     \
  })

#define MN_FAILURE_EMIT(err, lvl, msg, ...)                                            \
  ({                                                                                   \
    auto status = MN_FAILURE(err);                                                     \
    mn_log(lvl, msg __VA_OPT__(, ) __VA_ARGS__);                                       \
    status;                                                                            \
  })

#define MN_WARN_EMIT(err, msg, ...)                                                    \
  MN_FAILURE_EMIT(err, MN_LOG_LEVEL_WARN, msg __VA_OPT__(, ) __VA_ARGS__)

#define MN_ERROR_EMIT(err, msg, ...)                                                   \
  MN_FAILURE_EMIT(err, MN_LOG_LEVEL_ERROR, msg __VA_OPT__(, ) __VA_ARGS__)

#ifdef NDEBUG
#define mn_assert(condition)
#else
#define mn_assert(condition)                                                           \
  ({                                                                                   \
    if (!(condition)) {                                                                \
      mn_log_error(#condition " assertion failed");                                    \
      abort();                                                                         \
    }                                                                                  \
  })
#endif
