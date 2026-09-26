#pragma once

#include "makinori/response.h"
#include "makinori/string.h"

#ifndef MN_REQUEST_MAX_PATH_LEN
#define MN_REQUEST_MAX_PATH_LEN 2048
#endif

static_assert(MN_REQUEST_MAX_PATH_LEN >= 256);

#ifndef MN_REQUEST_MAX_CAPTURES
#define MN_REQUEST_MAX_CAPTURES 24
#endif

static_assert(MN_REQUEST_MAX_CAPTURES >= 0);

#ifndef MN_REQUEST_MAX_QUERY_PARAMS
#define MN_REQUEST_MAX_QUERY_PARAMS 32
#endif

static_assert(MN_REQUEST_MAX_QUERY_PARAMS >= 0);

enum mn_method : unsigned int {
  MN_METHOD_GET = 0,
};

struct mn_query_param {
  struct mn_view key;
  struct mn_view value;
};

struct mn_request {
  enum mn_method method;
  struct mn_str uri;
  struct mn_view path;

  struct mn_query_param query[MN_REQUEST_MAX_QUERY_PARAMS];
  size_t query_count;

  struct mn_view captures[MN_REQUEST_MAX_CAPTURES];
  size_t capture_count;

  char buffer_[MN_REQUEST_MAX_PATH_LEN];
};

typedef struct mn_status
mn_route_handler_t(struct mn_request const, struct mn_response *const);

struct mn_route {
  enum mn_method method;
  struct mn_str pattern;
  mn_route_handler_t *handler;
  struct mn_route *next;
};
