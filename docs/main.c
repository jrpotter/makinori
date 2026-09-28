#include "makinori.h"

// =================================================================================
// Router

static struct mn_str const css_ext = mn_str_lit(".css");
static struct mn_str const html_ext = mn_str_lit(".html");
static struct mn_str const js_ext = mn_str_lit(".js");

static struct mn_status
handle_index(struct mn_request req, struct mn_response *const res)
{
  mn_assert(req.capture_count == 1);

  char buffer[MN_REQUEST_MAX_PATH_LEN] = {};

  struct mn_str prefix = mn_str_lit("docs/_build/html/");
  size_t total = mn_view_cpy(buffer, prefix.view);
  total += mn_view_cpy(buffer + prefix.len, req.captures[0]);

  const struct mn_str path = mn_str_ref(buffer, total);

  MN_PAIR(struct mn_str, struct mn_str)
  cts[] = {
      {css_ext, MN_MEDIA_TYPE_CSS},
      {html_ext, MN_MEDIA_TYPE_HTML},
      {js_ext, MN_MEDIA_TYPE_JAVASCRIPT},
  };

  for (size_t i = 0; i < MN_ARR_SIZE(cts); ++i) {
    if (mn_view_find(path.view, cts[i].fst.view) < path.len) {
      mn_response_set_header(res, MN_HEADER_CONTENT_TYPE, cts[i].snd);
      break;
    }
  }

  // TODO: Redirect / to index.html.
  // TODO: Do not allow navigating outside of directory.
  return mn_response_write_file(res, path);
}

static struct mn_route route_index = {
    .method = MN_METHOD_GET,
    .pattern = mn_str_lit("/(.*)"),
    .next = nullptr,
    .handler = handle_index};

// =================================================================================
// Main

int main(void)
{
  struct mn_config config = {};
  auto status = mn_config_load(&config);
  if (status.error) {
    return EXIT_FAILURE;
  }

  struct mn_server server = {.config = config, .route = route_index};
  status = mn_server_run(&server);

  mn_config_unload(&config);
  return status.error ? EXIT_FAILURE : EXIT_SUCCESS;
}
