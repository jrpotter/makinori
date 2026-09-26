#pragma once

#include "makinori/config.h"
#include "makinori/request.h"

/// A representation of the server.
struct mn_server {
  struct mn_config config;
  struct mn_route route;
};

/// Entrypoint to start the server.
///
/// Runs according to the configuration settings defined in server.config.
/// Serves requests according to the user-defined callbacks registered in
/// server.router.
struct mn_status mn_server_run(struct mn_server server[static 1]);
