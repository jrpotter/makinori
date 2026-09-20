#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>

#include "nori/server.h"
#include "nori/util.h"

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
// Coroutines
// ================================================================

// A response must be written serially, starting with headers before writing any
// of the body. These states indicate where the user-defined callback is.
enum nori_response_state {
  // Initial state indicating the user is writing headers.
  // Set by the main context.
  NORI_RESPONSE_STATE_HEADER,
  // State indicating the user is about to write to the body. Pending headers
  // should be flushed at this point.
  // Set by the user context.
  NORI_RESPONSE_STATE_HEADER_FLUSH,
  // State indicating the user is writing to the body.
  // Set by the main context.
  NORI_RESPONSE_STATE_BODY,
  // State indicating the user is finished. Pending contents should be flushed
  // at this point.
  // Set by the user context.
  NORI_RESPONSE_STATE_BODY_FLUSH,
  // State indicating the user-defined callback has finished.
  // Set by the main context.
  NORI_RESPONSE_STATE_CLOSING,
  // State indicating the user-defined callback has finished.
  // Set by the main context.
  NORI_RESPONSE_STATE_CLOSED,
};

constexpr size_t MAX_PENDING_HEADERS = 8;

struct nori_response {
  // The lws context this response is associated with.
  struct lws *nr_wsi;
  // The docstring for `lws_add_http_common_headers` indicates it should
  // be replaceable using just the LWS public API, but it isn't clear
  // how to do so. This utility seems to update private state that other
  // functions do not touch. As a workaround, save the fields needed by
  // `lws_add_http_common_headers` separately.
  enum nori_http_code nr_common_code;
  struct nori_str_view nr_common_type;
  struct nori_str_view nr_common_length;
  bool nr_common_flushed;
  // A reference to the HTTP header that needs to be written out. Switch back to
  // the main context when this buffer is full so we can flush it.
  size_t nr_pending_headers_count;
  struct {
    struct nori_str_view nr_key;
    struct nori_str_view nr_val;
  } nr_pending_headers[MAX_PENDING_HEADERS];
  // FD of in/out buffers to read/write the response into.
  int nr_fd_in;
  int nr_fd_out;
  // The return status of the user-defined callback.
  struct nori_status nr_result;
  // The coroutine context and a flag indicating its current state.
  struct ucontext_t *nr_context;
  enum nori_response_state nr_state;
  // FAM representing the coroutine's stack.
  char nr_co_stack[];
};

struct nori_pss {
  nori_route_callback_t *nc_callback;
  struct nori_request nc_request;
  // Keep @nc_response last to support its FAM.
  struct nori_response nc_response;
};

// A reference to our main context. Every coroutine should always link back to
// this one.
static thread_local ucontext_t context_main;

// The call to `makecontext` does not permit any arguments but the user-defined
// callback expects the request/response pair introduced in the current
// transaction's PSS. Use this to temporarily hold the value for @coro_wrapper
// to reference.
static thread_local struct nori_pss *coro_pss;

static struct nori_status coro_suspend(struct nori_response *const r)
{
  if (swapcontext(r->nr_context, &context_main) == -1) {
    perror("swapcontext");
    return NORI_FAILURE;
  }
  return NORI_SUCCESS;
}

static struct nori_status coro_resume(struct nori_response *const r)
{
  if (swapcontext(&context_main, r->nr_context) == -1) {
    perror("swapcontext");
    return NORI_FAILURE;
  }
  return NORI_SUCCESS;
}

// Wrapper supplied to ucontext.
static void coro_wrapper(void)
{
  struct nori_pss *pss = coro_pss;
  coro_pss = nullptr;

  struct nori_response *const r = &pss->nc_response;
  r->nr_result = pss->nc_callback(pss->nc_request, r);

  // Finishing transitioning through the state machine. It's possible the
  // user-defined callback does nothing at all in which case we are at the first
  // state.

  if (r->nr_state == NORI_RESPONSE_STATE_HEADER) {
    nori_log_debug("Transitioning from HEADER to HEADER_FLUSH");
    r->nr_state = NORI_RESPONSE_STATE_HEADER_FLUSH;
    struct nori_status status = coro_suspend(r);
    if (!status.ns_success) {
      nori_log_error("Could not suspend coroutine");
      return;
    }
  }

  if (r->nr_state == NORI_RESPONSE_STATE_HEADER_FLUSH) {
    struct nori_status status = coro_suspend(r);
    if (!status.ns_success) {
      nori_log_error("Could not suspend coroutine");
      return;
    }
  }

  if (r->nr_state == NORI_RESPONSE_STATE_BODY) {
    nori_log_debug("Transitioning from BODY to BODY_FLUSH");
    r->nr_state = NORI_RESPONSE_STATE_BODY_FLUSH;
    struct nori_status status = coro_suspend(r);
    if (!status.ns_success) {
      nori_log_error("Could not suspend coroutine");
      return;
    }
  }

  if (r->nr_state == NORI_RESPONSE_STATE_BODY_FLUSH) {
    struct nori_status status = coro_suspend(r);
    if (!status.ns_success) {
      nori_log_error("Could not suspend coroutine");
      return;
    }
  }
}

