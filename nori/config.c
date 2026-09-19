#include <lua5.4/lauxlib.h>
#include <lua5.4/lualib.h>

#include "nori/config.h"
#include "nori/util.h"

static const char config_base[] = {
#embed "nori/base.lua"
    , '\0'};

static const char config_verify[] = {
#embed "nori/verify.lua"
    , '\0'};

static constexpr struct nori_str_view LEVEL_ERROR = NSV("error");
static constexpr struct nori_str_view LEVEL_WARN = NSV("warn");
static constexpr struct nori_str_view LEVEL_NOTICE = NSV("notice");
static constexpr struct nori_str_view LEVEL_INFO = NSV("info");

struct nori_status nori_config_load(
    const struct nori_str_view path,
    struct nori_config out[const static 1])
{
  memset(out, 0, sizeof(*out));

  lua_State *L = luaL_newstate();
  luaL_openlibs(L);

  if (luaL_loadstring(L, config_base)) {
    nori_log_error("Load base config: %s", lua_tostring(L, -1));
    return NORI_FAILURE;
  }

  if (lua_pcall(L, 0, 0, 0)) {
    nori_log_error("Exec base config: %s", lua_tostring(L, -1));
    return NORI_FAILURE;
  }

  if (path.len > 0) {
    if (luaL_loadfile(L, path.view)) {
      nori_log_error("Open user config: %s", lua_tostring(L, -1));
      return NORI_FAILURE;
    }
    if (lua_pcall(L, 0, 0, 0)) {
      nori_log_error("Exec user config: %s", lua_tostring(L, -1));
      return NORI_FAILURE;
    }
  }

  if (luaL_loadstring(L, config_verify)) {
    nori_log_error("Load verify config: %s", lua_tostring(L, -1));
    return NORI_FAILURE;
  }

  if (lua_pcall(L, 0, 0, 0)) {
    nori_log_error("Exec verify config: %s", lua_tostring(L, -1));
    return NORI_FAILURE;
  }

  // Our verification script succeeded. Assume it's safe to access globals.

  {
    lua_getglobal(L, "LOG_LEVEL");
    size_t len = 0;
    struct nori_str_view val = nori_str_view_of(lua_tolstring(L, -1, &len));

    out->nc_log = LLL_ERR;
    if (!nori_str_view_eq(val, LEVEL_ERROR)) {
      out->nc_log |= LLL_WARN;
      if (!nori_str_view_eq(val, LEVEL_WARN)) {
        out->nc_log |= LLL_NOTICE;
        if (!nori_str_view_eq(val, LEVEL_NOTICE)) {
          out->nc_log |= LLL_INFO;
          if (!nori_str_view_eq(val, LEVEL_INFO)) {
            out->nc_log |= LLL_DEBUG;
          }
        }
      }
    }
  }

  {
    lua_getglobal(L, "PORT");
    long long val = lua_tointeger(L, -1);
    out->nc_port = val;
  }

  {
    out->nc_state = L;
  }

  nori_set_log_level(out->nc_log);

  return NORI_SUCCESS;
}

struct nori_status nori_config_unload(struct nori_config config[const static 1])
{
  lua_close(config->nc_state);

  config->nc_state = nullptr;
  config->nc_port = 0;
  config->nc_log = 0;

  return NORI_SUCCESS;
}
