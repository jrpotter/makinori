#include <assert.h>
#include <stdlib.h>

#include "nori.h"

// ================================================================
// Command Line
// ================================================================

static struct nori_flag FLAG_HELP = {
    .nf_sflag = SS("h"),
    .nf_lflag = SS("help"),
    .nf_arity = NORI_FLAG_ARITY_ZERO,
};

static struct nori_flag FLAG_CONFIG = {
    .nf_sflag = SS("c"),
    .nf_lflag = SS("config"),
    .nf_arity = NORI_FLAG_ARITY_ONE,
};

struct nori_flag *NORI_FLAG_OPTIONS[] = {
    &FLAG_HELP,
    &FLAG_CONFIG,
    nullptr,
};

// ================================================================
// Router
// ================================================================

static struct nori_router route_static = {
    .nr_method = NORI_METHOD_GET,
    .nr_path = SS("/static"),
    .nr_next = nullptr};

struct nori_status
serve_root(struct nori_request request, struct nori_response *const response)
{
  return NORI_SUCCESS;
}

static struct nori_router route_root = {
    .nr_method = NORI_METHOD_POST,
    .nr_path = SS("/"),
    .nr_next = &route_static,
    .nr_callback = serve_root};

// ================================================================
// Main
// ================================================================

int main(int argc, const char *argv[argc])
{
  struct nori_status status = nori_init();
  if (!status.ns_success) {
    return EXIT_FAILURE;
  }

  struct nori_slice action = {};
  status = nori_cmdline_parse(argc, argv, &action);
  if (!status.ns_success) {
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
  if (!status.ns_success) {
    goto cleanup;
  }

  if (nori_slice_eq(action, SS("run"))) {
    struct nori_server server = {
        .config = config,
        .router = route_root,
    };
    status = nori_server_run(&server);
  } else {
    status = NORI_FAILURE_ERROR("Unknown action %s", action.ss);
  }

cleanup:
  nori_config_unload(&config);
  return status.ns_success ? EXIT_SUCCESS : EXIT_FAILURE;
}
