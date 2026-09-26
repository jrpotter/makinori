#pragma once

#include "makinori/util.h"

typedef struct lua_State mn_lua_t;

enum mn_event_loop : unsigned int {
  MN_EVENT_LOOP_POLL,
};

struct mn_config {
  unsigned long long coro_pages; // COROUTINE_PAGES
  enum mn_event_loop ev_loop;    // EVENT_LOOP
  enum mn_log_level log_level;   // LOG_LEVEL
  unsigned long port;            // PORT
  mn_lua_t *lua_;
};

struct mn_status mn_config_load(struct mn_config out[const static 1]);

struct mn_status
mn_config_load_with(struct mn_str const path, struct mn_config out[const static 1]);

void mn_config_unload(struct mn_config[const static 1]);
