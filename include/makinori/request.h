#pragma once

#include "makinori/response.h"
#include "makinori/runtime.h"
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

/// A representation of supported HTTP methods.
enum mn_method {
  MN_METHOD_GET,
};

/// An immutable HTTP request object.
///
/// Instances of this `struct` are created by the server and supplied to
/// user-registered callbacks defined in the `struct mn_route`.
struct mn_request {
  enum mn_method method;
  struct mn_str uri;
  struct mn_view path;
  MN_PAIR(struct mn_view, struct mn_view) query[MN_REQUEST_MAX_QUERY_PARAMS];
  struct mn_view captures[MN_REQUEST_MAX_CAPTURES];
  char buffer_[MN_REQUEST_MAX_PATH_LEN];
};

typedef struct mn_status
mn_route_handler_t(struct mn_request const, struct mn_response *const);

/// Route callback registration.
///
/// The user is expected to define an instance of this router with all of the
/// various paths they want to support. Paths are checked against each route in
/// order, according  to the next field. On a match, the corresponding handler
/// is invoked.
///
/// A basic example of two routes, one on `/static` and one on `/` is as follows:
///
/// ```c
/// static struct mn_route route_static = {
///     .method = MN_METHOD_GET,
///     .path   = mn_str_lit("/static"),
///     .next   = nullptr};
///
/// static struct mn_route route_root = {
///     .method = MN_METHOD_GET,
///     .path   = mn_str_lit("/"),
///     .next   = &route_static};
/// ```
///
/// In this case, the server checks against the root route first. If the request does
/// not match, it then checks against the static route.
struct mn_route {
  enum mn_method method;
  struct mn_str pattern;
  mn_route_handler_t *handler;
  struct mn_route *next;
};

struct mn_route const *const mn_route_match(
    mn_runtime_t *const runtime,
    struct mn_route const route[const static 1],
    struct mn_request req[static 1]);
