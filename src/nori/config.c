#include <lua5.4/lauxlib.h>
#include <lua5.4/lualib.h>
#include <string.h>

#include "nori/config.h"
#include "nori/logger.h"
#include "nori/util.h"

static char const base_lua[] = {
#embed "./base.lua"
    , '\0'};

static char const verify_lua[] = {
#embed "./verify.lua"
    , '\0'};

static char const util_lua[] = {
#embed "./util.lua"
    , '\0'};

static struct nori_str constexpr FLAG_LEVEL_DEBUG = nori_str_lit("debug");
static struct nori_str constexpr FLAG_LEVEL_INFO = nori_str_lit("info");
static struct nori_str constexpr FLAG_LEVEL_NOTICE = nori_str_lit("notice");
static struct nori_str constexpr FLAG_LEVEL_WARN = nori_str_lit("warn");
static struct nori_str constexpr FLAG_LEVEL_ERROR = nori_str_lit("error");

struct nori_status nori_config_load(
    lua_State *const lua,
    struct nori_str const path,
    struct nori_config out[const static 1])
{
  memset(out, 0, sizeof(*out));

  if (luaL_loadstring(lua, base_lua)) {
    auto status =
        NORI_ERROR_EMIT(NORI_ERROR_CONFIG, "Load base.lua: %s", lua_tostring(lua, -1));
    lua_pop(lua, 1);
    return status;
  }

  if (lua_pcall(lua, 0, 0, 0)) {
    auto status =
        NORI_ERROR_EMIT(NORI_ERROR_CONFIG, "Exec base.lua", lua_tostring(lua, -1));
    lua_pop(lua, 1);
    return status;
  }

  if (path.len > 0) {
    if (luaL_loadfile(lua, path.ss)) {
      auto status = NORI_ERROR_EMIT(
          NORI_ERROR_CONFIG, "Load user config: %s", lua_tostring(lua, -1));
      lua_pop(lua, 1);
      return status;
    }
    if (lua_pcall(lua, 0, 0, 0)) {
      auto status = NORI_ERROR_EMIT(
          NORI_ERROR_CONFIG, "Exec user config: %s", lua_tostring(lua, -1));
      lua_pop(lua, 1);
      return status;
    }
  }

  if (luaL_loadstring(lua, verify_lua)) {
    auto status = NORI_ERROR_EMIT(
        NORI_ERROR_CONFIG, "Load verify.lua: %s", lua_tostring(lua, -1));
    lua_pop(lua, 1);
    return status;
  }

  if (lua_pcall(lua, 0, 0, 0)) {
    auto status = NORI_ERROR_EMIT(
        NORI_ERROR_CONFIG, "Exec verify.lua: %s", lua_tostring(lua, -1));
    lua_pop(lua, 1);
    return status;
  }

  if (luaL_loadstring(lua, util_lua)) {
    auto status =
        NORI_ERROR_EMIT(NORI_ERROR_CONFIG, "Load util.lua: %s", lua_tostring(lua, -1));
    lua_pop(lua, 1);
    return status;
  }

  if (lua_pcall(lua, 0, 0, 0)) {
    auto status =
        NORI_ERROR_EMIT(NORI_ERROR_CONFIG, "Exec util.lua: %s", lua_tostring(lua, -1));
    lua_pop(lua, 1);
    return status;
  }

  // Our verification script succeeded. Assume it's safe to access globals.

  {
    lua_getglobal(lua, "COROUTINE_PAGES");
    long long val = lua_tointeger(lua, -1);
    out->nc_co_pages = val;
    lua_pop(lua, 1);
  }

  {
    // Placeholder. Currently 'poll' is the only option.
    lua_getglobal(lua, "EVENT_LOOP");
    out->nc_ev_loop = NORI_EVENT_LOOP_POLL;
    lua_pop(lua, 1);
  }

  {
    lua_getglobal(lua, "LOG_LEVEL");
    size_t len = 0;
    const char *lua_val = lua_tolstring(lua, -1, &len);
    struct nori_str val = nori_str_ref(lua_val, len);

    if (nori_str_eq(val, FLAG_LEVEL_DEBUG)) {
      out->nc_log_level = NORI_LOG_LEVEL_DEBUG;
    } else if (nori_str_eq(val, FLAG_LEVEL_INFO)) {
      out->nc_log_level = NORI_LOG_LEVEL_INFO;
    } else if (nori_str_eq(val, FLAG_LEVEL_NOTICE)) {
      out->nc_log_level = NORI_LOG_LEVEL_NOTICE;
    } else if (nori_str_eq(val, FLAG_LEVEL_WARN)) {
      out->nc_log_level = NORI_LOG_LEVEL_WARN;
    } else if (nori_str_eq(val, FLAG_LEVEL_ERROR)) {
      out->nc_log_level = NORI_LOG_LEVEL_ERROR;
    } else {
      nori_assert(false);
    }

    lua_pop(lua, 1);
  }

  {
    lua_getglobal(lua, "PORT");
    long long val = lua_tointeger(lua, -1);
    out->nc_port = val;
    lua_pop(lua, 1);
  }

  return NORI_SUCCESS;
}
