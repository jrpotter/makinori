#pragma once

#define CPP_PROXY(X) #X
#define CPP_STR(X) CPP_PROXY(X)

// Toggle on for debugging
#define NORI_TRACE_LWS_CALLBACK false
#define NORI_TRACE_RESPONSE_STATE false

enum nori_log_level {
  NORI_LOG_LEVEL_DEBUG = 0,
  NORI_LOG_LEVEL_INFO = 1,
  NORI_LOG_LEVEL_NOTICE = 2,
  NORI_LOG_LEVEL_WARN = 3,
  NORI_LOG_LEVEL_ERROR = 4,
};

/// Set the per-thread log level. Lower level logs are not output.
void nori_log_set_level(const enum nori_log_level);

/// Emit a log at the specified level.
[[gnu::__format__(__printf__, 2, 3)]]
void _nori_log(enum nori_log_level const level, char const *const msg, ...);

#define nori_log(level, msg, ...)                                                      \
  _nori_log(                                                                           \
      (level), (__FILE__ ":" CPP_STR(__LINE__) ": " msg "")__VA_OPT__(, ) __VA_ARGS__)

#define nori_log_debug(msg, ...)                                                       \
  nori_log(NORI_LOG_LEVEL_DEBUG, msg __VA_OPT__(, ) __VA_ARGS__)

#define nori_log_info(msg, ...)                                                        \
  nori_log(NORI_LOG_LEVEL_INFO, msg __VA_OPT__(, ) __VA_ARGS__)

#define nori_log_notice(msg, ...)                                                      \
  nori_log(NORI_LOG_LEVEL_NOTICE, msg __VA_OPT__(, ) __VA_ARGS__)

#define nori_log_warn(msg, ...)                                                        \
  nori_log(NORI_LOG_LEVEL_WARN, msg __VA_OPT__(, ) __VA_ARGS__)

#define nori_log_error(msg, ...)                                                       \
  nori_log(NORI_LOG_LEVEL_ERROR, msg __VA_OPT__(, ) __VA_ARGS__)

/// Emits a log message.
///
/// Unlike the level-specific logging functions, this ignores the current log level.
/// Instead, application code is expected to introduce defines to toggle the log.
/// For example:
///
/// ```c
/// #define TRACE_REQUEST true
/// ...
/// nori_trace(TRACE_REQUEST, "Received request w/id %d", id);
/// ```
///
/// Traces are not output if NDEBUG is true.
///
/// @param tag - Whether the trace should be enabled or not.
[[gnu::__format__(__printf__, 2, 3)]]
void _nori_trace(bool tag, char const *const msg, ...);

#define nori_trace(tag, msg, ...)                                                      \
  _nori_trace(                                                                         \
      (tag), (__FILE__ ":" CPP_STR(__LINE__) ": " msg "")__VA_OPT__(, ) __VA_ARGS__)
