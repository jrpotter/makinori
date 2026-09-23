#pragma once

#include "nori/response.h"
#include "nori/string.h"

#ifndef NORI_REQUEST_MAX_PATH_LEN
#define NORI_REQUEST_MAX_PATH_LEN 2048
#endif

#ifndef NORI_REQUEST_MAX_CAPTURES
#define NORI_REQUEST_MAX_CAPTURES 16
#endif

/// A representation of supported HTTP methods.
enum nori_method {
  NORI_METHOD_GET,
};

/// An immutable HTTP request object.
///
/// Instances of this `struct` are created by the server and supplied to
/// user-registered callbacks defined in the `struct nori_route`.
struct nori_request {
  enum nori_method nr_method;
  char nr_path_[NORI_REQUEST_MAX_PATH_LEN];
  struct nori_str nr_path;
  struct nori_view nr_captures[NORI_REQUEST_MAX_CAPTURES];
};

/// User-supplied callback registered within a `struct nori_route` instance.
typedef struct nori_status
nori_route_callback_t(struct nori_request const, struct nori_response *const);

/// Route callback registration.
///
/// The user is expected to define an instance of this router with all of the
/// various paths they want to support. Paths are checked against each route
/// in order, according  to the nr_next field. On a match, the corresponding
/// nr_callback is invoked.
///
/// A basic example of two routes, one on `/static` and one on `/` is as follows:
///
/// ```c
/// static struct nori_route route_static = {
///     .nr_method = NORI_METHOD_GET,
///     .nr_path   = NSV("/static"),
///     .nr_next   = nullptr};
///
/// static struct nori_route route_root = {
///     .nr_method = NORI_METHOD_GET,
///     .nr_path   = NSV("/"),
///     .nr_next   = &route_static};
/// ```
///
/// In this case, the server checks against the root route first. If the request does
/// not match, it then checks against the static route.
struct nori_route {
  enum nori_method nr_method;
  struct nori_str nr_pattern;
  nori_route_callback_t *nr_callback;
  struct nori_route *nr_next;
};

struct nori_route const *const nori_route_match(
    struct nori_route const route[const static 1],
    struct nori_request req[static 1]);
