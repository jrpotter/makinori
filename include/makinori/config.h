#pragma once

#include "makinori/runtime.h"
#include "makinori/util.h"

enum mn_event_loop {
  MN_EVENT_LOOP_POLL,
};

/// Server configuration parameters.
struct mn_config {
  unsigned long long coro_pages; // COROUTINE_PAGES
  enum mn_event_loop ev_loop;    // EVENT_LOOP
  enum mn_log_level log_level;   // LOG_LEVEL
  unsigned long port;            // PORT
};

struct mn_status
mn_config_load(mn_runtime_t *const runtime, struct mn_config out[const static 1]);

/// Load the base configuration state.
///
/// This should be called even if no user-supplied configuration file at path
/// is specified. It loads default state necessary for initializing the out
/// argument.
///
/// @param path The path of a user-supplied configuration file. Ignored if empty.
/// @param out  The struct mn_config to initialize.
struct mn_status mn_config_load_with(
    struct mn_str const path,
    mn_runtime_t *const runtime,
    struct mn_config out[const static 1]);
