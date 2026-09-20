#pragma once

#include <libwebsockets.h>

#include "nori/string.h"

#define CPP_PROXY(X) #X
#define CPP_STR(X) CPP_PROXY(X)

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

#define NORI_FAILURE_EMIT(lvl, msg, ...)                                               \
  ({                                                                                   \
    auto status = NORI_FAILURE;                                                        \
    _lws_log(                                                                          \
        lvl, ("%s:%s: " msg ""), status.ns_file.view,                                  \
        status.ns_line.view __VA_OPT__(, ) __VA_ARGS__);                               \
    status;                                                                            \
  })

#define NORI_FAILURE_WARN(msg, ...)                                                    \
  NORI_FAILURE_EMIT(LLL_WARN, msg __VA_OPT__(, ) __VA_ARGS__)

#define NORI_FAILURE_ERROR(msg, ...)                                                   \
  NORI_FAILURE_EMIT(LLL_ERR, msg __VA_OPT__(, ) __VA_ARGS__)

/// Initialize the nori state.
///
/// Generally speaking, this should be the first thing called in `main()`.
struct nori_status nori_init(void);
