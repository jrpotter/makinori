#include <libwebsockets.h>
#include <signal.h>

#include "nori/server.h"

// ================================================================
// Signaling
// ================================================================

static sig_atomic_t SERVER_RUNNING = 1;

static void signal_server_stop(const int signal)
{
  if (signal == SIGINT) {
    SERVER_RUNNING = 0;
  }
}

// ================================================================
// Processing
// ================================================================

static struct nori_status lws_http_route(
    struct lws *const wsi,
    const struct nori_router route,
    struct nori_request request)
{
  struct nori_response response = {
      .nr_status = HTTP_STATUS_OK,
  };

  struct nori_status status = route.nr_callback(request, &response);

  // TODO: Use the nori_bytes instance to populate the response.

  return status;
}

static int lws_http_callback(
    struct lws *const wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
  struct nori_status status = {};

  const struct lws_protocols *proto = lws_get_protocol(wsi);
  struct nori_server *server = proto->user;

  if (reason == LWS_CALLBACK_HTTP) {
    char path[2048] = {}; // TODO: Return 414 if longer than this
    lws_snprintf(path, sizeof(path) - 1, "%s", (const char *)in);

    struct nori_request request = {
        .nr_path = nori_view_create(path),
    };

    for (struct nori_router *route = &server->router; route; route = route->nr_next) {
      if (nori_view_eq(route->nr_path, request.nr_path)) {
        if (route->nr_callback) {
          status = lws_http_route(wsi, *route, request);
        } else {
          nori_log_warn(
              "No callback registered for path %s",
              request.nr_path.len == 0 ? "<EMPTY>" : request.nr_path.ss);
        }
        break;
      }
    }

    nori_log_warn(
        "No route matched %s",
        request.nr_path.len == 0 ? "<EMPTY>" : request.nr_path.ss);
  }

  return status.success ? 0 : 1;
}

// ================================================================
// API
// ================================================================

struct nori_status nori_server_run(struct nori_server server[static 1])
{
  const struct sigaction act = {.sa_handler = signal_server_stop};
  if (sigaction(SIGINT, &act, nullptr) == -1) {
    return NORI_FAILURE_ERROR("Could not install interrupt handler");
  };

  const struct lws_protocols http_protocol = {
      .name = "http",
      .callback = lws_http_callback,
      .id = 0,
      .user = server,
      .per_session_data_size = 0,
      .rx_buffer_size = 0,
      .tx_packet_size = 0};

  const struct lws_protocols *pprotocols[] = {&http_protocol, nullptr};

  const struct lws_http_mount http_mount = {
      .protocol = "http",
      .mountpoint = "",
      .mountpoint_len = 0,
      .origin_protocol = LWSMPRO_CALLBACK,
  };

  struct lws_context_creation_info info;
  lws_context_info_defaults(&info, nullptr);
  info.vhost_name = "localhost";
  info.pprotocols = pprotocols;
  info.mounts = &http_mount;
  info.port = server->config.nc_port;
  info.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS;

  struct lws_context *context = lws_create_context(&info);
  if (!context) {
    return NORI_FAILURE_ERROR("Could not create lws context");
  }

  struct lws_vhost *vh = lws_create_vhost(context, &info);
  if (!vh) {
    return NORI_FAILURE_ERROR("Could not create lws vhost");
  }

  nori_log_notice("Starting server on port %ld", server->config.nc_port);

  int status = 0;
  while (status >= 0 && SERVER_RUNNING) {
    status = lws_service(context, /* unused */ 0);
  }

  nori_log_notice("Stopping server");
  lws_context_destroy(context);

  return NORI_SUCCESS;
}
