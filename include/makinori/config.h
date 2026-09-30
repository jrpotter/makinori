#pragma once

#include "makinori/util.h"

#ifndef MN_CMDLINE_MAX_FLAGS
#define MN_CMDLINE_MAX_FLAGS 16
#endif

#ifndef MN_CMDLINE_MAX_ARITY
#define MN_CMDLINE_MAX_ARITY 8
#endif

#ifndef MN_REQUEST_MAX_PATH_LEN
#define MN_REQUEST_MAX_PATH_LEN 2048
#endif

#ifndef MN_REQUEST_MAX_CAPTURES
#define MN_REQUEST_MAX_CAPTURES 24
#endif

#ifndef MN_REQUEST_MAX_QUERY_PARAMS
#define MN_REQUEST_MAX_QUERY_PARAMS 32
#endif

static_assert(MN_CMDLINE_MAX_FLAGS >= 1);
static_assert(MN_CMDLINE_MAX_ARITY >= 1);
static_assert(MN_REQUEST_MAX_PATH_LEN >= 256);
static_assert(MN_REQUEST_MAX_CAPTURES >= 1);
static_assert(MN_REQUEST_MAX_QUERY_PARAMS >= 1);

typedef struct lua_State mn_lua_t;

struct mn_config {
  unsigned long long coro_pages; // COROUTINE_PAGES
  enum mn_log_level log_level;   // LOG_LEVEL
  unsigned long port;            // PORT
  mn_lua_t *lua_;
};

struct mn_status mn_config_load(struct mn_config out[const static 1]);

struct mn_status
mn_config_load_file(struct mn_str const path, struct mn_config out[const static 1]);

struct mn_status mn_config_load_chunk(
    char const chunk[const static 1],
    struct mn_config out[const static 1]);

void mn_config_unload(struct mn_config c[const static 1]);