// ================================================================
// Responses
// ================================================================

struct nori_status
nori_response_set_code(struct nori_response *const r, enum nori_http_code code)
{
  if (code == NORI_HTTP_CODE_INTERNAL) {
    return NORI_FAILURE_ERROR("Cannot use internal HTTP code");
  }

  if (r->nr_common_code == NORI_HTTP_CODE_INTERNAL) {
    r->nr_common_code = code;
    return NORI_SUCCESS;
  }

  return NORI_FAILURE_ERROR("Cannot specify HTTP code more than once");
}

struct nori_status nori_response_set_header(
    struct nori_response *const r,
    struct nori_str_view header,
    struct nori_str_view value)
{
  if (nori_str_view_ieq(header, NSV("Content-Type"))) {
    if (r->nr_common_type.len > 0) {
      return NORI_FAILURE_ERROR("Cannot specify Content-Type more than once");
    }
    r->nr_common_type = value;
    return NORI_SUCCESS;
  }

  if (nori_str_view_ieq(header, NSV("Content-Length"))) {
    if (r->nr_common_length.len > 0) {
      return NORI_FAILURE_ERROR("Cannot specify Content-Length more than once");
    }
    r->nr_common_length = value;
    return NORI_SUCCESS;
  }

  assert(r->nr_pending_headers_count < MAX_PENDING_HEADERS);
  r->nr_pending_headers[r->nr_pending_headers_count].nr_key = header;
  r->nr_pending_headers[r->nr_pending_headers_count].nr_val = value;
  r->nr_pending_headers_count += 1;

  // Must suspend at this point to flush headers.
  if (r->nr_pending_headers_count == MAX_PENDING_HEADERS) {
    struct nori_status status = coro_suspend(r);
    if (!status.ns_success) {
      nori_log_error("Could not suspend coroutine");
      return status;
    }
  }

  return NORI_SUCCESS;
}

// ================================================================
// Server
// ================================================================

static void nori_log_callback(enum lws_callback_reasons reason)
{
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
    break;
  }
  case LWS_CALLBACK_HTTP_BODY: {
    nori_log_debug("LWS_CALLBACK_HTTP_BODY");
    break;
  }
  case LWS_CALLBACK_HTTP_BODY_COMPLETION: {
    nori_log_debug("LWS_CALLBACK_HTTP_BODY_COMPLETION");
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
    nori_log_warn("Unmanaged callback %u", reason);
    break;
  }
  }
}

// LWS's documentation is...lacking. From what I can tell, any nonzero value
// indicates closing the connection.
constexpr int LWS_CONTINUE = 0;
constexpr int LWS_CLOSE = -1;

// Size of the outbound buffer. Avoid making too large since otherwise LWS in
// turn has to buffer any remaining value, requiring additional heap allocations
// and generally slower processing.
constexpr ssize_t BUFFER_SIZE = 4096;

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

static struct nori_status nori_write_lws_common(
    struct lws *const wsi,
    struct nori_response *const r,
    unsigned char **p,
    unsigned char *end)
{
  unsigned int code = NORI_HTTP_CODE_OK;
  if (r->nr_common_code != NORI_HTTP_CODE_INTERNAL) {
    code = r->nr_common_code;
  }

  const char *content_type = "text/html";
  if (r->nr_common_type.len > 0) {
    content_type = r->nr_common_type.view;
  }

  // TODO: Actually use content response content length.
  lws_filepos_t content_len = LWS_ILLEGAL_HTTP_CONTENT_LEN;

  if (lws_add_http_common_headers(wsi, code, content_type, content_len, p, end)) {
    return NORI_FAILURE_ERROR("Could not write common headers");
  }

  return NORI_SUCCESS;
}

