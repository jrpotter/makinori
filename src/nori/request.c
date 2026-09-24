#include <lauxlib.h>

#include "nori/request.h"
#include "nori/string.h"
#include "nori/util.h"

static struct nori_status
nori_route_validate(struct nori_route const route[const static 1])
{
  if (route->nr_pattern.len == 0) {
    return NORI_WARN_EMIT(
        NORI_ERROR_INVALID_ARG, "Encountered route with empty pattern");
  }

  unsigned int capture_count = 0;

  for (size_t i = 0; i < route->nr_pattern.len; ++i) {
    if (route->nr_pattern.ss[i] == '%') {
      i += 1;
    } else if (route->nr_pattern.ss[i] == '(') {
      capture_count += 1;
    }
  }

  if (capture_count > NORI_REQUEST_MAX_CAPTURES) {
    return NORI_WARN_EMIT(
        NORI_ERROR_INVALID_ARG, "Pattern %s has more captures than %d",
        route->nr_pattern.ss, NORI_REQUEST_MAX_CAPTURES);
  }

  return NORI_SUCCESS;
}

static struct nori_view
find_substr(struct nori_str const path, struct nori_str const needle)
{
  for (int i = 0; i < path.len - needle.len + 1; ++i) {
    struct nori_view substr = nori_str_substr(path, i, i + needle.len);
    if (nori_view_eq(substr, nori_str_to_view(needle))) {
      return substr;
    }
  }
  return nori_str_to_view(nori_str_lit(""));
}

struct nori_route const *const nori_route_match(
    nori_runtime_t *const runtime,
    struct nori_route const route[const static 1],
    struct nori_request req[static 1])
{
  if (req->nr_path.len == 0) {
    nori_log_warn("No route matches an empty path");
    return nullptr;
  }

  lua_getglobal(runtime, "nori");

  struct nori_route const *match = nullptr;

  for (struct nori_route const *curr = route; curr; curr = route->nr_next) {
    struct nori_status status = nori_route_validate(curr);
    if (status.ns_error) {
      continue;
    }

    lua_getfield(runtime, -1, "anchor_string_match");
    lua_pushlstring(runtime, req->nr_path.ss, req->nr_path.len);
    lua_pushlstring(runtime, curr->nr_pattern.ss, curr->nr_pattern.len);

    if (lua_pcall(runtime, 2, LUA_MULTRET, 0)) {
      nori_log_warn("nori.anchor_string_match: %s", lua_tostring(runtime, -1));
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
    nori_assert(captured <= NORI_REQUEST_MAX_CAPTURES);

    // We need our captures to reference the string in the request path, not
    // the return values from Lua (which will be memory collected once we reset
    // the virtual stack). Though string.match doesn't tell us which match was
    // actually captured, it doesn't matter. We can just pick the first one.
    for (int i = 0; i < captured; ++i) {
      size_t len = 0;
      char const *capture = lua_tolstring(runtime, -1, &len);
      struct nori_str needle = nori_str_ref(capture, len);

      // Pull the substring out of the request path for lifetime handling.
      struct nori_view substr = find_substr(req->nr_path, needle);
      nori_assert(!nori_view_empty(substr));
      req->nr_captures[captured - i - 1] = substr;

      lua_pop(runtime, 1);
    }

    match = curr;
    break;
  }

  lua_pop(runtime, 1); // nori global
  return match;
}
