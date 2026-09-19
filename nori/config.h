#pragma once

#include "nori/util.h"

/// Server configuration parameters.
struct nori_config {
  unsigned long long nc_co_stack; // COROUTINE_STACK
  unsigned long nc_log;           // LOG_LEVEL
  unsigned long nc_port;          // PORT
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
