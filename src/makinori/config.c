#include <lauxlib.h>
#include <lualib.h>
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

static char const runtime_lua[] = {
#embed "./runtime.lua"
    , '\0'};

static struct mn_view constexpr FLAG_LEVEL_DEBUG = mn_view_lit("debug");
static struct mn_view constexpr FLAG_LEVEL_INFO = mn_view_lit("info");
static struct mn_view constexpr FLAG_LEVEL_NOTICE = mn_view_lit("notice");
static struct mn_view constexpr FLAG_LEVEL_WARN = mn_view_lit("warn");
static struct mn_view constexpr FLAG_LEVEL_ERROR = mn_view_lit("error");

static struct mn_status mn_load_before(mn_lua_t *L)
{
  luaL_openlibs(L);
  // lua_checkstack never shrinks the stack. The default size should be able to
  // accommodate typical usage.
  mn_assert(lua_checkstack(L, 2 * MN_REQUEST_MAX_CAPTURES));

  if (luaL_loadstring(L, base_lua)) {
    auto status =
        MN_ERROR_EMIT(MN_ERROR_CONFIG, "On loading base.lua: %s", lua_tostring(L, -1));
    lua_close(L);
    return status;
  }

  if (lua_pcall(L, 0, 0, 0)) {
    auto status =
        MN_ERROR_EMIT(MN_ERROR_CONFIG, "On running base.lua: %s", lua_tostring(L, -1));
    lua_close(L);
    return status;
  }

  return MN_SUCCESS;
}

static struct mn_status mn_load_after(mn_lua_t *L, struct mn_config out[const static 1])
{
  if (luaL_loadstring(L, verify_lua)) {
    auto status = MN_ERROR_EMIT(
        MN_ERROR_CONFIG, "On loading verify.lua: %s", lua_tostring(L, -1));
    lua_close(L);
    return status;
  }

  if (lua_pcall(L, 0, 0, 0)) {
    auto status = MN_ERROR_EMIT(
        MN_ERROR_CONFIG, "On running verify.lua: %s", lua_tostring(L, -1));
    lua_close(L);
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
    struct mn_view val = mn_view_ref(lua_val, len);

    if (mn_view_eq(val, FLAG_LEVEL_DEBUG)) {
      out->log_level = MN_LOG_LEVEL_DEBUG;
    } else if (mn_view_eq(val, FLAG_LEVEL_INFO)) {
      out->log_level = MN_LOG_LEVEL_INFO;
    } else if (mn_view_eq(val, FLAG_LEVEL_NOTICE)) {
      out->log_level = MN_LOG_LEVEL_NOTICE;
    } else if (mn_view_eq(val, FLAG_LEVEL_WARN)) {
      out->log_level = MN_LOG_LEVEL_WARN;
    } else if (mn_view_eq(val, FLAG_LEVEL_ERROR)) {
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

struct mn_status mn_config_load(struct mn_config out[const static 1])
{
  mn_lua_t *L = luaL_newstate();

  {
    auto status = mn_load_before(L);
    if (status.error) {
      return status;
    }
  }

  return mn_load_after(L, out);
}

struct mn_status
mn_config_load_file(struct mn_str const path, struct mn_config out[const static 1])
{
  mn_lua_t *L = luaL_newstate();

  {
    auto status = mn_load_before(L);
    if (status.error) {
      return status;
    }
  }

  // If this were to continue to `luaL_loadfile`, it would attempt to read in
  // stdin.
  if (path.len == 0) {
    auto status = MN_ERROR_EMIT(MN_ERROR_CONFIG, "Attempted to load an empty file");
    lua_close(L);
    return status;
  }

  if (luaL_loadfile(L, path.ss)) {
    auto status =
        MN_ERROR_EMIT(MN_ERROR_CONFIG, "On loading file: %s", lua_tostring(L, -1));
    lua_close(L);
    return status;
  }

  if (lua_pcall(L, 0, 0, 0)) {
    auto status =
        MN_ERROR_EMIT(MN_ERROR_CONFIG, "On running file: %s", lua_tostring(L, -1));
    lua_close(L);
    return status;
  }

  return mn_load_after(L, out);
}

struct mn_status mn_config_load_chunk(
    char const chunk[const static 1],
    struct mn_config out[const static 1])
{
  mn_lua_t *L = luaL_newstate();

  {
    auto status = mn_load_before(L);
    if (status.error) {
      return status;
    }
  }

  if (luaL_loadstring(L, chunk)) {
    auto status =
        MN_ERROR_EMIT(MN_ERROR_CONFIG, "On loading chunk: %s", lua_tostring(L, -1));
    lua_close(L);
    return status;
  }

  if (lua_pcall(L, 0, 0, 0)) {
    auto status =
        MN_ERROR_EMIT(MN_ERROR_CONFIG, "On running chunk: %s", lua_tostring(L, -1));
    lua_close(L);
    return status;
  }

  return mn_load_after(L, out);
}

void mn_config_unload(struct mn_config c[const static 1])
{
  lua_close(c->lua_);
  c->lua_ = nullptr;
}
