#pragma once

#include <lua5.4/lua.h>

#include "nori/util.h"

/**
 * Server configuration parameters.
 */
struct nori_config {
  unsigned long nc_log;
  unsigned long nc_port;
  lua_State *nc_state;
};

/**
 * Log level used prior to loading configuration file.
 */
constexpr unsigned long INIT_LOG_LEVEL = LLL_INFO | LLL_NOTICE | LLL_WARN | LLL_ERR;

/**
 * Attempt to load a configuration file at path.
 *
 * @param path The path of a user-supplied configuration file. Ignored if length is 0.
 * @param out  The struct nori_config to initialize.
 */
struct nori_status
nori_config_load(const struct nori_view path, struct nori_config out[const static 1]);

/**
 * To call once finished using a struct nori_config.
 */
struct nori_status nori_config_unload(struct nori_config config[const static 1]);
