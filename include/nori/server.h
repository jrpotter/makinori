#pragma once

#include "nori/config.h"
#include "nori/request.h"
#include "nori/response.h"

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
/// Runs according to the configuration settings defined in server.config.
/// Serves requests according to the user-defined callbacks registered in
/// server.router.
struct nori_status nori_server_run(struct nori_server server[static 1]);
