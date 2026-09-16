#pragma once

#include <libwebsockets.h>

#include "nori/string.h"

struct nori_status {
  bool success;
  struct nori_str file;
  struct nori_str line;
};

#define NORI_SUCCESS                                                                   \
  ((struct nori_status){.success = true,                                               \
                        .file = NORI_STR(__FILE__),                                    \
                        .line = NORI_STR(CPP_STR(__LINE__))})

#define NORI_FAILURE                                                                   \
  ((struct nori_status){.success = false,                                              \
                        .file = NORI_STR(__FILE__),                                    \
                        .line = NORI_STR(CPP_STR(__LINE__))})

#define NORI_FAILURE_EMIT(msg, lvl, ...)                                               \
  ({                                                                                   \
    auto status = NORI_FAILURE;                                                        \
    _lwsl_log(                                                                         \
        lvl, ("%s:%s: " msg ""), status.file.ss,                                       \
        status.line.ss __VA_OPT__(, ) __VA_ARGS__);                                    \
    status;                                                                            \
  })

#define NORI_FAILURE_WARN(msg, ...)                                                    \
  NORI_FAILURE_EMIT(msg, LLL_WARN __VA_OPT(, ) __VA_ARGS__)

#define NORI_FAILURE_ERROR(msg, ...)                                                   \
  NORI_FAILURE_EMIT(msg, LLL_ERR __VA_OPT(, ) __VA_ARGS__)
