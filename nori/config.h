#pragma once

#include <lua5.4/lua.h>

#include "nori/util.h"

/// Server configuration parameters.
struct nori_config {
  unsigned long nc_log;  // The desired log level.
  unsigned long nc_port; // The port to serve HTTP requests on.
  lua_State *nc_state;   // Loaded configuration state.
};

/// Load the base configuration state.
///
/// This should be called even if no user-supplied configuration file at @path
/// is specified. It loads default state necessary for initializing the @out
/// argument.
///
/// @param path The path of a user-supplied configuration file. Ignored if empty.
/// @param out  The struct nori_config to initialize.
struct nori_status nori_config_load(
    const struct nori_str_view path,
    struct nori_config out[const static 1]);

/// Clean up the supplied `struct nori_config` instance.
struct nori_status nori_config_unload(struct nori_config config[const static 1]);
