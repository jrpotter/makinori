#include <lauxlib.h>
#include <lualib.h>
#include <string.h>

#include "makinori/config.h"
#include "makinori/logger.h"
#include "makinori/request.h"
#include "makinori/util.h"

static char const base_lua[] = {
#embed "./base.lua"
    , '\0'};

static char const verify_lua[] = {
#embed "./verify.lua"
    , '\0'};

static char const runtime_lua[] = {
#embed "./runtime.lua"
    , '\0'};

static struct mn_str constexpr FLAG_LEVEL_DEBUG = mn_str_lit("debug");
static struct mn_str constexpr FLAG_LEVEL_INFO = mn_str_lit("info");
static struct mn_str constexpr FLAG_LEVEL_NOTICE = mn_str_lit("notice");
static struct mn_str constexpr FLAG_LEVEL_WARN = mn_str_lit("warn");
static struct mn_str constexpr FLAG_LEVEL_ERROR = mn_str_lit("error");

struct mn_status mn_config_load(struct mn_config out[const static 1])
{
  return mn_config_load_with(mn_str_lit(""), out);
}

struct mn_status
mn_config_load_with(struct mn_str const path, struct mn_config out[const static 1])
{
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);

  // Keep in mind lua_checkstack only grows the stack, never shrinks it. The
  // default size should be able to accommodate typical usage.
  mn_assert(lua_checkstack(L, 2 * MN_REQUEST_MAX_CAPTURES));

  if (luaL_loadstring(L, base_lua)) {
    auto status =
        MN_ERROR_EMIT(MN_ERROR_CONFIG, "On loading base.lua: %s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return status;
  }

  if (lua_pcall(L, 0, 0, 0)) {
    auto status =
        MN_ERROR_EMIT(MN_ERROR_CONFIG, "On running base.lua: %s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return status;
  }

  if (path.len > 0) {
    if (luaL_loadfile(L, path.ss)) {
      auto status = MN_ERROR_EMIT(
          MN_ERROR_CONFIG, "On loading user config: %s", lua_tostring(L, -1));
      lua_pop(L, 1);
      return status;
    }
    if (lua_pcall(L, 0, 0, 0)) {
      auto status = MN_ERROR_EMIT(
          MN_ERROR_CONFIG, "On running user config: %s", lua_tostring(L, -1));
      lua_pop(L, 1);
      return status;
    }
  }

  if (luaL_loadstring(L, verify_lua)) {
    auto status = MN_ERROR_EMIT(
        MN_ERROR_CONFIG, "On loading verify.lua: %s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return status;
  }

  if (lua_pcall(L, 0, 0, 0)) {
    auto status = MN_ERROR_EMIT(
        MN_ERROR_CONFIG, "On running verify.lua: %s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return status;
  }

  if (luaL_loadstring(L, runtime_lua)) {
    auto status = MN_ERROR_EMIT(
        MN_ERROR_CONFIG, "On loading runtime.lua: %s", lua_tostring(L, -1));
    lua_close(L);
    return status;
  }

  if (lua_pcall(L, 0, 0, 0)) {
    auto status = MN_ERROR_EMIT(
        MN_ERROR_CONFIG, "On running runtime.lua: %s", lua_tostring(L, -1));
    lua_close(L);
    return status;
  }

  // Our verification script succeeded. Assume it's safe to access globals.

  {
    lua_getglobal(L, "COROUTINE_PAGES");
    long long val = lua_tointeger(L, -1);
    out->coro_pages = val;
    lua_pop(L, 1);
  }

  {
    lua_getglobal(L, "LOG_LEVEL");
    size_t len = 0;
    const char *lua_val = lua_tolstring(L, -1, &len);
    struct mn_str val = mn_str_ref(lua_val, len);

    if (mn_view_eq(val.view, FLAG_LEVEL_DEBUG.view)) {
      out->log_level = MN_LOG_LEVEL_DEBUG;
    } else if (mn_view_eq(val.view, FLAG_LEVEL_INFO.view)) {
      out->log_level = MN_LOG_LEVEL_INFO;
    } else if (mn_view_eq(val.view, FLAG_LEVEL_NOTICE.view)) {
      out->log_level = MN_LOG_LEVEL_NOTICE;
    } else if (mn_view_eq(val.view, FLAG_LEVEL_WARN.view)) {
      out->log_level = MN_LOG_LEVEL_WARN;
    } else if (mn_view_eq(val.view, FLAG_LEVEL_ERROR.view)) {
      out->log_level = MN_LOG_LEVEL_ERROR;
    } else {
      mn_assert(false);
    }

    lua_pop(L, 1);
  }

  {
    lua_getglobal(L, "PORT");
    long long val = lua_tointeger(L, -1);
    out->port = val;
    lua_pop(L, 1);
  }

  out->lua_ = L;

  return MN_SUCCESS;
}

void mn_config_unload(struct mn_config c[const static 1])
{
  lua_close(c->lua_);
  c->lua_ = nullptr;
}
