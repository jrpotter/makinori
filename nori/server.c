#include <libwebsockets.h>
#include <signal.h>

#include "nori/server.h"
#include "nori/util.h"

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

struct nori_pss {
  struct nori_request nc_request;
  struct nori_router nc_route;
  struct nori_response nc_response;
};

const struct nori_router *const nori_router_match(
    const struct nori_server server[const static 1],
    const struct nori_request request)
{
  for (const struct nori_router *route = &server->router; route;
       route = route->nr_next) {
    nori_log_warn("%s, %s", route->nr_path.ss, request.nr_path.ss);
    if (route->nr_method != request.nr_method) {
      continue;
    }
    if (!nori_view_eq(route->nr_path, request.nr_path)) {
      // TODO: Support more complex route matching.
      continue;
    }
    return route;
  }

  return nullptr;
}

static int lws_http_callback(
    struct lws *const wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
  struct nori_pss *pss = user;

  // Ordered in roughly the order the callbacks are triggered.
  switch (reason) {
  case LWS_CALLBACK_WSI_CREATE: {
    nori_log_debug("LWS_CALLBACK_WSI_CREATE");
    break;
  }
  case LWS_CALLBACK_PROTOCOL_INIT: {
    nori_log_debug("LWS_CALLBACK_PROTOCOL_INIT");
    break;
  }
  case LWS_CALLBACK_FILTER_NETWORK_CONNECTION: {
    nori_log_debug("LWS_CALLBACK_FILTER_NETWORK_CONNECTION");
    break;
  }
  case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED: {
    nori_log_debug("LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED");
    break;
  }
  case LWS_CALLBACK_EVENT_WAIT_CANCELLED: {
    nori_log_debug("LWS_CALLBACK_EVENT_WAIT_CANCELLED");
    break;
  }
  case LWS_CALLBACK_FILTER_HTTP_CONNECTION: {
    nori_log_debug("LWS_CALLBACK_FILTER_HTTP_CONNECTION");
    break;
  }
  case LWS_CALLBACK_HTTP_BIND_PROTOCOL: {
    nori_log_debug("LWS_CALLBACK_HTTP_BIND_PROTOCOL");
    break;
  }
  case LWS_CALLBACK_CHECK_ACCESS_RIGHTS: {
    nori_log_debug("LWS_CALLBACK_CHECK_ACCESS_RIGHTS");
    break;
  }
  case LWS_CALLBACK_HTTP: {
    nori_log_debug("LWS_CALLBACK_HTTP");

    memset(pss, 0, sizeof(struct nori_pss));

    if (lws_hdr_total_length(wsi, WSI_TOKEN_GET_URI)) {
      pss->nc_request.nr_method = NORI_METHOD_GET;
    } else if (lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI)) {
      pss->nc_request.nr_method = NORI_METHOD_POST;
    } else {
      nori_log_info("Unmanaged HTTP method");
      break;
    }

    char path[2048] = {'/'}; // TODO: Return 414 if longer than this
    lws_snprintf(path + 1, sizeof(path) - 1, "%s", (const char *)in);
    pss->nc_request.nr_path = nori_view_create(path); // TODO: This'll be lost.

    const struct lws_protocols *proto = lws_get_protocol(wsi);
    const struct nori_server *const server = proto->user;
    const struct nori_router *const route = nori_router_match(server, pss->nc_request);

    if (route == nullptr) {
      break;
    }

    if (!route->nr_callback) {
      nori_log_warn(
          "No callback registered for path %s",
          route->nr_path.len == 0 ? "<EMPTY>" : route->nr_path.ss);
      break;
    }

    pss->nc_route = *route;

    lws_callback_on_writable(wsi);

    return 0;
  }
  case LWS_CALLBACK_HTTP_BODY: {
    nori_log_debug("LWS_CALLBACK_HTTP_BODY");
    // TODO: If POST, pull content.
    break;
  }
  case LWS_CALLBACK_HTTP_BODY_COMPLETION: {
    nori_log_debug("LWS_CALLBACK_HTTP_BODY_COMPLETION");
    // TODO: If POST, pull content.
    break;
  }
  case LWS_CALLBACK_HTTP_WRITEABLE: {
    nori_log_debug("LWS_CALLBACK_HTTP_WRITEABLE");

    break;
  }
  case LWS_CALLBACK_HTTP_DROP_PROTOCOL: {
    nori_log_debug("LWS_CALLBACK_HTTP_DROP_PROTOCOL");
    break;
  }
  case LWS_CALLBACK_CLOSED_HTTP: {
    nori_log_debug("LWS_CALLBACK_CLOSED_HTTP");
    break;
  }
  case LWS_CALLBACK_PROTOCOL_DESTROY: {
    nori_log_debug("LWS_CALLBACK_PROTOCOL_DESTROY");
    break;
  }
  case LWS_CALLBACK_WSI_DESTROY: {
    nori_log_debug("LWS_CALLBACK_WSI_DESTROY");
    break;
  }
  default: {
    nori_log_info("Unmanaged callback %u", reason);
    break;
  }
  }

  // TODO: Add a better 404 handler.
  return lws_callback_http_dummy(wsi, reason, user, in, len);
}

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
      .per_session_data_size = sizeof(struct nori_pss),
      .rx_buffer_size = 0,
      .tx_packet_size = 0};

  const struct lws_protocols *pprotocols[] = {&http_protocol, nullptr};

  const struct lws_http_mount http_mount = {
      .protocol = "http",
      .mountpoint = "/",
      .mountpoint_len = 1,
      .origin_protocol = LWSMPRO_CALLBACK,
  };

  struct lws_context_creation_info info;
  lws_context_info_defaults(&info, nullptr);
  info.mounts = &http_mount;
  info.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
  info.port = server->config.nc_port;
  info.pprotocols = pprotocols;
  info.server_string = "maki";
  info.vhost_name = "localhost";

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
