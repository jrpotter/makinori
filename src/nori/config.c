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

  struct nori_status status = NORI_SUCCESS;

  if (luaL_loadstring(L, base_lua)) {
    status =
        NORI_ERROR_EMIT(NORI_ERROR_CONFIG, "Load base.lua: %s", lua_tostring(L, -1));
    goto cleanup;
  }

  if (lua_pcall(L, 0, 0, 0)) {
    status = NORI_ERROR_EMIT(NORI_ERROR_CONFIG, "Exec base.lua", lua_tostring(L, -1));
    goto cleanup;
  }

  if (path.len > 0) {
    if (luaL_loadfile(L, path.view)) {
      status = NORI_ERROR_EMIT(
          NORI_ERROR_CONFIG, "Load user config: %s", lua_tostring(L, -1));
      goto cleanup;
    }
    if (lua_pcall(L, 0, 0, 0)) {
      status = NORI_ERROR_EMIT(
          NORI_ERROR_CONFIG, "Exec user config: %s", lua_tostring(L, -1));
      goto cleanup;
    }
  }

  if (luaL_loadstring(L, verify_lua)) {
    status =
        NORI_ERROR_EMIT(NORI_ERROR_CONFIG, "Load verify.lua: %s", lua_tostring(L, -1));
    goto cleanup;
  }

  if (lua_pcall(L, 0, 0, 0)) {
    status =
        NORI_ERROR_EMIT(NORI_ERROR_CONFIG, "Exec verify.lua: %s", lua_tostring(L, -1));
    goto cleanup;
  }

  // Our verification script succeeded. Assume it's safe to access globals.

  {
    lua_getglobal(L, "COROUTINE_PAGES");
    long long val = lua_tointeger(L, -1);
    out->nc_co_pages = val;
  }

  {
    // Placeholder. Currently 'poll' is the only option.
    lua_getglobal(L, "EVENT_LOOP");
    out->nc_ev_loop = NORI_EVENT_LOOP_POLL;
  }

  {
    lua_getglobal(L, "LOG_LEVEL");
    size_t len = 0;
    const char *lua_val = lua_tolstring(L, -1, &len);
    struct nori_str_view val = nori_str_view_create(lua_val, len);

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

cleanup:
  lua_close(L);
  return status;
}
