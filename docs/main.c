#include "makinori.h"
#include "makinori/request.h"

// =================================================================================
// Router

static struct mn_status
handle_index(struct mn_request req, struct mn_response *const res)
{
  mn_assert(req.capture_count == 1);

  char buffer[MN_REQUEST_MAX_PATH_LEN] = {};
  struct mn_str prefix = mn_str_lit("docs/_build/html/");
  mn_str_cpy(buffer, prefix);
  mn_view_cpy(buffer + prefix.len, req.captures[0]);

  return mn_response_write_file(
      res, mn_str_ref(buffer, prefix.len + req.captures[0].len));
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