static int lws_http_callback(
    struct lws *const wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
  nori_log_callback(reason);

  struct nori_pss *pss = user;
  struct nori_status status = {};

  // Ordered in roughly the order the callbacks are triggered.
  switch (reason) {
  case LWS_CALLBACK_HTTP: {
    nori_log_debug("LWS_CALLBACK_HTTP");

    // ----------------------------------------------------------------
    // Setup
    // ----------------------------------------------------------------

    const struct lws_protocols *proto = lws_get_protocol(wsi);
    const struct nori_server *const server = proto->user;
    memset(pss, 0, sizeof(struct nori_pss) + server->config.nc_co_stack);

    // Resources that need to be potentially cleaned up on error.
    int pipefd[2] = {};

    // ----------------------------------------------------------------
    // Route matching
    // ----------------------------------------------------------------

    enum nori_method request_method = NORI_METHOD_GET;
    if (lws_hdr_total_length(wsi, WSI_TOKEN_GET_URI)) {
      request_method = NORI_METHOD_GET;
    } else if (lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI)) {
      request_method = NORI_METHOD_POST;
    } else { // TODO: Return a 500
      nori_log_error("Unmanaged HTTP method");
      goto lws_callback_http_cleanup;
    }

    char in_path[2048] = {'/'}; // TODO: Return 414 if longer than buffer.
    lws_snprintf(in_path + 1, sizeof(in_path) - 1, "%s", (const char *)in);

    const struct nori_route *const route =
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
    pss->nc_request.nr_method = route->nr_method;
    pss->nc_request.nr_path = route->nr_path;

    // ----------------------------------------------------------------
    // Response
    // ----------------------------------------------------------------

    if (pipe2(pipefd, O_NONBLOCK) == -1) { // TODO: This should return a 500.
      perror("pipe2");
      nori_log_error("Could not create coroutine");
      goto lws_callback_http_cleanup;
    }

    pss->nc_response.nr_wsi = wsi;
    pss->nc_response.nr_common_code = NORI_HTTP_CODE_INTERNAL;
    pss->nc_response.nr_common_length = NSV("");
    pss->nc_response.nr_common_type = NSV("");
    pss->nc_response.nr_common_flushed = false;
    pss->nc_response.nr_pending_headers_count = 0;
    // pss->nc_response.nr_pending_headers = {};
    pss->nc_response.nr_fd_in = pipefd[0];
    pss->nc_response.nr_fd_out = pipefd[1];
    pss->nc_response.nr_result = NORI_FAILURE;

    // ----------------------------------------------------------------
    // Coroutine setup
    // ----------------------------------------------------------------

    ucontext_t context_coro = {};
    if (getcontext(&context_coro) == -1) { // TODO: Return a 500.
      perror("getcontext");
      nori_log_error("Could not create coroutine");
      goto lws_callback_http_cleanup;
    }

    context_coro.uc_stack.ss_sp = pss->nc_response.nr_co_stack;
    context_coro.uc_stack.ss_size = server->config.nc_co_stack;
    context_coro.uc_link = &context_main;
    makecontext(&context_coro, coro_wrapper, 0);

    pss->nc_response.nr_context = &context_coro;
    pss->nc_response.nr_state = NORI_RESPONSE_STATE_HEADER;
    coro_pss = pss; // Set before context switch.

    status = coro_resume(&pss->nc_response);
    if (!status.ns_success) {
      nori_log_error("Could not resume coroutine");
      goto lws_callback_http_cleanup;
    }

    // Queues HTTP_BODY.* and HTTP_WRITEABLE callbacks.
    lws_callback_on_writable(wsi);

    return LWS_CONTINUE;

  lws_callback_http_cleanup:
    if (pipefd[0] && close(pipefd[0]) == -1) {
      perror("close pipefd[0]");
    }
    if (pipefd[1] && close(pipefd[1]) == -1) {
      perror("close pipefd[1]");
    }
    return LWS_CLOSE;
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
    // Write HTTP
    // ----------------------------------------------------------------

    struct nori_response *const r = &pss->nc_response;

    uint8_t buffer[BUFFER_SIZE]; // HTTP does not need LWS_PRE
    uint8_t *start = buffer;
    uint8_t *p = start;
    uint8_t *end = buffer + sizeof(buffer) - 1;

    switch (pss->nc_response.nr_state) {
    case NORI_RESPONSE_STATE_HEADER:
    case NORI_RESPONSE_STATE_HEADER_FLUSH: {
      if (!r->nr_common_flushed &&
          (pss->nc_response.nr_state == NORI_RESPONSE_STATE_HEADER_FLUSH ||
           (r->nr_common_code != NORI_HTTP_CODE_INTERNAL && r->nr_common_type.len > 0 &&
            r->nr_common_length.len > 0))) {
        status = nori_write_lws_common(wsi, &pss->nc_response, &p, end);
        if (!status.ns_success) {
          nori_log_error("Could not write common headers");
          goto lws_callback_http_writeable_cleanup;
        }
        r->nr_common_flushed = true;
      }

      for (size_t i = 0; i < r->nr_pending_headers_count; ++i) {
        struct nori_str_view header = r->nr_pending_headers[i].nr_key;
        struct nori_str_view value = r->nr_pending_headers[i].nr_val;
        if (lws_add_http_header_by_name(
                wsi, (const unsigned char *)header.view,
                (const unsigned char *)value.view, value.len, &p, end)) {
          status = NORI_FAILURE_ERROR("Could not write header %s", header.view);
          goto lws_callback_http_writeable_cleanup;
        }
      }
      r->nr_pending_headers_count = 0;

      // Advance state machine.
      if (r->nr_state == NORI_RESPONSE_STATE_HEADER_FLUSH) {
        if (lws_finalize_write_http_header(wsi, start, &p, end)) {
          status = NORI_FAILURE_ERROR("Could not finalize http headers");
          goto lws_callback_http_writeable_cleanup;
        }
        nori_log_debug("Transitioned from HEADER_FLUSH to BODY");
        r->nr_state = NORI_RESPONSE_STATE_BODY;
      }

      status = coro_resume(r);
      if (!status.ns_success) {
        nori_log_error("Could not resume coroutine");
        goto lws_callback_http_writeable_cleanup;
      }

      lws_callback_on_writable(wsi);
      return LWS_CONTINUE;
    }
    case NORI_RESPONSE_STATE_BODY:
    case NORI_RESPONSE_STATE_BODY_FLUSH: {
      // TODO: Check if any content is pending.
      // if (!pss->nc_response.nr_coro_done) {
      //   char out[BUFFER_OUT];
      //   ssize_t count = read(pss->nc_response.nr_fd_out, out, BUFFER_OUT);

      //   if (count == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
      //     perror("read");
      //     nori_log_error("Could not read from coroutine pipe");
      //     return -1;
      //   }

      //   if (count == 0) {
      //     nori_log_error("Coroutine pipe unexpectedly closed");
      //     return -1;
      //   }

      //   lws_callback_on_writable(wsi); // More to read. Queue again.
      //   return 0;
      // }

      // Advance state machine.
      if (r->nr_state == NORI_RESPONSE_STATE_BODY_FLUSH) {
        if (lws_write(
                wsi, start, lws_ptr_diff_size_t(p, start), LWS_WRITE_HTTP_FINAL) !=
            lws_ptr_diff(p, start)) {
          status = NORI_FAILURE_ERROR("Could not write to body");
          goto lws_callback_http_writeable_cleanup;
        }
        nori_log_debug("Transitioned from BODY_FLUSH to CLOSING");
        r->nr_state = NORI_RESPONSE_STATE_CLOSING;
      }

      status = coro_resume(r);
      if (!status.ns_success) {
        nori_log_error("Could not resume coroutine");
        goto lws_callback_http_writeable_cleanup;
      }

      if (r->nr_state != NORI_RESPONSE_STATE_CLOSING) {
        lws_callback_on_writable(wsi);
        return LWS_CONTINUE;
      }

      [[fallthrough]];
    }
    case NORI_RESPONSE_STATE_CLOSING: {
      if (lws_http_transaction_completed(wsi)) {
        nori_log_debug("Transitioning from CLOSING to CLOSED");
        r->nr_state = NORI_RESPONSE_STATE_CLOSED;
      }

      // Otherwise keep the connection open. LWS is responsible for logically
      // separating each transaction over a connection and thereby resetting our
      // state machine.

      [[fallthrough]];
    }
    case NORI_RESPONSE_STATE_CLOSED: {
      break;
    }
    }

  lws_callback_http_writeable_cleanup:
    if (r->nr_fd_in && close(r->nr_fd_in) == -1) {
      perror("close fd_in");
    }
    r->nr_fd_in = 0;

    if (r->nr_fd_out && close(r->nr_fd_out) == -1) {
      perror("close fd_out");
    }
    r->nr_fd_out = 0;

    return r->nr_state == NORI_RESPONSE_STATE_CLOSED ? LWS_CLOSE : LWS_CONTINUE;
  }

  default: {
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
    return NORI_FAILURE_ERROR("Could not install interrupt handler");
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
