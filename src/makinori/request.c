#include <lauxlib.h>

#include "makinori/request.h"
#include "makinori/string.h"
#include "makinori/util.h"

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
    if (route->pattern.len == 0) {
      mn_log_warn("Encountered route with empty pattern");
      continue;
    }

    size_t expected_count = 0;
    for (size_t i = 0; i < route->pattern.len; ++i) {
      if (route->pattern.ss[i] == '%') {
        i += 1;
      } else if (route->pattern.ss[i] == '(') {
        expected_count += 1;
      }
    }

    // Must abort this path. We configured the amount of virtual stack space
    // according to MN_REQUEST_MAX_CAPTURES.
    if (expected_count > MN_REQUEST_MAX_CAPTURES) {
      mn_log_warn(
          "Pattern %s has more captures than %d", route->pattern.ss,
          MN_REQUEST_MAX_CAPTURES);
      continue;
    }

    lua_getfield(config->lua_, -1, "anchor_string_match");
    // Use of ss_ is safe since we also pass the length.
    lua_pushlstring(config->lua_, req->path.ss_, req->path.len);
    lua_pushlstring(config->lua_, curr->pattern.ss, curr->pattern.len);

    if (lua_pcall(config->lua_, 2, LUA_MULTRET, 0)) {
      mn_log_warn("nori.anchor_string_match: %s", lua_tostring(config->lua_, -1));
      lua_pop(config->lua_, 1);
      break;
    }

    // On a failed match, string.match returns nil.
    if (lua_type(config->lua_, -1) == LUA_TNIL) {
      lua_pop(config->lua_, 1);
      continue;
    }

    // On a successful match with no specificed captures, it returns the entire
    // string. Otherwise it returns one or more substrings corresponding to
    // each capture.
    int result_count = 0;
    while (lua_type(config->lua_, -result_count - 1) == LUA_TSTRING) {
      result_count += 1;
    }
    req->capture_count = expected_count == 0 && result_count == 1 ? 0 : result_count;
    mn_assert(expected_count == req->capture_count);
    mn_assert(req->capture_count <= MN_REQUEST_MAX_CAPTURES);

    // We need our captures to reference the string in the request path, not
    // the return values from Lua (which will be memory collected once we reset
    // the virtual stack). Though string.match doesn't tell us which match was
    // actually captured, it doesn't matter. We can just pick the first one.
    for (int i = 0; i < result_count; ++i) {
      size_t len = 0;
      char const *capture = lua_tolstring(config->lua_, -1, &len);
      struct mn_view needle = mn_view_ref(capture, len);
      struct mn_view substr = find_substr(req->path, needle);
      mn_assert(substr.len > 0);

      req->captures[result_count - i - 1] = substr;
      lua_pop(config->lua_, 1);
    }

    match = curr;
    break;
  }

  lua_pop(config->lua_, 1); // nori global
  return match;
}
