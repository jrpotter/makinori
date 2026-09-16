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

#define NORI_FAILURE_EMIT(lvl, msg, ...)                                               \
  ({                                                                                   \
    auto status = NORI_FAILURE;                                                        \
    _lws_log(                                                                          \
        lvl, ("%s:%s: " msg ""), status.file.ss,                                       \
        status.line.ss __VA_OPT__(, ) __VA_ARGS__);                                    \
    status;                                                                            \
  })

#define NORI_FAILURE_WARN(msg, ...)                                                    \
  NORI_FAILURE_EMIT(LLL_WARN, msg __VA_OPT__(, ) __VA_ARGS__)

#define NORI_FAILURE_ERROR(msg, ...)                                                   \
  NORI_FAILURE_EMIT(LLL_ERR, msg __VA_OPT__(, ) __VA_ARGS__)

#define nori_log_error(msg, ...) lwsl_err(msg __VA_OPT__(, ) __VA_ARGS__)
#define nori_log_warn(msg, ...) lwsl_warn(msg __VA_OPT__(, ) __VA_ARGS__)
#define nori_log_notice(msg, ...) lwsl_notice(msg __VA_OPT__(, ) __VA_ARGS__)
#define nori_log_info(msg, ...) lwsl_info(msg __VA_OPT__(, ) __VA_ARGS__)
#define nori_log_debug(msg, ...) lwsl_debug(msg __VA_OPT__(, ) __VA_ARGS__)
#define nori_set_log_level(lvl) (lws_set_log_level(lvl, nullptr))
