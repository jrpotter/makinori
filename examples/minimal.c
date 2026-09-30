#include "makinori.h"

static struct mn_status
handle_index(struct mn_request req, struct mn_response *const res)
{
  return mn_response_write(res, mn_str_lit("Hello, world"));
}

static struct mn_route route_index = {
    .method = MN_METHOD_GET,
    .pattern = mn_str_lit("/"),
    .handler = handle_index};

int main(void)
{
  struct mn_config config = {};
  mn_config_load(&config);

  struct mn_server server = {.config = config, .route = route_index};
  mn_server_run(&server);

  mn_config_unload(&config);
}
