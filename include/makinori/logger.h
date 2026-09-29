#pragma once

#include <errno.h>

#include "makinori/macro.h"

#ifndef MN_TRACE_LWS_CALLBACK
#define MN_TRACE_LWS_CALLBACK false
#endif

#ifndef MN_TRACE_RESPONSE_STATE
#define MN_TRACE_RESPONSE_STATE false
#endif

enum mn_log_level : unsigned int {
  MN_LOG_LEVEL_DEBUG = 0,
  MN_LOG_LEVEL_INFO = 1,
  MN_LOG_LEVEL_NOTICE = 2,
  MN_LOG_LEVEL_WARN = 3,
  MN_LOG_LEVEL_ERROR = 4,
};

/// Set the per-thread log level. Lower level logs are not output.
void mn_log_set_level(enum mn_log_level const);

/// Emit a log at the specified level.
[[gnu::__format__(__printf__, 2, 3)]]
void mn_log_(enum mn_log_level const level, char const *const msg, ...);

#define mn_log(level, msg, ...)                                                        \
  mn_log_(                                                                             \
      (level),                                                                         \
      (__FILE__ ":" MN_STR_TO(__LINE__) ": " msg "")__VA_OPT__(, ) __VA_ARGS__)

#define mn_log_debug(msg, ...)                                                         \
  mn_log(MN_LOG_LEVEL_DEBUG, msg __VA_OPT__(, ) __VA_ARGS__)

#define mn_log_info(msg, ...) mn_log(MN_LOG_LEVEL_INFO, msg __VA_OPT__(, ) __VA_ARGS__)

#define mn_log_notice(msg, ...)                                                        \
  mn_log(MN_LOG_LEVEL_NOTICE, msg __VA_OPT__(, ) __VA_ARGS__)

#define mn_log_warn(msg, ...) mn_log(MN_LOG_LEVEL_WARN, msg __VA_OPT__(, ) __VA_ARGS__)

#define mn_log_error(msg, ...)                                                         \
  mn_log(MN_LOG_LEVEL_ERROR, msg __VA_OPT__(, ) __VA_ARGS__)

[[gnu::__format__(__printf__, 2, 3)]]
void mn_trace_(bool tag, char const *const msg, ...);

#define mn_trace(tag, msg, ...)                                                        \
  mn_trace_(                                                                           \
      (tag), (__FILE__ ":" MN_STR_TO(__LINE__) ": " msg "")__VA_OPT__(, ) __VA_ARGS__)

/// A leaner and more consistently formatted perror alternative.
[[gnu::__format__(__printf__, 1, 2)]]
void mn_perror_(char const *const msg, ...);

#define mn_perror(msg, ...)                                                            \
  mn_perror_(                                                                          \
      (__FILE__ ":" MN_STR_TO(__LINE__) ": [errno:%d] " msg ""),                       \
      errno __VA_OPT__(, ) __VA_ARGS__)
