#include <libwebsockets.h>
#include <stdarg.h>

#include "nori/logger.h"

// We map TRACE to THREAD because both happen to start with a 'T'.
#define LLL_TRACE LLL_THREAD

void nori_log_set_level(enum nori_log_level const value)
{
  int lws_level = 0;

  if (value <= NORI_LOG_LEVEL_ERROR) {
    lws_level |= LLL_ERR;
  }
  if (value <= NORI_LOG_LEVEL_WARN) {
    lws_level |= LLL_WARN;
  }
  if (value <= NORI_LOG_LEVEL_NOTICE) {
    lws_level |= LLL_NOTICE;
  }
  if (value <= NORI_LOG_LEVEL_INFO) {
    lws_level |= LLL_INFO;
  }
  if (value <= NORI_LOG_LEVEL_DEBUG) {
    lws_level |= LLL_DEBUG;
  }

  // Always include for tracing purposes.
  lws_level |= LLL_TRACE;

  lws_set_log_level(lws_level, nullptr);
}

void nori_log_(enum nori_log_level const level, char const *const msg, ...)
{
  va_list ap = {};
  va_start(ap, msg);
  switch (level) {
  case NORI_LOG_LEVEL_DEBUG: {
    _lws_logv(LLL_DEBUG, msg, ap);
    break;
  }
  case NORI_LOG_LEVEL_INFO: {
    _lws_logv(LLL_INFO, msg, ap);
    break;
  }
  case NORI_LOG_LEVEL_NOTICE: {
    _lws_logv(LLL_NOTICE, msg, ap);
    break;
  }
  case NORI_LOG_LEVEL_WARN: {
    _lws_logv(LLL_WARN, msg, ap);
    break;
  }
  case NORI_LOG_LEVEL_ERROR: {
    _lws_logv(LLL_ERR, msg, ap);
    break;
  }
  }
  va_end(ap);
}

void nori_trace_(bool tag, char const *const msg, ...)
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
