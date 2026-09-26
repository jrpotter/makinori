#include <lauxlib.h>

#include "makinori/request.h"
#include "makinori/string.h"
#include "makinori/util.h"

static struct mn_status mn_route_validate(struct mn_route const route[const static 1])
{
  if (route->pattern.len == 0) {
    return MN_WARN_EMIT(MN_ERROR_INVALID_ARG, "Encountered route with empty pattern");
  }

  unsigned int count = 0;

  for (size_t i = 0; i < route->pattern.len; ++i) {
    if (route->pattern.ss[i] == '%') {
      i += 1;
    } else if (route->pattern.ss[i] == '(') {
      count += 1;
    }
  }

  if (count > MN_REQUEST_MAX_CAPTURES) {
    return MN_WARN_EMIT(
        MN_ERROR_INVALID_ARG, "Pattern %s has more captures than %d", route->pattern.ss,
        MN_REQUEST_MAX_CAPTURES);
  }

  return MN_SUCCESS;
}

static struct mn_view
find_substr(struct mn_view const path, struct mn_view const needle)
{
  for (int i = 0; i < path.len - needle.len + 1; ++i) {
    struct mn_view substr = mn_view_substr(path, i, i + needle.len);
    if (mn_view_eq(substr, needle)) {
      return substr;
    }
  }
  return mn_str_to_view(mn_str_lit(""));
}

struct mn_route const *const mn_route_match(
    struct mn_config const config[const static 1],
    struct mn_route const route[const static 1],
    struct mn_request req[static 1])
{
  if (req->path.len == 0) {
    mn_log_warn("No route matches an empty path");
    return nullptr;
  }

  lua_getglobal(config->lua_, "nori");

  struct mn_route const *match = nullptr;

  for (struct mn_route const *curr = route; curr; curr = route->next) {
    struct mn_status status = mn_route_validate(curr);
    if (status.error) {
      continue;
    }

    lua_getfield(config->lua_, -1, "anchor_string_match");
    // Use of ss_ is safe here since we also pass the length.
    lua_pushlstring(config->lua_, req->path.ss_, req->path.len);
    lua_pushlstring(config->lua_, curr->pattern.ss, curr->pattern.len);

    if (lua_pcall(config->lua_, 2, LUA_MULTRET, 0)) {
      mn_log_warn("nori.anchor_string_match: %s", lua_tostring(config->lua_, -1));
      lua_pop(config->lua_, 1);
      break;
    }

    // Matcher returned nil meaning the path did not match the pattern.
    if (lua_type(config->lua_, -1) == LUA_TNIL) {
      lua_pop(config->lua_, 1);
      continue;
    }

    int sentinel = -1;
    while (lua_type(config->lua_, sentinel) == LUA_TSTRING) {
      sentinel -= 1;
    }
    int results = -sentinel - 1;
    mn_assert(results <= MN_REQUEST_MAX_CAPTURES);

    // We need our captures to reference the string in the request path, not
    // the return values from Lua (which will be memory collected once we reset
    // the virtual stack). Though string.match doesn't tell us which match was
    // actually captured, it doesn't matter. We can just pick the first one.
    for (int i = 0; i < results; ++i) {
      size_t len = 0;
      char const *capture = lua_tolstring(config->lua_, -1, &len);
      struct mn_view needle = mn_view_ref(capture, len);

      // Pull the substring out of the request path for lifetime handling.
      struct mn_view substr = find_substr(req->path, needle);
      mn_assert(substr.len > 0);
      req->captures[results - i - 1] = substr;

      lua_pop(config->lua_, 1);
    }

    match = curr;
    break;
  }

  lua_pop(config->lua_, 1); // nori global
  return match;
}
