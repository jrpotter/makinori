#include <fcntl.h>
#include <libwebsockets.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>

#include "nori/server.h"
#include "nori/util.h"

constexpr ssize_t BUFFER_OUT = 4096;
static_assert(BUFFER_OUT < SSIZE_MAX, "BUFFER_OUT >= SSIZE_MAX");

// ================================================================
// Signaling
// ================================================================

// TODO: We should have support to be able to run multiple servers behind their
// own thread. In such a multithreaded situation, signaling introduced undefined
// behavior. Can instead setup a keyevent listener in the main poll loop to
// detect something like an interrupt.

static sig_atomic_t SERVER_RUNNING = 1;

static void signal_server_stop(const int signal)
{
  if (signal == SIGINT) {
    SERVER_RUNNING = 0;
  }
}

// ================================================================
// Responses
// ================================================================

struct nori_response {
  // HTTP headers.
  struct nori_str_view nr_content_length;
  struct nori_str_view nr_content_type;
  // HTTP status code.
  unsigned int nr_status;
  // FD of in/out buffers to read/write the response into.
  int nr_fd_in;
  int nr_fd_out;
  // The lws context this response is associated with.
  struct lws *nr_wsi;
  // The return status of the user-defined callback.
  struct nori_status nr_result;
  // Flag indicating the coroutine is finished.
  bool nr_coro_done;
};

struct nori_status nori_response_set_header(
    struct nori_response *const response,
    enum nori_header header,
    struct nori_str_view value)
{
  switch (header) {
  case NORI_HEADER_CONTENT_LENGTH: {
    response->nr_content_length = value;
    break;
  }
  case NORI_HEADER_CONTENT_TYPE: {
    response->nr_content_type = value;
    break;
  }
  }
  return NORI_SUCCESS;
}

struct nori_status
nori_response_set_status(struct nori_response *const response, unsigned int status)
{
  response->nr_status = status;
  return NORI_SUCCESS;
}

// ================================================================
// Coroutines
// ================================================================

const struct nori_route *const nori_route_match(
    const struct nori_server server[const static 1],
    const enum nori_method method,
    const struct nori_str_view path)
{
  for (const struct nori_route *route = &server->router; route;
       route = route->nr_next) {
    nori_log_warn("%s, %s", route->nr_path.view, path.view);
    if (route->nr_method != method) {
      continue;
    }
    if (!nori_str_view_eq(route->nr_path, path)) {
      // TODO: Support more complex route matching.
      continue;
    }
    return route;
  }

  return nullptr;
}

struct nori_pss {
  nori_route_callback_t *nc_callback;
  struct nori_request nc_request;
  struct nori_response nc_response;
  char co_stack[]; // FAM representing the coroutine's stack.
};

// A reference to our main context. Every coroutine should always link back to
// this one.
static thread_local ucontext_t context_main;

// The call to `makecontext` does not permit any arguments but the user-defined
// callback expects the request/response pair introduced in the current
// transaction's PSS. Use this to temporarily hold the value for @coro_wrapper
// to reference.
static thread_local struct nori_pss *coro_pss;

