#pragma once

#include "nori/config.h"

/// A representation of supported HTTP methods.
enum nori_method {
  NORI_METHOD_GET,
  NORI_METHOD_POST,
};

/// An immutable HTTP request object.
///
/// Instances of this `struct` are created by the server and supplied to
/// user-registered callbacks defined in the `struct nori_router`.
struct nori_request {
  enum nori_method nr_method; // The HTTP method.
  struct nori_slice nr_path;  // The full path component.
};

/// A mutable HTTP response object.
///
/// Instances of this `struct` are created by the server and supplied to
/// user-registered callbacks defined in the `struct nori_router`. Unlike the
/// `struct nori_request`, the user is expected to update fields as they see
/// fit. The server is responsible for then managing the updated response.
struct nori_response {
  unsigned int nr_status;            // The HTTP status code.
  struct nori_slice nr_content_type; // TODO: Need dynamic content.
};

/// User-supplied callback registered within a `struct nori_router` instance.
typedef struct nori_status
nori_router_callback_t(const struct nori_request, struct nori_response *const);

/// Route callback registration.
///
/// The user is expected to define an instance of this router with all of the
/// various paths they want to support. Paths are checked against each route
/// in order, according  to the @nr_next field. On a match, the corresponding
/// @nr_callback is invoked.
///
/// A basic example of two routes, one on `/static` and one on `/` is as follows:
///
/// ```c
/// static struct nori_router route_static = {
///     .nr_method = NORI_METHOD_GET,
///     .nr_path   = SS("/static"),
///     .nr_next   = nullptr};
///
/// static struct nori_router route_root = {
///     .nr_method = NORI_METHOD_GET,
///     .nr_path   = SS("/"),
///     .nr_next   = &route_static};
/// ```
///
/// In this case, the server checks against the root route first. If the request does
/// not match, it then checks against the static route.
struct nori_router {
  enum nori_method nr_method;
  struct nori_slice nr_path;
  nori_router_callback_t *nr_callback;
  struct nori_router *nr_next;
};

/// A representation of the server.
struct nori_server {
  struct nori_config config;
  struct nori_router router;
};

struct nori_status nori_server_run(struct nori_server server[static 1]);
