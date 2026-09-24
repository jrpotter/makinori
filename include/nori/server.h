#pragma once

#include "nori/config.h"
#include "nori/request.h"
#include "nori/runtime.h"

/// A representation of the server.
struct nori_server {
  nori_runtime_t *const ns_runtime;
  struct nori_config ns_config;
  struct nori_route ns_route;
};

/// Entrypoint to start the server.
///
/// Runs according to the configuration settings defined in server.config.
/// Serves requests according to the user-defined callbacks registered in
/// server.router.
struct nori_status nori_server_run(struct nori_server server[static 1]);
