#include <lua5.4/lauxlib.h>
#include <lua5.4/lualib.h>
#include <string.h>

#include "nori/config.h"
#include "nori/logger.h"
#include "nori/util.h"

static char const config_base[] = {
#embed "./base.lua"
    , '\0'};

static char const config_verify[] = {
#embed "./verify.lua"
    , '\0'};

static struct nori_str_view constexpr FLAG_LEVEL_DEBUG = NSV("debug");
static struct nori_str_view constexpr FLAG_LEVEL_INFO = NSV("info");
static struct nori_str_view constexpr FLAG_LEVEL_NOTICE = NSV("notice");
static struct nori_str_view constexpr FLAG_LEVEL_WARN = NSV("warn");
static struct nori_str_view constexpr FLAG_LEVEL_ERROR = NSV("error");

struct nori_status nori_config_load(
    struct nori_str_view const path,
    struct nori_config out[const static 1])
{
  memset(out, 0, sizeof(*out));

  lua_State *L = luaL_newstate();
  luaL_openlibs(L);

  if (luaL_loadstring(L, config_base)) {
    return NORI_ERROR_EMIT(
        NORI_ERROR_GENERIC, "Load base config: %s", lua_tostring(L, -1));
  }

  if (lua_pcall(L, 0, 0, 0)) {
    return NORI_ERROR_EMIT(
        NORI_ERROR_GENERIC, "Exec base config: %s", lua_tostring(L, -1));
  }

  if (path.len > 0) {
    if (luaL_loadfile(L, path.view)) {
      return NORI_ERROR_EMIT(
          NORI_ERROR_GENERIC, "Open user config: %s", lua_tostring(L, -1));
    }
    if (lua_pcall(L, 0, 0, 0)) {
      return NORI_ERROR_EMIT(
          NORI_ERROR_GENERIC, "Exec user config: %s", lua_tostring(L, -1));
    }
  }

  if (luaL_loadstring(L, config_verify)) {
    return NORI_ERROR_EMIT(
        NORI_ERROR_GENERIC, "Load verify config: %s", lua_tostring(L, -1));
  }

  if (lua_pcall(L, 0, 0, 0)) {
    return NORI_ERROR_EMIT(
        NORI_ERROR_GENERIC, "Exec verify config: %s", lua_tostring(L, -1));
  }

  // Our verification script succeeded. Assume it's safe to access globals.

  {
    lua_getglobal(L, "COROUTINE_STACK");
    long long val = lua_tointeger(L, -1);
    out->nc_co_stack = val;
  }

  {
    lua_getglobal(L, "EVENT_LOOP");
    size_t len = 0;
    [[maybe_unused]] struct nori_str_view val = // Must be 'poll'.
        nori_str_view_of(lua_tolstring(L, -1, &len));
    out->nc_ev_loop = NORI_EVENT_LOOP_POLL;
  }

  {
    lua_getglobal(L, "LOG_LEVEL");
    size_t len = 0;
    struct nori_str_view val = nori_str_view_of(lua_tolstring(L, -1, &len));

    if (nori_str_view_eq(val, FLAG_LEVEL_DEBUG)) {
      out->nc_log_level = NORI_LOG_LEVEL_DEBUG;
    } else if (nori_str_view_eq(val, FLAG_LEVEL_INFO)) {
      out->nc_log_level = NORI_LOG_LEVEL_INFO;
    } else if (nori_str_view_eq(val, FLAG_LEVEL_NOTICE)) {
      out->nc_log_level = NORI_LOG_LEVEL_NOTICE;
    } else if (nori_str_view_eq(val, FLAG_LEVEL_WARN)) {
      out->nc_log_level = NORI_LOG_LEVEL_WARN;
    } else if (nori_str_view_eq(val, FLAG_LEVEL_ERROR)) {
      out->nc_log_level = NORI_LOG_LEVEL_ERROR;
    } else {
      nori_assert(false);
    }
  }

  {
    lua_getglobal(L, "PORT");
    long long val = lua_tointeger(L, -1);
    out->nc_port = val;
  }

  lua_close(L);

  return NORI_SUCCESS;
}
