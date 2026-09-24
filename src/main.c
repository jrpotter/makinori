#include <stdio.h>
#include <stdlib.h>

#include "makinori.h"

// =================================================================================
// Command Line

static struct mn_flag FLAG_HELP = {
    .sflag = mn_str_lit("h"),
    .lflag = mn_str_lit("help"),
    .arity = MN_FLAG_ARITY_ZERO,
};

static struct mn_flag FLAG_CONFIG = {
    .sflag = mn_str_lit("c"),
    .lflag = mn_str_lit("config"),
    .arity = MN_FLAG_ARITY_ONE,
};

struct mn_flag *MN_FLAG_OPTIONS[] = {
    &FLAG_HELP,
    &FLAG_CONFIG,
    nullptr,
};

// =================================================================================
// Routes

struct mn_status serve_root(struct mn_request req, struct mn_response *const res)
{
  mn_log_info("Serving root");
  return mn_response_write_file(res, mn_str_lit("./public/index.html"));
}

static struct mn_route route_root = {
    .method = MN_METHOD_GET,
    .pattern = mn_str_lit("/"),
    .next = nullptr,
    .callback = serve_root};

// =================================================================================
// Main

int main(int argc, char const *argv[argc])
{
  struct mn_str action = {};
  struct mn_status status = mn_cmdline_parse(argc, argv, &action);
  if (status.error) {
    return EXIT_FAILURE;
  }

  if (action.len == 0 || FLAG_HELP.set) {
    printf(
        "Usage: %s [run]\n"
        "-h                   Print this help description.\n"
        "-c, --config <FILE>  Load an optional config file.\n",
        argv[0]);
    return EXIT_SUCCESS;
  }

  mn_runtime_t *runtime = mn_runtime_create();
  mn_assert(runtime);

  struct mn_config config = {};
  status = mn_config_load(runtime, FLAG_CONFIG.vals[0], &config);
  if (status.error) {
    goto done;
  }

  // Logs emitted earlier are ERRORs so setting now behaves the same as if we
  // were to set it sooner.
  mn_log_set_level(config.log_level);

  if (mn_str_eq(action, mn_str_lit("run"))) {
    struct mn_server server = {
        .runtime = runtime,
        .config = config,
        .route = route_root,
    };
    status = mn_server_run(&server);
  } else {
    status = MN_ERROR_EMIT(MN_ERROR_INVALID_ARG, "Unknown action %s", action.ss);
  }

done:
  mn_runtime_destroy(runtime);
  return status.error ? EXIT_FAILURE : EXIT_SUCCESS;
}
