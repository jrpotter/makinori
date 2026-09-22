#include <fcntl.h>
#include <libwebsockets.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>

#include "nori/internal/response.h"
#include "nori/logger.h"
#include "nori/server.h"
#include "nori/util.h"

// =================================================================================
// Signaling

// TODO: We should add support for running multiple servers, each behind their
// own thread. In such a multithreaded situation, signaling introduced undefined
// behavior. Can instead setup a keyevent listener in the main poll loop to
// detect something like an interrupt.

static sig_atomic_t volatile SERVER_RUNNING = 1;

static void server_signal_handler(int const signal)
{
  switch (signal) {
  case SIGINT: {
    SERVER_RUNNING = 0;
    break;
  }
  }
}

// =================================================================================
// Coroutines

struct nori_pss {
  nori_route_callback_t *nc_callback;
  struct nori_request nc_request;
  struct nori_response nc_response; // Keep last for FAM.
};

static thread_local struct nori_pss *coro_arg;

// Entrypoint for the coroutine. Arguments, if provided, must be `int`s which
// may or may not be large enough to hold a pointer. As a workaround, use the
// coroutine_arg variable.
static void coro_start(void)
{
  struct nori_pss *pss = coro_arg;
  coro_arg = nullptr;

  struct nori_response *const res = &pss->nc_response;
  res->nr_status = pss->nc_callback(pss->nc_request, res);

  // Finish transitioning through the state machine. It's possible the callback
  // does nothing at all in which case we are at the first state. We should
  // always hit the .*_BODY condition since no function can be called from the
  // callback that transitions beyond .*_BODY.

  if (res->nr_state == NORI_RESPONSE_STATE_HEADER) {
    nori_trace(NORI_TRACE_RESPONSE_STATE, "(%p) HEADER -> HEADER_FLUSH", (void *){res});
    res->nr_state = NORI_RESPONSE_STATE_HEADER_FLUSH;
    struct nori_status status = nori_response_suspend(res);
    if (status.ns_error) {
      nori_log_error("Could not suspend coroutine");
      return;
    }
  }

  if (res->nr_state == NORI_RESPONSE_STATE_HEADER_FLUSH) {
    struct nori_status status = nori_response_suspend(res);
    if (status.ns_error) {
      nori_log_error("Could not suspend coroutine");
      return;
    }
  }

  if (res->nr_state == NORI_RESPONSE_STATE_BODY) {
    nori_trace(NORI_TRACE_RESPONSE_STATE, "(%p) BODY -> BODY_FLUSH", (void *){res});
    res->nr_state = NORI_RESPONSE_STATE_BODY_FLUSH;
    struct nori_status status = nori_response_suspend(res);
    if (status.ns_error) {
      nori_log_error("Could not suspend coroutine");
      return;
    }
  }

  if (res->nr_state == NORI_RESPONSE_STATE_BODY_FLUSH) {
    struct nori_status status = nori_response_suspend(res);
    if (status.ns_error) {
      nori_log_error("Could not suspend coroutine");
      return;
    }
  }

  nori_trace(NORI_TRACE_RESPONSE_STATE, "(%p) FINISHED", (void *){res});
}

// =================================================================================
// Server

