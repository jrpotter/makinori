#pragma once

#include "makinori/config.h"
#include "makinori/response.h"

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
