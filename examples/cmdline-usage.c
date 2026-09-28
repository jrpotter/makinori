/**
 * Demonstrates usage of the cmdline.h module to parse command line flags.
 *
 * ```sh
 * $ make bin/cmdline-usage
 * $ ./cmdline-usage -h
 * $ ./cmdline-usage run
 * $ curl localhost:8000
 * ```
 */
#include <stdio.h>
#include <stdlib.h>

#define MN_CMDLINE_MAX_FLAGS 4
#define MN_CMDLINE_MAX_ARITY 2

#include "makinori.h"

// =================================================================================
// Router

static struct mn_status
handle_index(struct mn_request req, struct mn_response *const res)
{
  return mn_response_write(res, mn_str_lit("Hello, world"));
}

static struct mn_route route_index = {
    .method = MN_METHOD_GET,
    .pattern = mn_str_lit("/"),
    .next = nullptr,
    .handler = handle_index};

// =================================================================================
// Main

int main(int argc, char const *argv[argc])
{
  struct mn_flag flag_help = {
      .sflag = mn_str_lit("h"),
      .lflag = mn_str_lit("help"),
      .arity = 0,
  };

  struct mn_flag flag_config = {
      .sflag = mn_str_lit("c"),
      .lflag = mn_str_lit("config"),
      .arity = 1,
  };

  struct mn_cmdline cl = {.flags = {&flag_help, &flag_config}};

  struct mn_status status = mn_cmdline_parse(argc, argv, &cl);
  if (status.error) {
    return EXIT_FAILURE;
  }

  if (cl.action.len == 0 || flag_help.set) {
    printf(
        "Usage: %s [run]\n"
        "-h                   Print this help description.\n"
        "-c, --config <FILE>  Load an optional config file.\n",
        argv[0]);
    return EXIT_SUCCESS;
  }

  struct mn_config config = {};
  status = mn_config_load_with(flag_config.vals[0], &config);
  if (status.error) {
    return EXIT_FAILURE;
  }

  // Any logs emitted earlier are ERRORs and are always emitted. Therefore
  // setting this now behaves the same as if we were to have set it earlier.
  mn_log_set_level(config.log_level);

  if (mn_str_eq(cl.action, mn_str_lit("run"))) {
    struct mn_server server = {.config = config, .route = route_index};
    status = mn_server_run(&server);
  } else {
    status = MN_ERROR_EMIT(MN_ERROR_INVALID_ARG, "Unknown action %s", cl.action.ss);
  }

  mn_config_unload(&config);
  return status.error ? EXIT_FAILURE : EXIT_SUCCESS;
}