struct nori_route const *const nori_route_match(
    struct nori_server const server[const static 1],
    enum nori_method const method,
    struct nori_str_view const path)
{
  for (struct nori_route const *route = &server->router; route;
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

static struct nori_status nori_write_lws_common(
    struct lws *const wsi,
    struct nori_response *const r,
    unsigned char **p,
    unsigned char *end)
{
  unsigned int code = NORI_HTTP_CODE_OK;
  if (r->nr_common_code != 0) {
    code = r->nr_common_code;
  }

  char const *content_type = "text/html";
  if (r->nr_common_type.len > 0) {
    content_type = r->nr_common_type.view;
  }

  // TODO: Actually use content response content length.
  lws_filepos_t content_len = LWS_ILLEGAL_HTTP_CONTENT_LEN;

  if (lws_add_http_common_headers(wsi, code, content_type, content_len, p, end)) {
    return NORI_ERROR_EMIT(NORI_ERROR_SYSTEM, "Could not write common headers");
  }

  return NORI_SUCCESS;
}

// LWS's documentation is...lacking. From what I can tell, any nonzero value
// indicates closing the connection.
int constexpr LWS_CONTINUE = 0;
int constexpr LWS_CLOSE = -1;

static void nori_trace_callback(enum lws_callback_reasons reason)
{
  // Ordered in roughly the order the callbacks are triggered.
  switch (reason) {
  case LWS_CALLBACK_WSI_CREATE: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_WSI_CREATE");
    break;
  }
  case LWS_CALLBACK_PROTOCOL_INIT: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_PROTOCOL_INIT");
    break;
  }
  case LWS_CALLBACK_FILTER_NETWORK_CONNECTION: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_FILTER_NETWORK_CONNECTION");
    break;
  }
  case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED");
    break;
  }
  case LWS_CALLBACK_EVENT_WAIT_CANCELLED: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_EVENT_WAIT_CANCELLED");
    break;
  }
  case LWS_CALLBACK_FILTER_HTTP_CONNECTION: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_FILTER_HTTP_CONNECTION");
    break;
  }
  case LWS_CALLBACK_HTTP_BIND_PROTOCOL: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_HTTP_BIND_PROTOCOL");
    break;
  }
  case LWS_CALLBACK_CHECK_ACCESS_RIGHTS: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_CHECK_ACCESS_RIGHTS");
    break;
  }
  case LWS_CALLBACK_HTTP: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_HTTP");
    break;
  }
  case LWS_CALLBACK_HTTP_BODY: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_HTTP_BODY");
    break;
  }
  case LWS_CALLBACK_HTTP_BODY_COMPLETION: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_HTTP_BODY_COMPLETION");
    break;
  }
  case LWS_CALLBACK_HTTP_WRITEABLE: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_HTTP_WRITEABLE");
    break;
  }
  case LWS_CALLBACK_HTTP_DROP_PROTOCOL: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_HTTP_DROP_PROTOCOL");
    break;
  }
  case LWS_CALLBACK_CLOSED_HTTP: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_CLOSED_HTTP");
    break;
  }
  case LWS_CALLBACK_PROTOCOL_DESTROY: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_PROTOCOL_DESTROY");
    break;
  }
  case LWS_CALLBACK_WSI_DESTROY: {
    nori_trace(NORI_TRACE_LWS_CALLBACK, "LWS_CALLBACK_WSI_DESTROY");
    break;
  }
  default: {
    nori_log_warn("Unmanaged callback %u", reason);
    break;
  }
  }
}

