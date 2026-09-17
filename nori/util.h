#pragma once

#include <libwebsockets.h>

#define CPP_PROXY(X) #X
#define CPP_STR(X) CPP_PROXY(X)

// ================================================================
// Views
// ================================================================

/**
 * An immutable representation of a string.
 *
 * The ss pointer must remain valid while an instance is in use.
 */
struct nori_view {
  const char *ss;
  size_t len;
};

#define NORI_VIEW(X)                                                                   \
  ((struct nori_view){.ss = ("" X ""), .len = (sizeof(X) / sizeof(X[0])) - 1})

/**
 * Wrapper around a C string.
 */
struct nori_view nori_view_create(const char ss[static 1]);

/**
 * Check if two strings are equal by value.
 *
 * @return true if s1 and s2 are equal.
 */
bool nori_view_eq(const struct nori_view s1, const struct nori_view s2);

// ================================================================
// Logging
// ================================================================

/**
 * Log level used prior to loading configuration file.
 */
constexpr unsigned long INIT_LOG_LEVEL = LLL_INFO | LLL_NOTICE | LLL_WARN | LLL_ERR;

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
  struct nori_view file;
  struct nori_view line;
};

#define NORI_SUCCESS                                                                   \
  ((struct nori_status){.success = true,                                               \
                        .file = NORI_VIEW(__FILE__),                                   \
                        .line = NORI_VIEW(CPP_STR(__LINE__))})

#define NORI_FAILURE                                                                   \
  ((struct nori_status){.success = false,                                              \
                        .file = NORI_VIEW(__FILE__),                                   \
                        .line = NORI_VIEW(CPP_STR(__LINE__))})

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

struct nori_status nori_init(void);
