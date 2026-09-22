#include <stdio.h>

#include "nori.h"

// =================================================================================
// Command Line

static struct nori_flag FLAG_HELP = {
    .nf_sflag = NSV("h"),
    .nf_lflag = NSV("help"),
    .nf_arity = NORI_FLAG_ARITY_ZERO,
};

static struct nori_flag FLAG_CONFIG = {
    .nf_sflag = NSV("c"),
    .nf_lflag = NSV("config"),
    .nf_arity = NORI_FLAG_ARITY_ONE,
};

struct nori_flag *NORI_FLAG_OPTIONS[] = {
    &FLAG_HELP,
    &FLAG_CONFIG,
    nullptr,
};

// =================================================================================
// Router

struct nori_status
serve_static(struct nori_request req, struct nori_response *const res)
{
  nori_log_info("Serving static");
  return NORI_SUCCESS;
}

static struct nori_route route_static = {
    .nr_method = NORI_METHOD_GET,
    .nr_path = NSV("/static"),
    .nr_next = nullptr,
    .nr_callback = serve_static};

struct nori_status serve_root(struct nori_request req, struct nori_response *const res)
{
  nori_log_info("Serving root");
  return nori_response_write_file(res, NSV("./public/index.html"));
}

static struct nori_route route_root = {
    .nr_method = NORI_METHOD_GET,
    .nr_path = NSV("/"),
    .nr_next = &route_static,
    .nr_callback = serve_root};

// =================================================================================
// Main

int main(int argc, char const *argv[argc])
{
  struct nori_str_view action = {};
  struct nori_status status = nori_cmdline_parse(argc, argv, &action);
  if (status.ns_error) {
    return EXIT_FAILURE;
  }

  if (action.len == 0 || FLAG_HELP.nf_set) {
    printf(
        "Usage: %s [run]\n"
        "-h                   Print this help description.\n"
        "-c, --config <FILE>  Load an optional config file.\n",
        argv[0]);
    return EXIT_SUCCESS;
  }

  struct nori_config config = {};
  status = nori_config_load(FLAG_CONFIG.nf_vals[0], &config);
  if (status.ns_error) {
    goto done;
  }

  // Logs emitted earlier are ERRORs so setting now behaves the same as if we
  // were to set it sooner.
  nori_log_set_level(config.nc_log_level);

  if (nori_str_view_eq(action, NSV("run"))) {
    struct nori_server server = {
        .config = config,
        .router = route_root,
    };
    status = nori_server_run(&server);
  } else {
    status = NORI_ERROR_EMIT(NORI_ERROR_INVALID_ARG, "Unknown action %s", action.view);
  }

done:
  return status.ns_error ? EXIT_FAILURE : EXIT_SUCCESS;
}
