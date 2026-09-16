#include <signal.h>

#include "nori/server.h"

static sig_atomic_t SERVER_RUNNING = 1;

static void signal_server_stop(int signal)
{
  if (signal == SIGINT) {
    SERVER_RUNNING = 0;
  }
}

struct nori_status nori_server_run(struct nori_config config[static 1])
{
  const struct sigaction act = {.sa_handler = signal_server_stop};
  if (sigaction(SIGINT, &act, nullptr) == -1) {
    return NORI_FAILURE_ERROR("Could not install interrupt handler");
  };

  struct lws_context_creation_info info;
  lws_context_info_defaults(&info, nullptr);

  info.vhost_name = "localhost";
  info.port = config->nc_port;
  info.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS;

  struct lws_context *context = lws_create_context(&info);
  if (!context) {
    return NORI_FAILURE_ERROR("Could not create lws context");
  }

  struct lws_vhost *vh = lws_create_vhost(context, &info);
  if (!vh) {
    return NORI_FAILURE_ERROR("Could not create lws vhost");
  }

  nori_log_notice("Starting server on port %ld", config->nc_port);

  int status = 0;
  while (status >= 0 && SERVER_RUNNING) {
    status = lws_service(context, /* unused */ 0);
  }

  nori_log_notice("Stopping server");
  lws_context_destroy(context);

  return NORI_SUCCESS;
}
