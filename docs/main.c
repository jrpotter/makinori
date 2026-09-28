#include "makinori.h"

// =================================================================================
// Router

static struct mn_str constexpr css_ext = mn_str_lit(".css");
static struct mn_str constexpr html_ext = mn_str_lit(".html");
static struct mn_str constexpr js_ext = mn_str_lit(".js");

static struct mn_status
handle_docs(struct mn_request req, struct mn_response *const res)
{
  mn_assert(req.capture_count == 1);

  char buffer[MN_REQUEST_MAX_PATH_LEN] = {};
  struct mn_str prefix = mn_str_lit("docs/_build/html/");
  size_t total = mn_view_cpy(buffer, prefix.view);
  total += mn_view_cpy(buffer + prefix.len, req.captures[0]);

  struct mn_str const filename = mn_str_ref(buffer, total);

  MN_PAIR(struct mn_str, struct mn_str)
  media[] = {
      {css_ext, MN_MEDIA_TYPE_CSS},
      {html_ext, MN_MEDIA_TYPE_HTML},
      {js_ext, MN_MEDIA_TYPE_JAVASCRIPT},
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
handle_redirect(struct mn_request req, struct mn_response *const res)
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

static struct mn_route route_docs = {
    .method = MN_METHOD_GET,
    .pattern = mn_str_lit("/(.*)"),
    .handler = handle_docs};

static struct mn_route route_redirect = {
    .method = MN_METHOD_GET,
    .pattern = mn_str_lit("/"),
    .next = &route_docs,
    .handler = handle_redirect};

// =================================================================================
// Main

int main(void)
{
  struct mn_config config = {};
  auto status = mn_config_load(&config);
  if (status.error) {
    return EXIT_FAILURE;
  }

  struct mn_server server = {.config = config, .route = route_redirect};
  status = mn_server_run(&server);

  mn_config_unload(&config);
  return status.error ? EXIT_FAILURE : EXIT_SUCCESS;
}
