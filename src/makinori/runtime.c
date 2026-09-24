#include <lauxlib.h>
#include <lualib.h>

#include "makinori/request.h"
#include "makinori/runtime.h"
#include "makinori/util.h"

static char const runtime_lua[] = {
#embed "./runtime.lua"
    , '\0'};

mn_runtime_t *const mn_runtime_create(void)
{
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);

  // Keep in mind lua_checkstack only grows the stack, never shrinks it. The
  // default size should be able to accommodate typical usage.
  mn_assert(lua_checkstack(L, 2 * MN_REQUEST_MAX_CAPTURES));

  if (luaL_loadstring(L, runtime_lua)) {
    mn_log_error("On loading runtime.lua: %s", lua_tostring(L, -1));
    lua_close(L);
    return nullptr;
  }

  if (lua_pcall(L, 0, 0, 0)) {
    mn_log_error("On running runtime.lua: %s", lua_tostring(L, -1));
    lua_close(L);
    return nullptr;
  }

  return L;
}

void mn_runtime_destroy(mn_runtime_t *const rt)
{
  lua_close(rt);
}
