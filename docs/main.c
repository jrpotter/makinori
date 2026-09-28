#include "makinori.h"

// =================================================================================
// Router

static struct mn_str constexpr CSS_EXT = mn_str_lit(".css");
static struct mn_str constexpr HTML_EXT = mn_str_lit(".html");
static struct mn_str constexpr JS_EXT = mn_str_lit(".js");
static struct mn_str constexpr PREFIX = mn_str_lit("docs/_build/html/");

static struct mn_status handle_dir(struct mn_request req, struct mn_response *const res)
{
  mn_assert(req.capture_count == 1);

  char buffer[MN_REQUEST_MAX_PATH_LEN] = {};
  size_t n = mn_view_cpy(buffer, PREFIX.view, MN_REQUEST_MAX_PATH_LEN);
  n += mn_view_cpy(buffer + n, req.captures[0], MN_REQUEST_MAX_PATH_LEN - n);

  struct mn_str const filename = mn_str_ref(buffer, n);

  MN_PAIR(struct mn_str, struct mn_str)
  media[] = {
      {CSS_EXT, MN_MEDIA_TYPE_CSS},
      {HTML_EXT, MN_MEDIA_TYPE_HTML},
      {JS_EXT, MN_MEDIA_TYPE_JAVASCRIPT},
  };

  for (size_t i = 0; i < MN_ARR_SIZE(media); ++i) {
    if (mn_view_find(filename.view, media[i].fst.view) < filename.len) {
      auto status = mn_response_set_header(res, MN_HEADER_CONTENT_TYPE, media[i].snd);
      if (status.error) {
        return status;
      }
      break;
    }
  }

  return mn_response_write_file(res, filename);
}

static struct mn_status
handle_main(struct mn_request req, struct mn_response *const res)
{
  auto status = mn_response_set_code(res, MN_HTTP_MOVED_PERMANENTLY);
  if (status.error) {
    return status;
  }
  status = mn_response_set_header(res, MN_HEADER_LOCATION, mn_str_lit("/index.html"));
  if (status.error) {
    return status;
  }
  return MN_SUCCESS;
}

static struct mn_route route_dir = {
    .method = MN_METHOD_GET,
    .pattern = mn_str_lit("/(.*)"),
    .handler = handle_dir};

static struct mn_route route_main = {
    .method = MN_METHOD_GET,
    .pattern = mn_str_lit("/"),
    .next = &route_dir,
    .handler = handle_main};

// =================================================================================
// Main

int main(void)
{
  struct mn_config config = {};
  auto status = mn_config_load(&config);
  if (status.error) {
    return EXIT_FAILURE;
  }

  config.port = 1314;
  struct mn_server server = {.config = config, .route = route_main};
  status = mn_server_run(&server);

  mn_config_unload(&config);
  return status.error ? EXIT_FAILURE : EXIT_SUCCESS;
}
