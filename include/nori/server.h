#pragma once

#include "nori/config.h"
#include "nori/request.h"

/// A representation of the server.
struct nori_server {
  struct nori_config config;
  struct nori_route route;
};

/// Entrypoint to start the server.
///
/// Runs according to the configuration settings defined in server.config.
/// Serves requests according to the user-defined callbacks registered in
/// server.router.
struct nori_status nori_server_run(struct nori_server server[static 1]);
