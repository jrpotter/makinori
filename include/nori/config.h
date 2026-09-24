#pragma once

#include <lua5.4/lua.h>

#include "nori/util.h"

enum nori_event_loop {
  NORI_EVENT_LOOP_POLL,
};

/// Server configuration parameters.
struct nori_config {
  unsigned long long nc_co_pages;   // COROUTINE_PAGES
  enum nori_event_loop nc_ev_loop;  // EVENT_LOOP
  enum nori_log_level nc_log_level; // LOG_LEVEL
  unsigned long nc_port;            // PORT
};

/// Load the base configuration state.
///
/// This should be called even if no user-supplied configuration file at path
/// is specified. It loads default state necessary for initializing the out
/// argument.
///
/// @param path The path of a user-supplied configuration file. Ignored if empty.
/// @param out  The struct nori_config to initialize.
struct nori_status nori_config_load(
    lua_State *const lua,
    struct nori_str const path,
    struct nori_config out[const static 1]);
