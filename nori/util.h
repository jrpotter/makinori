#pragma once

#include <libwebsockets.h>

#include "nori/string.h"

#define nori_set_log_level(lvl) (lws_set_log_level(lvl, nullptr))

#define nori_log(lvl, msg, ...)                                                        \
  _lws_log(                                                                            \
      lvl, ("[%s:%s] " msg ""), __FILE__,                                              \
      CPP_STR(__LINE__) __VA_OPT__(, ) __VA_ARGS__)

#define nori_log_error(msg, ...) nori_log(LLL_ERR, msg __VA_OPT__(, ) __VA_ARGS__)
#define nori_log_warn(msg, ...) nori_log(LLL_WARN, msg __VA_OPT__(, ) __VA_ARGS__)
#define nori_log_notice(msg, ...) nori_log(LLL_NOTICE, msg __VA_OPT__(, ) __VA_ARGS__)
#define nori_log_info(msg, ...) nori_log(LLL_INFO, msg __VA_OPT__(, ) __VA_ARGS__)
#define nori_log_debug(msg, ...) nori_log(LLL_DEBUG, msg __VA_OPT__(, ) __VA_ARGS__)

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
        lvl, ("[%s:%s] " msg ""), __FILE__,                                            \
        CPP_STR(__LINE__) __VA_OPT__(, ) __VA_ARGS__);                                 \
    status;                                                                            \
  })

#define NORI_FAILURE_WARN(msg, ...)                                                    \
  NORI_FAILURE_EMIT(LLL_WARN, msg __VA_OPT__(, ) __VA_ARGS__)

#define NORI_FAILURE_ERROR(msg, ...)                                                   \
  NORI_FAILURE_EMIT(LLL_ERR, msg __VA_OPT__(, ) __VA_ARGS__)
