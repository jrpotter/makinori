#include <lauxlib.h>
#include <string.h>

#include "makinori/config.h"
#include "makinori/logger.h"
#include "makinori/util.h"

static char const base_lua[] = {
#embed "./base.lua"
    , '\0'};

static char const verify_lua[] = {
#embed "./verify.lua"
    , '\0'};

static struct mn_str constexpr FLAG_LEVEL_DEBUG = mn_str_lit("debug");
static struct mn_str constexpr FLAG_LEVEL_INFO = mn_str_lit("info");
static struct mn_str constexpr FLAG_LEVEL_NOTICE = mn_str_lit("notice");
static struct mn_str constexpr FLAG_LEVEL_WARN = mn_str_lit("warn");
static struct mn_str constexpr FLAG_LEVEL_ERROR = mn_str_lit("error");

struct mn_status mn_config_load(
    mn_runtime_t *const runtime,
    struct mn_str const path,
    struct mn_config out[const static 1])
{
  memset(out, 0, sizeof(*out));

  if (luaL_loadstring(runtime, base_lua)) {
    auto status = MN_ERROR_EMIT(
        MN_ERROR_CONFIG, "On loading base.lua: %s", lua_tostring(runtime, -1));
    lua_pop(runtime, 1);
    return status;
  }

  if (lua_pcall(runtime, 0, 0, 0)) {
    auto status = MN_ERROR_EMIT(
        MN_ERROR_CONFIG, "On running base.lua: %s", lua_tostring(runtime, -1));
    lua_pop(runtime, 1);
    return status;
  }

  if (path.len > 0) {
    if (luaL_loadfile(runtime, path.ss)) {
      auto status = MN_ERROR_EMIT(
          MN_ERROR_CONFIG, "On loading user config: %s", lua_tostring(runtime, -1));
      lua_pop(runtime, 1);
      return status;
    }
    if (lua_pcall(runtime, 0, 0, 0)) {
      auto status = MN_ERROR_EMIT(
          MN_ERROR_CONFIG, "On running user config: %s", lua_tostring(runtime, -1));
      lua_pop(runtime, 1);
      return status;
    }
  }

  if (luaL_loadstring(runtime, verify_lua)) {
    auto status = MN_ERROR_EMIT(
        MN_ERROR_CONFIG, "On loading verify.lua: %s", lua_tostring(runtime, -1));
    lua_pop(runtime, 1);
    return status;
  }

  if (lua_pcall(runtime, 0, 0, 0)) {
    auto status = MN_ERROR_EMIT(
        MN_ERROR_CONFIG, "On running verify.lua: %s", lua_tostring(runtime, -1));
    lua_pop(runtime, 1);
    return status;
  }

  // Our verification script succeeded. Assume it's safe to access globals.

  {
    lua_getglobal(runtime, "COROUTINE_PAGES");
    long long val = lua_tointeger(runtime, -1);
    out->coro_pages = val;
    lua_pop(runtime, 1);
  }

  {
    // Placeholder. Currently 'poll' is the only option.
    lua_getglobal(runtime, "EVENT_LOOP");
    out->ev_loop = MN_EVENT_LOOP_POLL;
    lua_pop(runtime, 1);
  }

  {
    lua_getglobal(runtime, "LOG_LEVEL");
    size_t len = 0;
    const char *lua_val = lua_tolstring(runtime, -1, &len);
    struct mn_str val = mn_str_ref(lua_val, len);

    if (mn_str_eq(val, FLAG_LEVEL_DEBUG)) {
      out->log_level = MN_LOG_LEVEL_DEBUG;
    } else if (mn_str_eq(val, FLAG_LEVEL_INFO)) {
      out->log_level = MN_LOG_LEVEL_INFO;
    } else if (mn_str_eq(val, FLAG_LEVEL_NOTICE)) {
      out->log_level = MN_LOG_LEVEL_NOTICE;
    } else if (mn_str_eq(val, FLAG_LEVEL_WARN)) {
      out->log_level = MN_LOG_LEVEL_WARN;
    } else if (mn_str_eq(val, FLAG_LEVEL_ERROR)) {
      out->log_level = MN_LOG_LEVEL_ERROR;
    } else {
      mn_assert(false);
    }

    lua_pop(runtime, 1);
  }

  {
    lua_getglobal(runtime, "PORT");
    long long val = lua_tointeger(runtime, -1);
    out->port = val;
    lua_pop(runtime, 1);
  }

  return MN_SUCCESS;
}
