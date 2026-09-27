#pragma once

#include "makinori/config.h"
#include "makinori/request.h"

struct mn_server {
  struct mn_config config;
  struct mn_route route;
};

struct mn_status mn_server_run(struct mn_server s[static 1]);
