#include <assert.h>
#include <stdlib.h>

#include "nori/server.h"

// ================================================================
// Command Line
// ================================================================

enum action {
  ACTION_NONE,
  ACTION_HELP,
  ACTION_RUN,
};

struct user_args {
  enum action user_action;
  struct nori_view user_config; // -c, --config
};

struct flag {
  struct nori_view flag_short;
  struct nori_view flag_long;
  unsigned short flag_arity;
};

static constexpr struct flag FLAG_HELP = {
    .flag_short = NORI_VIEW("-h"),
    .flag_long = NORI_VIEW("--help"),
    .flag_arity = 0,
};

static constexpr struct flag FLAG_CONFIG = {
    .flag_short = NORI_VIEW("-c"),
    .flag_long = NORI_VIEW("--config"),
    .flag_arity = 1,
};

static const struct flag *FLAG_OPTIONS[] = {
    &FLAG_HELP,
    &FLAG_CONFIG,
};

static const struct flag *flag_match(struct nori_view key)
{
  for (size_t i = 0; i < sizeof(FLAG_OPTIONS) / sizeof(FLAG_OPTIONS[0]); ++i) {
    if (nori_view_eq(key, FLAG_OPTIONS[i]->flag_short) ||
        nori_view_eq(key, FLAG_OPTIONS[i]->flag_long)) {
      return FLAG_OPTIONS[i];
    }
  }
  return nullptr;
}

static struct nori_status cmdline_parse(
    const int argc,
    const char *argv[argc],
    struct user_args out[const static 1])
{
  memset(out, 0, sizeof(struct user_args));
  out->user_action = ACTION_NONE;

  // The binary assumes a single positional argument defines the action to take. This
  // could be "run" for running the server, "migrate" for running database migrations,
  // etc. Look for this action first since it may dictate what flags are permitted.
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      const struct nori_view key = nori_view_create(argv[i]);
      const struct flag *flag = flag_match(key);
      if (flag) {
        i += flag->flag_arity;
      } else {
        return NORI_FAILURE_ERROR("Unknown flag %s", key.ss);
      }
    } else {
      struct nori_view action = nori_view_create(argv[i]);
      enum action new_action = ACTION_NONE;
      if (nori_view_eq(action, NORI_VIEW("run"))) {
        new_action = ACTION_RUN;
      } else {
        return NORI_FAILURE_ERROR("Unknown action %s", action.ss);
      }
      if (out->user_action == ACTION_NONE) {
        out->user_action = new_action;
      } else {
        return NORI_FAILURE_ERROR("Cannot specify > 1 action");
      }
    }
  }

  // As a special case, if no action is supplied, assume the user needs help.
  if (out->user_action == ACTION_NONE) {
    out->user_action = ACTION_HELP;
    return NORI_SUCCESS;
  }

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] != '-') {
      continue;
    }

    const struct nori_view key = nori_view_create(argv[i]);
    const struct flag *flag = flag_match(key);
    assert(flag); // checked in previous loop

    if (flag == &FLAG_HELP) {
      out->user_action = ACTION_HELP;
      return NORI_SUCCESS;
    }

    if (i + flag->flag_arity >= argc) {
      return NORI_FAILURE_ERROR("Missing values for '%s'\n", key.ss);
    }

    struct nori_view val = nori_view_create(argv[i + 1]);
    if (flag == &FLAG_CONFIG) {
      out->user_config = val;
    } else {
      return NORI_FAILURE_ERROR("Unknown flag '%s'\n", key.ss);
    }
  }

  return NORI_SUCCESS;
}

// ================================================================
// Router
// ================================================================

static struct nori_router route_static = {
    .nr_method = NORI_METHOD_GET,
    .nr_path = NORI_VIEW("/static"),
    .nr_next = nullptr};

struct nori_status
serve_root(struct nori_request request, struct nori_response *const response)
{
  return NORI_SUCCESS;
}

static struct nori_router route_root = {
    .nr_method = NORI_METHOD_POST,
    .nr_path = NORI_VIEW("/"),
    .nr_next = &route_static,
    .nr_callback = serve_root};

// ================================================================
// Main
// ================================================================

int main(int argc, const char *argv[argc])
{
  struct nori_status status = {};

#ifdef NDEBUG
  nori_set_log_level(INIT_LOG_LEVEL);
#else
  nori_set_log_level(INIT_LOG_LEVEL | LLL_DEBUG | LLL_INFO);
#endif

  struct user_args args = {};
  status = cmdline_parse(argc, argv, &args);
  if (!status.success) {
    return EXIT_FAILURE;
  }

  if (args.user_action == ACTION_HELP) {
    printf(
        "Usage: %s [run]\n"
        "-h                   Print this help description.\n"
        "-c, --config <FILE>  Load an optional config file.\n",
        argv[0]);
    return EXIT_SUCCESS;
  }

  struct nori_config config = {};
  status = nori_config_load(args.user_config, &config);
  if (!status.success) {
    goto cleanup;
  }

  nori_set_log_level(config.nc_log);

  struct nori_server server = {
      .config = config,
      .router = route_root,
  };

  if (args.user_action == ACTION_RUN) {
    status = nori_server_run(&server);
  }

cleanup:
  nori_config_unload(&config);
  return status.success ? EXIT_SUCCESS : EXIT_FAILURE;
}
