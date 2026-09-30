#include <libwebsockets.h>
#include <stdarg.h>

#include "makinori/logger.h"

// Relate together because they start with the same letter.
#define LLL_PERROR LLL_PARSER
#define LLL_TRACE LLL_THREAD

void mn_log_set_level(enum mn_log_level const value)
{
  int lws_level = LLL_PERROR | LLL_TRACE;

  if (value <= MN_LOG_LEVEL_ERROR) {
    lws_level |= LLL_ERR;
  }
  if (value <= MN_LOG_LEVEL_WARN) {
    lws_level |= LLL_WARN;
  }
  if (value <= MN_LOG_LEVEL_NOTICE) {
    lws_level |= LLL_NOTICE;
  }
  if (value <= MN_LOG_LEVEL_INFO) {
    lws_level |= LLL_INFO;
  }
  if (value <= MN_LOG_LEVEL_DEBUG) {
    lws_level |= LLL_DEBUG;
  }

  lws_set_log_level(lws_level, nullptr);
}

void mn_log_(enum mn_log_level const level, char const *const msg, ...)
{
  va_list ap = {};
  va_start(ap, msg);
  switch (level) {
  case MN_LOG_LEVEL_DEBUG: {
    _lws_logv(LLL_DEBUG, msg, ap);
    break;
  }
  case MN_LOG_LEVEL_INFO: {
    _lws_logv(LLL_INFO, msg, ap);
    break;
  }
  case MN_LOG_LEVEL_NOTICE: {
    _lws_logv(LLL_NOTICE, msg, ap);
    break;
  }
  case MN_LOG_LEVEL_WARN: {
    _lws_logv(LLL_WARN, msg, ap);
    break;
  }
  case MN_LOG_LEVEL_ERROR: {
    _lws_logv(LLL_ERR, msg, ap);
    break;
  }
  case MN_LOG_LEVEL_OFF: {
    // Do nothing
    break;
  }
  }
  va_end(ap);
}

void mn_trace_(bool tag, char const *const msg, ...)
{
#ifndef NDEBUG
  if (tag) {
    va_list ap = {};
    va_start(ap, msg);
    _lws_logv(LLL_TRACE, msg, ap);
    va_end(ap);
  }
#endif
}

void mn_perror_(char const *const msg, ...)
{
  va_list ap = {};
  va_start(ap, msg);
  _lws_logv(LLL_PERROR, msg, ap);
  va_end(ap);
}