static void coro_wrapper(void)
{
  struct nori_pss *pss = coro_pss;
  coro_pss = nullptr;
  pss->nc_response.nr_result = pss->nc_callback(pss->nc_request, &pss->nc_response);
  pss->nc_response.nr_coro_done = true;
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

    // ----------------------------------------------------------------
    // Init
    // ----------------------------------------------------------------

    const struct lws_protocols *proto = lws_get_protocol(wsi);
    const struct nori_server *const server = proto->user;
    memset(pss, 0, sizeof(struct nori_pss) + server->config.nc_co_stack);

    // ----------------------------------------------------------------
    // Route matching
    // ----------------------------------------------------------------

    enum nori_method request_method = NORI_METHOD_GET;
    if (lws_hdr_total_length(wsi, WSI_TOKEN_GET_URI)) {
      request_method = NORI_METHOD_GET;
    } else if (lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI)) {
      request_method = NORI_METHOD_POST;
    } else {
      nori_log_info("Unmanaged HTTP method");
      break;
    }

    // TODO: Return 414 if longer than the @in_path buffer size.
    char in_path[2048] = {'/'};
    lws_snprintf(in_path + 1, sizeof(in_path) - 1, "%s", (const char *)in);

    const struct nori_route *const route =
        nori_route_match(server, request_method, nori_str_view_of(in_path));

    if (route == nullptr) {
      break;
    }

    if (!route->nr_callback) {
      // TODO: This should return a 500.
      nori_log_warn(
          "No callback registered for path %s",
          route->nr_path.len == 0 ? "<EMPTY>" : route->nr_path.view);
      break;
    }

    // Use the route's values since they live outside this frame.
    pss->nc_callback = route->nr_callback;
    pss->nc_request.nr_method = route->nr_method;
    pss->nc_request.nr_path = route->nr_path;

    // ----------------------------------------------------------------
    // Write back
    // ----------------------------------------------------------------

    int pipefd[2] = {};
    if (pipe2(pipefd, O_NONBLOCK) == -1) {
      // TODO: This should return a 500.
      perror("pipe2");
      nori_log_error("Could not create coroutine");
      break;
    }

    pss->nc_response.nr_content_length = NSV("");
    pss->nc_response.nr_content_type = NSV("text/html");
    pss->nc_response.nr_status = HTTP_STATUS_OK;
    pss->nc_response.nr_fd_in = pipefd[0];
    pss->nc_response.nr_fd_out = pipefd[1];
    pss->nc_response.nr_wsi = wsi;
    pss->nc_response.nr_result = NORI_FAILURE;
    pss->nc_response.nr_coro_done = false;

    // ----------------------------------------------------------------
    // Coroutine setup
    // ----------------------------------------------------------------

    ucontext_t context_coro = {};
    if (getcontext(&context_coro) == -1) {
      // TODO: This should return a 500.
      perror("getcontext");
      nori_log_error("Could not create coroutine");
      break;
    }

    context_coro.uc_stack.ss_sp = pss->co_stack;
    context_coro.uc_stack.ss_size = server->config.nc_co_stack;
    context_coro.uc_link = &context_main;
    makecontext(&context_coro, coro_wrapper, 0);

    coro_pss = pss; // Set before context switch.
    if (swapcontext(&context_main, &context_coro) == -1) {
      // TODO: This should return a 500.
      perror("swapcontext");
      nori_log_error("Could not start coroutine");
      break;
    }

    // ----------------------------------------------------------------
    // Finish
    // ----------------------------------------------------------------

    // Queues HTTP_BODY.* and HTTP_WRITEABLE callbacks.
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

    // ----------------------------------------------------------------
    // Continue HTTP
    // ----------------------------------------------------------------

    if (!pss->nc_response.nr_coro_done) {
      char out[BUFFER_OUT];
      ssize_t count = read(pss->nc_response.nr_fd_out, out, BUFFER_OUT);

      if (count == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("read");
        nori_log_error("Could not read from coroutine pipe");
        return -1;
      }

      if (count == 0) {
        nori_log_error("Coroutine pipe unexpectedly closed");
        return -1;
      }

      lws_callback_on_writable(wsi); // More to read. Queue again.
      return 0;
    }

    // ----------------------------------------------------------------
    // Cleanup Response
    // ----------------------------------------------------------------

    if (close(pss->nc_response.nr_fd_in) == -1) {
      perror("close fd_in");
    }
    if (close(pss->nc_response.nr_fd_out) == -1) {
      perror("close fd_out");
    }

    // ----------------------------------------------------------------
    // Validate Response
    // ----------------------------------------------------------------

    // TODO: Validate the response object.

    // ----------------------------------------------------------------
    // Finish HTTP
    // ----------------------------------------------------------------

    // TODO: Finish the response.

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

// ================================================================
// Entrypoint
// ================================================================

struct nori_status nori_server_run(struct nori_server server[static 1])
{
  const struct sigaction act = {.sa_handler = signal_server_stop};
  if (sigaction(SIGINT, &act, nullptr) == -1) {
    perror("sigaction");
    nori_log_error("Could not install interrupt handler");
    return NORI_FAILURE;
  };

  const struct lws_protocols http_protocol = {
      .name = "http",
      .callback = lws_http_callback,
      .id = 0,
      .user = server,
      .per_session_data_size = sizeof(struct nori_pss) + server->config.nc_co_stack,
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
    nori_log_error("Could not create lws context");
    return NORI_FAILURE;
  }

  struct lws_vhost *vh = lws_create_vhost(context, &info);
  if (!vh) {
    nori_log_error("Could not create lws vhost");
    return NORI_FAILURE;
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