static int lws_http_callback(
    struct lws *const wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
  struct nori_pss *pss = user;
  struct nori_request *const req = &pss->nc_request;
  struct nori_response *const res = &pss->nc_response;

  nori_trace_callback(reason);

  // Ordered in roughly the same order the callbacks are triggered.
  switch (reason) {
  case LWS_CALLBACK_HTTP: {
    struct lws_protocols const *proto = lws_get_protocol(wsi);
    struct nori_server const *const server = proto->user;
    memset(pss, 0, sizeof(struct nori_pss) + server->config.nc_co_stack);

    // Resources that need to be potentially cleaned up on error.
    int pipefd[2] = {};

    { // --- Setup the request. ----------------------------------------------------
      enum nori_method request_method = NORI_METHOD_GET;
      if (lws_hdr_total_length(wsi, WSI_TOKEN_GET_URI)) {
        request_method = NORI_METHOD_GET;
      } else { // TODO: Return a 500
        nori_log_error("Unmanaged HTTP method");
        goto lws_callback_http_cleanup;
      }

      char in_path[2048] = {'/'}; // TODO: Return 414 if longer than buffer.
      lws_snprintf(in_path + 1, sizeof(in_path) - 1, "%s", (char const *)in);

      struct nori_route const *const route =
          nori_route_match(server, request_method, nori_str_view_of(in_path));

      if (route == nullptr) { // TODO: This should return a 404.
        goto lws_callback_http_cleanup;
      }

      if (!route->nr_callback) { // TODO: This should return a 500.
        nori_log_error(
            "No callback registered for path %s",
            route->nr_path.len == 0 ? "<EMPTY>" : route->nr_path.view);
        goto lws_callback_http_cleanup;
      }

      // Use the route's values since they live outside this frame.
      pss->nc_callback = route->nr_callback;
      req->nr_method = route->nr_method;
      req->nr_path = route->nr_path;
    }

    { // --- Setup the response. ---------------------------------------------------
      if (pipe2(pipefd, O_NONBLOCK) == -1) {
        // TODO: This should return a 500.
        perror("[lws_callback_http,pipe2] ret:-1");
        nori_log_error("Could not create coroutine");
        goto lws_callback_http_cleanup;
      }

      res->nr_wsi = wsi;
      res->nr_common_code = 0;
      res->nr_common_type = NSV("");
      res->nr_common_length = NSV("");
      res->nr_common_flushed = false;
      res->nr_pending_headers_count = 0;
      // r->nr_pending_headers = {};
      res->nr_fd_read = pipefd[0];
      res->nr_fd_write = pipefd[1];
      res->nr_status = NORI_FAILURE(NORI_ERROR_SYSTEM);
      // res->nr_context = {};
      res->nr_state = NORI_RESPONSE_STATE_HEADER;
    }

    { // --- Setup the coroutine. --------------------------------------------------

      // Saves the content of the registers, signal mask, and the stack.
      if (getcontext(&res->nr_context) == -1) { // TODO: Return a 500.
        perror("[lws_callback_http,getcontext] ret:-1");
        nori_log_error("Could not get coroutine");
        goto lws_callback_http_cleanup;
      }

      sigemptyset(&res->nr_context.uc_sigmask);
      res->nr_context.uc_stack.ss_sp = res->nr_co_stack;
      res->nr_context.uc_stack.ss_size = server->config.nc_co_stack;
      res->nr_context.uc_link = &context_server;
      makecontext(&res->nr_context, coro_start, 0);
    }

    { // --- Trigger first context switch. -----------------------------------------
      coro_arg = pss; // Set before context switch.
      res->nr_status = nori_response_resume(res);
      if (res->nr_status.ns_error) {
        nori_log_error("Could not resume coroutine");
        goto lws_callback_http_cleanup;
      }
    }

    { // --- Queue BODY and WRITEABLE callbacks. -----------------------------------
      lws_callback_on_writable(wsi);
    }

    return LWS_CONTINUE;

  lws_callback_http_cleanup:
    if (pipefd[0] && close(pipefd[0]) == -1) {
      perror("[lws_callback_http,pipefd[0]] close:-1");
    }
    if (pipefd[1] && close(pipefd[1]) == -1) {
      perror("[lws_callback_http,pipefd[1]] close:-1");
    }
    return LWS_CLOSE;
  }

  case LWS_CALLBACK_HTTP_BODY: {
    // TODO: If POST, pull content.
    break;
  }

  case LWS_CALLBACK_HTTP_BODY_COMPLETION: {
    // TODO: If POST, pull content.
    break;
  }

  case LWS_CALLBACK_HTTP_WRITEABLE: {
    // If the user-defined callback returns failure, we close the connection.
    // Unfortunately HTTP/1.0 cannot distinguish between a completed response
    // and a failure. At least with HTTP/1.1 and HTTP/2, there will be no
    // terminating chunk/frame so the client knows something happened.
    if (res->nr_status.ns_error) {
      return LWS_CLOSE;
    }
    // The event loop may trigger spurious writeable callbacks for internal
    // reasons. If our status is failed or state is closed, then we have already
    // cleaned up resources and there should be nothing left to do.
    if (res->nr_state == NORI_RESPONSE_STATE_CLOSED) {
      return LWS_CLOSE;
    }

    uint8_t buffer[NORI_RESPONSE_BODY_THRESHOLD]; // HTTP does not need LWS_PRE
    uint8_t *start = buffer;
    uint8_t *p = start;
    uint8_t *end = buffer + sizeof(buffer) - 1;

    switch (res->nr_state) {
    case NORI_RESPONSE_STATE_HEADER:
    case NORI_RESPONSE_STATE_HEADER_FLUSH: {
      { // --- Check if LWS "common" fields should be written. ---------------------
        if (!res->nr_common_flushed &&
            (res->nr_state == NORI_RESPONSE_STATE_HEADER_FLUSH ||
             (res->nr_common_code != 0 && res->nr_common_type.len > 0 &&
              res->nr_common_length.len > 0))) {
          res->nr_status = nori_write_lws_common(wsi, res, &p, end);
          if (res->nr_status.ns_error) {
            nori_log_error("Could not write common headers");
            goto lws_callback_http_writeable_cleanup;
          }
          res->nr_common_flushed = true;
        }
      }

      { // --- Flush any pending headers. ------------------------------------------
        for (size_t i = 0; i < res->nr_pending_headers_count; ++i) {
          struct nori_str_view header = res->nr_pending_headers[i].nr_key;
          struct nori_str_view value = res->nr_pending_headers[i].nr_val;
          if (lws_add_http_header_by_name(
                  wsi, (unsigned char const *)header.view,
                  (unsigned char const *)value.view, value.len, &p, end)) {
            res->nr_status = NORI_ERROR_EMIT(
                NORI_ERROR_SYSTEM, "Could not write header %s", header.view);
            goto lws_callback_http_writeable_cleanup;
          }
        }
        res->nr_pending_headers_count = 0;
      }

      { // --- Advance the state machine. ------------------------------------------
        if (res->nr_state == NORI_RESPONSE_STATE_HEADER_FLUSH) {
          if (lws_finalize_write_http_header(wsi, start, &p, end)) {
            res->nr_status =
                NORI_ERROR_EMIT(NORI_ERROR_SYSTEM, "Could not finalize http headers");
            goto lws_callback_http_writeable_cleanup;
          }
          nori_trace(
              NORI_TRACE_RESPONSE_STATE, "(%p) HEADER_FLUSH -> BODY", (void *){res});
          res->nr_state = NORI_RESPONSE_STATE_BODY;
        }
      }

      { // --- Resume the coroutine. -----------------------------------------------
        res->nr_status = nori_response_resume(res);
        if (res->nr_status.ns_error) {
          nori_log_error("Could not resume coroutine");
          goto lws_callback_http_writeable_cleanup;
        }
      }

      { // --- Queue up another LWS_CALLBACK_HTTP_WRITEABLE. -----------------------
        lws_callback_on_writable(wsi);
      }

      return LWS_CONTINUE;
    }
    case NORI_RESPONSE_STATE_BODY:
    case NORI_RESPONSE_STATE_BODY_FLUSH: {
      { // --- Attempt to read HTTP content. ---------------------------------------
        ssize_t n = read(res->nr_fd_read, p, NORI_RESPONSE_BODY_THRESHOLD);

        if (n == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
          perror("[lws_callback_http_writable,read] ret:-1");
          res->nr_status =
              NORI_ERROR_EMIT(NORI_ERROR_SYSTEM, "Could not read from coroutine");
          goto lws_callback_http_writeable_cleanup;
        }

        if (n == 0) {
          res->nr_status =
              NORI_ERROR_EMIT(NORI_ERROR_SYSTEM, "Coroutine pipe unexpected closed");
          goto lws_callback_http_writeable_cleanup;
        }

        if (n > 0) {
          p += n;
          if (lws_write(wsi, start, lws_ptr_diff_size_t(p, start), LWS_WRITE_HTTP) !=
              lws_ptr_diff(p, start)) {
            res->nr_status =
                NORI_ERROR_EMIT(NORI_ERROR_SYSTEM, "Could not write to body");
            goto lws_callback_http_writeable_cleanup;
          }
          lws_callback_on_writable(wsi); // Immediately re-enter event loop.
          return LWS_CONTINUE;
        }
      }

      { // --- Advance the state machine. ------------------------------------------
        if (res->nr_state == NORI_RESPONSE_STATE_BODY_FLUSH) {
          if (lws_write(
                  wsi, start, lws_ptr_diff_size_t(p, start), LWS_WRITE_HTTP_FINAL) !=
              lws_ptr_diff(p, start)) {
            res->nr_status =
                NORI_ERROR_EMIT(NORI_ERROR_SYSTEM, "Could not write to body");
            goto lws_callback_http_writeable_cleanup;
          }
          nori_trace(
              NORI_TRACE_RESPONSE_STATE, "(%p) BODY_FLUSH -> CLOSING", (void *){res});
          res->nr_state = NORI_RESPONSE_STATE_CLOSING;
        }
      }

      { // --- Resume the coroutine. -----------------------------------------------
        res->nr_status = nori_response_resume(res);
        if (res->nr_status.ns_error) {
          nori_log_error("Could not resume coroutine");
          goto lws_callback_http_writeable_cleanup;
        }
      }

      { // --- Queue up another LWS_CALLBACK_HTTP_WRITEABLE. -----------------------
        lws_callback_on_writable(wsi);
      }

      return LWS_CONTINUE;
    }
    case NORI_RESPONSE_STATE_CLOSING: {
      if (lws_http_transaction_completed(wsi)) {
        nori_trace(NORI_TRACE_RESPONSE_STATE, "(%p) CLOSING -> CLOSED", (void *){res});
        res->nr_state = NORI_RESPONSE_STATE_CLOSED;
      } else {
        // Otherwise the connection remains open. LWS is responsible for
        // logically separating each transaction over the connection and thereby
        // resetting our state machine.
      }
      [[fallthrough]];
    }
    case NORI_RESPONSE_STATE_CLOSED: {
      break;
    }
    }

  lws_callback_http_writeable_cleanup:
    nori_assert(res->nr_fd_read);
    if (res->nr_fd_read && close(res->nr_fd_read) == -1) {
      perror("[lws_callback_http_writeable,fd_read] close:-1");
    }
    nori_assert(res->nr_fd_write);
    if (res->nr_fd_write && close(res->nr_fd_write) == -1) {
      perror("[lws_callback_http_writeable,fd_write] close:-1");
    }
    res->nr_fd_read = 0;
    res->nr_fd_write = 0;

    if (res->nr_status.ns_error || res->nr_state == NORI_RESPONSE_STATE_CLOSED) {
      return LWS_CLOSE;
    }
    return LWS_CONTINUE;
  }

  default: {
    break;
  }
  }

  // TODO: Add a better 404 handler.
  return lws_callback_http_dummy(wsi, reason, user, in, len);
}

// =================================================================================
// Entrypoint

struct nori_status nori_server_run(struct nori_server server[static 1])
{
  struct sigaction const act = {.sa_handler = server_signal_handler};
  if (sigaction(SIGINT, &act, nullptr) == -1) {
    perror("nori_server_run;sigaction");
    return NORI_ERROR_EMIT(NORI_ERROR_SYSTEM, "Could not install signal handler");
  };

  struct lws_protocols const http_protocol = {
      .name = "http",
      .callback = lws_http_callback,
      .id = 0,
      .user = server,
      .per_session_data_size = sizeof(struct nori_pss) + server->config.nc_co_stack,
      .rx_buffer_size = 0,
      .tx_packet_size = 0};

  struct lws_protocols const *pprotocols[] = {&http_protocol, nullptr};

  struct lws_http_mount const http_mount = {
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
    return NORI_ERROR_EMIT(NORI_ERROR_SYSTEM, "Could not create lws context");
  }

  struct lws_vhost *vh = lws_create_vhost(context, &info);
  if (!vh) {
    return NORI_ERROR_EMIT(NORI_ERROR_SYSTEM, "Could not create lws vhost");
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
