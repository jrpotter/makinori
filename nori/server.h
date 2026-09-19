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
/// user-registered callbacks defined in the `struct nori_route`.
struct nori_request {
  enum nori_method nr_method;
  struct nori_str_view nr_path;
};

enum nori_header {
  NORI_HEADER_CONTENT_LENGTH,
  NORI_HEADER_CONTENT_TYPE,
};

/// A mutable HTTP response object.
///
/// Instances of this `struct` are created by the server and supplied to
/// user-registered callbacks defined in the `struct nori_route`. The user
/// is responsible for updating the response in the callback function.
struct nori_response;

/// Set an HTTP header value.
struct nori_status nori_response_set_header(
    struct nori_response *const,
    enum nori_header header,
    struct nori_str_view value);

/// Set the HTTP status return code.
struct nori_status
nori_response_set_status(struct nori_response *const, unsigned int status);

/// User-supplied callback registered within a `struct nori_route` instance.
typedef struct nori_status
nori_route_callback_t(const struct nori_request, struct nori_response *const);

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
  struct nori_str_view nr_path;
  nori_route_callback_t *nr_callback;
  struct nori_route *nr_next;
};

/// A representation of the server.
struct nori_server {
  struct nori_config config;
  struct nori_route router;
};

/// Entrypoint to start the server.
///
/// Runs according to the configuration settings defined in @server.config.
/// Serves requests according to the user-defined callbacks registered in
/// @server.router.
struct nori_status nori_server_run(struct nori_server server[static 1]);
