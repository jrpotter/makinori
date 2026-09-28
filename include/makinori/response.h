#pragma once

#include "makinori/string.h"

struct mn_response;

struct mn_status mn_response_suspend(struct mn_response *const res);

// =================================================================================
// Status Codes

enum mn_http_code : unsigned int {
  MN_HTTP_CONTINUE = 100,
  MN_HTTP_SWITCHING_PROTOCOLS = 101,
  MN_HTTP_PROCESSING = 102,
  MN_HTTP_EARLY_HINTS = 103,

  MN_HTTP_CODE_OK = 200,
  MN_HTTP_CODE_CREATED = 201,
  MN_HTTP_ACCEPTED = 202,
  MN_HTTP_NON_AUTH_INFO = 203,
  MN_HTTP_NO_CONTENT = 204,
  MN_HTTP_RESET_CONTENT = 205,
  MN_HTTP_PARTIAL_CONTENT = 206,
  MN_HTTP_MULTI_STATUS = 207,
  MN_HTTP_ALREADY_REPORTED = 208,
  MN_HTTP_IM_USED = 226,

  MN_HTTP_MULTIPLE_CHOICES = 300,
  MN_HTTP_MOVED_PERMANENTLY = 301,
  MN_HTTP_FOUND = 302,
  MN_HTTP_SEE_OTHER = 303,
  MN_HTTP_NOT_MODIFIED = 304,
  MN_HTTP_USE_PROXY = 305,
  MN_HTTP_TEMPORARY_REDIRECT = 307,
  MN_HTTP_PERMANENT_REDIRECT = 308,

  MN_HTTP_BAD_REQUEST = 400,
  MN_HTTP_UNAUTHORIZED = 401,
  MN_HTTP_PAYMENT_REQUIRED = 402,
  MN_HTTP_FORBIDDEN = 403,
  MN_HTTP_NOT_FOUND = 404,
  MN_HTTP_METHOD_NOT_ALLOWED = 405,
  MN_HTTP_NOT_ACCEPTABLE = 406,
  MN_HTTP_PROXY_AUTH_REQUIRED = 407,
  MN_HTTP_REQUEST_TIMEOUT = 408,
  MN_HTTP_CONFLICT = 409,
  MN_HTTP_GONE = 410,
  MN_HTTP_LENGTH_REQUIRED = 411,
  MN_HTTP_PRECONDITION_FAILED = 412,
  MN_HTTP_CONTENT_TOO_LARGE = 413,
  MN_HTTP_URI_TOO_LONG = 414,
  MN_HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
  MN_HTTP_RANGE_NOT_SATISFIABLE = 416,
  MN_HTTP_EXPECTATION_FAILED = 417,
  MN_HTTP_IM_A_TEAPOT = 418,
  MN_HTTP_MISDIRECTED_REQUEST = 421,
  MN_HTTP_UNPROCESSABLE_CONTENT = 422,
  MN_HTTP_LOCKED = 423,
  MN_HTTP_FAILED_DEPENDENCY = 424,
  MN_HTTP_TOO_EARLY = 425,
  MN_HTTP_UPGRADE_REQUIRED = 426,
  MN_HTTP_PRECONDITION_REQUIRED = 428,
  MN_HTTP_TOO_MANY_REQUESTS = 429,
  MN_HTTP_FIELDS_TOO_LARGE = 432,
  MN_HTTP_UNAVAILABLE_LEGAL = 451,

  MN_HTTP_INTERNAL_SERVER_ERROR = 500,
  MN_HTTP_NOT_IMPLEMENTED = 501,
  MN_HTTP_BAD_GATEWAY = 502,
  MN_HTTP_SERVICE_UNAVAILABLE = 503,
  MN_HTTP_GATEWAY_TIMEOUT = 504,
  MN_HTTP_VERSION_NOT_SUPPORTED = 505,
  MN_HTTP_VARIANT_ALSO_NEGOTIATES = 506,
  MN_HTTP_INSUFFICIENT_STORAGE = 507,
  MN_HTTP_LOOP_DETECTED = 508,
  MN_HTTP_NOT_EXTENDED = 510,
  MN_HTTP_NETWORK_AUTH_REQUIRED = 511,
};

// =================================================================================
// Header

struct mn_status
mn_response_set_code(struct mn_response *const, enum mn_http_code code);

struct mn_status mn_response_set_header(
    struct mn_response *const res,
    struct mn_str header,
    struct mn_str value);

// =================================================================================
// Body

struct mn_status
mn_response_write(struct mn_response *const res, struct mn_str const content);

struct mn_status
mn_response_write_file(struct mn_response *const res, struct mn_str const path);

struct mn_status mn_response_write_buffer(
    struct mn_response *const res,
    char const buffer[const static 1],
    size_t const len);
