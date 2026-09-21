#pragma once

#ifndef NDEBUG
#include <stdlib.h> // IWYU pragma: keep, needed for exit call
#endif

#include "nori/logger.h"
#include "nori/string.h"

/// A representation of a success or failure.
struct nori_status {
  bool ns_success;
  struct nori_str_view ns_file;
  struct nori_str_view ns_line;
};

#define NORI_SUCCESS                                                                   \
  ((struct nori_status){.ns_success = true,                                            \
                        .ns_file = NSV(__FILE__),                                      \
                        .ns_line = NSV(CPP_STR(__LINE__))})

#define NORI_FAILURE                                                                   \
  ((struct nori_status){.ns_success = false,                                           \
                        .ns_file = NSV(__FILE__),                                      \
                        .ns_line = NSV(CPP_STR(__LINE__))})

#define NORI_FAILURE_EMIT(lvl, msg, ...)                                               \
  ({                                                                                   \
    auto status = NORI_FAILURE;                                                        \
    nori_log(lvl, msg __VA_OPT__(, ) __VA_ARGS__);                                     \
    status;                                                                            \
  })

#define NORI_FAILURE_WARN(msg, ...)                                                    \
  NORI_FAILURE_EMIT(NORI_LOG_LEVEL_WARN, msg __VA_OPT__(, ) __VA_ARGS__)

#define NORI_FAILURE_ERROR(msg, ...)                                                   \
  NORI_FAILURE_EMIT(NORI_LOG_LEVEL_ERROR, msg __VA_OPT__(, ) __VA_ARGS__)

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
