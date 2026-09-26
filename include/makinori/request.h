#pragma once

#include "makinori/config.h"
#include "makinori/response.h"
#include "makinori/string.h"

#ifndef MN_REQUEST_MAX_PATH_LEN
#define MN_REQUEST_MAX_PATH_LEN 2048
#endif

static_assert(MN_REQUEST_MAX_PATH_LEN >= 256);

#ifndef MN_REQUEST_MAX_CAPTURES
#define MN_REQUEST_MAX_CAPTURES 16
#endif

static_assert(MN_REQUEST_MAX_CAPTURES >= 0);

#ifndef MN_REQUEST_MAX_QUERY_PARAMS
#define MN_REQUEST_MAX_QUERY_PARAMS 16
#endif

static_assert(MN_REQUEST_MAX_QUERY_PARAMS >= 0);

enum mn_method {
  MN_METHOD_GET,
};

struct mn_request {
  enum mn_method method;
  struct mn_str uri;
  struct mn_view path;

  MN_PAIR(struct mn_view, struct mn_view) query[MN_REQUEST_MAX_QUERY_PARAMS];
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

struct mn_route const *const mn_route_match(
    struct mn_config const config[const static 1],
    struct mn_route const route[const static 1],
    struct mn_request req[static 1]);
