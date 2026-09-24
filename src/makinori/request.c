#include <lauxlib.h>

#include "makinori/request.h"
#include "makinori/string.h"
#include "makinori/util.h"

static struct mn_status mn_route_validate(struct mn_route const route[const static 1])
{
  if (route->pattern.len == 0) {
    return MN_WARN_EMIT(MN_ERROR_INVALID_ARG, "Encountered route with empty pattern");
  }

  unsigned int capture_count = 0;

  for (size_t i = 0; i < route->pattern.len; ++i) {
    if (route->pattern.ss[i] == '%') {
      i += 1;
    } else if (route->pattern.ss[i] == '(') {
      capture_count += 1;
    }
  }

  if (capture_count > MN_REQUEST_MAX_CAPTURES) {
    return MN_WARN_EMIT(
        MN_ERROR_INVALID_ARG, "Pattern %s has more captures than %d", route->pattern.ss,
        MN_REQUEST_MAX_CAPTURES);
  }

  return MN_SUCCESS;
}

static struct mn_view find_substr(struct mn_str const path, struct mn_str const needle)
{
  for (int i = 0; i < path.len - needle.len + 1; ++i) {
    struct mn_view substr = mn_str_substr(path, i, i + needle.len);
    if (mn_view_eq(substr, mn_str_to_view(needle))) {
      return substr;
    }
  }
  return mn_str_to_view(mn_str_lit(""));
}

struct mn_route const *const mn_route_match(
    mn_runtime_t *const runtime,
    struct mn_route const route[const static 1],
    struct mn_request req[static 1])
{
  if (req->path.len == 0) {
    mn_log_warn("No route matches an empty path");
    return nullptr;
  }

  lua_getglobal(runtime, "nori");

  struct mn_route const *match = nullptr;

  for (struct mn_route const *curr = route; curr; curr = route->next) {
    struct mn_status status = mn_route_validate(curr);
    if (status.error) {
      continue;
    }

    lua_getfield(runtime, -1, "anchor_string_match");
    lua_pushlstring(runtime, req->path.ss, req->path.len);
    lua_pushlstring(runtime, curr->pattern.ss, curr->pattern.len);

    if (lua_pcall(runtime, 2, LUA_MULTRET, 0)) {
      mn_log_warn("nori.anchor_string_match: %s", lua_tostring(runtime, -1));
      lua_pop(runtime, 1);
      break;
    }

    // Matcher returned nil meaning the path did not match the pattern.
    if (lua_type(runtime, -1) == LUA_TNIL) {
      lua_pop(runtime, 1);
      continue;
    }

    int sentinel = -1;
    while (lua_type(runtime, sentinel) == LUA_TSTRING) {
      sentinel -= 1;
    }
    int captured = -sentinel - 1;
    mn_assert(captured <= MN_REQUEST_MAX_CAPTURES);

    // We need our captures to reference the string in the request path, not
    // the return values from Lua (which will be memory collected once we reset
    // the virtual stack). Though string.match doesn't tell us which match was
    // actually captured, it doesn't matter. We can just pick the first one.
    for (int i = 0; i < captured; ++i) {
      size_t len = 0;
      char const *capture = lua_tolstring(runtime, -1, &len);
      struct mn_str needle = mn_str_ref(capture, len);

      // Pull the substring out of the request path for lifetime handling.
      struct mn_view substr = find_substr(req->path, needle);
      mn_assert(!mn_view_empty(substr));
      req->captures[captured - i - 1] = substr;

      lua_pop(runtime, 1);
    }

    match = curr;
    break;
  }

  lua_pop(runtime, 1); // nori global
  return match;
}
