#include <fcntl.h>
#include <lauxlib.h>
#include <libwebsockets.h>
#include <signal.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <unistd.h>

#include "./response.h"
#include "makinori/logger.h"
#include "makinori/request.h"
#include "makinori/server.h"
#include "makinori/string.h"
#include "makinori/util.h"

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

struct mn_pss {
  mn_route_handler_t *handler;
  struct mn_request request;
  struct mn_response response;
};

static thread_local struct mn_pss *coro_arg;

// Entrypoint for the coroutine. Arguments, if provided, must be `int`s which
// may or may not be large enough to hold a pointer. As a workaround, use the
// coroutine_arg variable.
static void coro_start(void)
{
  struct mn_pss *pss = coro_arg;
  coro_arg = nullptr;

  struct mn_response *const res = &pss->response;
  res->status = pss->handler(pss->request, res);

  // Finish transitioning through the state machine. It's possible the handler
  // does nothing at all in which case we are at the first state. We should
  // always hit the .*_BODY condition since no function can be called from the
  // handler that transitions beyond .*_BODY.

  if (res->state == MN_RESPONSE_STATE_HEADER) {
    mn_trace(MN_TRACE_RESPONSE_STATE, "(%p) HEADER -> HEADER_FLUSH", (void *){res});
    res->state = MN_RESPONSE_STATE_HEADER_FLUSH;
    struct mn_status status = mn_response_suspend(res);
    if (status.error) {
      mn_log_error("Could not suspend coroutine");
      return;
    }
  }

  if (res->state == MN_RESPONSE_STATE_HEADER_FLUSH) {
    struct mn_status status = mn_response_suspend(res);
    if (status.error) {
      mn_log_error("Could not suspend coroutine");
      return;
    }
  }

  if (res->state == MN_RESPONSE_STATE_BODY) {
    mn_trace(MN_TRACE_RESPONSE_STATE, "(%p) BODY -> BODY_FLUSH", (void *){res});
    res->state = MN_RESPONSE_STATE_BODY_FLUSH;
    struct mn_status status = mn_response_suspend(res);
    if (status.error) {
      mn_log_error("Could not suspend coroutine");
      return;
    }
  }

  if (res->state == MN_RESPONSE_STATE_BODY_FLUSH) {
    struct mn_status status = mn_response_suspend(res);
    if (status.error) {
      mn_log_error("Could not suspend coroutine");
      return;
    }
  }

  mn_trace(MN_TRACE_RESPONSE_STATE, "(%p) FINISHED", (void *){res});
}

// =================================================================================
// Router

static struct mn_view
mn_find_substr(struct mn_view const path, struct mn_view const needle)
{
  for (int i = 0; i < path.len - needle.len + 1; ++i) {
    struct mn_view substr = mn_view_substr(path, i, i + needle.len);
    if (mn_view_eq(substr, needle)) {
      return substr;
    }
  }
  return mn_str_lit("").view;
}

static struct mn_route const *const mn_route_match(
    struct mn_server const server[const static 1],
    struct mn_request req[static 1])
{
  if (req->path.len == 0) {
    mn_log_warn("No route matches an empty path");
    return nullptr;
  }

  lua_getglobal(server->config.lua_, "makinori");

  struct mn_route const *match = nullptr;

  for (struct mn_route const *curr = &server->route; curr; curr = curr->next) {
    if (curr->pattern.len == 0) {
      mn_log_warn("Encountered route with empty pattern");
      continue;
    }

    size_t expected_count = 0;
    for (size_t i = 0; i < curr->pattern.len; ++i) {
      if (curr->pattern.ss[i] == '%') {
        i += 1;
      } else if (curr->pattern.ss[i] == '(') {
        expected_count += 1;
      }
    }

    // Must abort this path. We configured the amount of virtual stack space
    // according to MN_REQUEST_MAX_CAPTURES.
    if (expected_count > MN_REQUEST_MAX_CAPTURES) {
      mn_log_warn(
          "Pattern %s has more captures than %d", curr->pattern.ss,
          MN_REQUEST_MAX_CAPTURES);
      continue;
    }

    lua_getfield(server->config.lua_, -1, "anchor_string_match");
    // Use of ss_ is safe since we also pass the length.
    lua_pushlstring(server->config.lua_, req->path.ss_, req->path.len);
    lua_pushlstring(server->config.lua_, curr->pattern.ss, curr->pattern.len);

    if (lua_pcall(server->config.lua_, 2, LUA_MULTRET, 0)) {
      mn_log_warn(
          "nori.anchor_string_match: %s", lua_tostring(server->config.lua_, -1));
      lua_pop(server->config.lua_, 1);
      break;
    }

    // On a failed match, string.match returns nil.
    if (lua_type(server->config.lua_, -1) == LUA_TNIL) {
      lua_pop(server->config.lua_, 1);
      continue;
    }

    // On a successful match with no specificed captures, it returns the entire
    // string. Otherwise it returns one or more substrings corresponding to
    // each capture.
    int result_count = 0;
    while (lua_type(server->config.lua_, -result_count - 1) == LUA_TSTRING) {
      result_count += 1;
    }
    req->capture_count = expected_count == 0 && result_count == 1 ? 0 : result_count;
    mn_assert(expected_count == req->capture_count);
    mn_assert(req->capture_count <= MN_REQUEST_MAX_CAPTURES);

    // We need our captures to reference the string in the request path, not
    // the return values from Lua (which will be memory collected once we reset
    // the virtual stack). Though string.match doesn't tell us which match was
    // actually captured, it doesn't matter. We can just pick the first one.
    for (int i = 0; i < result_count; ++i) {
      size_t len = 0;
      char const *capture = lua_tolstring(server->config.lua_, -1, &len);

      if (len == 0) { // E.g. a capture of form (.*)
        req->captures[result_count - i - 1] = mn_view_ref("", 0);
      } else {
        struct mn_view needle = mn_view_ref(capture, len);
        struct mn_view substr = mn_find_substr(req->path, needle);
        mn_assert(substr.len > 0);
        req->captures[result_count - i - 1] = substr;
      }

      lua_pop(server->config.lua_, 1);
    }

    match = curr;
    break;
  }

  lua_pop(server->config.lua_, 1); // nori global
  return match;
}

// =================================================================================
// Server

static struct mn_status mn_write_lws_common(
    struct lws *const wsi,
    struct mn_response *const r,
    unsigned char **p,
    unsigned char *end)
{
  unsigned int code = MN_HTTP_CODE_OK;
  if (r->common_code != 0) {
    code = r->common_code;
  }

  char const *content_type = "text/plain";
  if (r->common_type.len > 0) {
    content_type = r->common_type.ss;
  }

  // TODO: Actually use content response content length.
  lws_filepos_t content_len = LWS_ILLEGAL_HTTP_CONTENT_LEN;

  if (lws_add_http_common_headers(wsi, code, content_type, content_len, p, end)) {
    return MN_ERROR_EMIT(MN_ERROR_SYSTEM, "Could not write common headers");
  }

  return MN_SUCCESS;
}

// LWS's documentation is...lacking. From what I can tell, any nonzero value
// indicates closing the connection.
int constexpr LWS_CONTINUE = 0;
int constexpr LWS_CLOSE = -1;

struct mn_method_map {
  enum lws_token_indexes from;
  enum mn_method to;
};

static void mn_trace_callback(enum lws_callback_reasons reason)
{
  // Ordered in roughly the order the callbacks are triggered.
  switch (reason) {
  case LWS_CALLBACK_WSI_CREATE: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_WSI_CREATE");
    break;
  }
  case LWS_CALLBACK_PROTOCOL_INIT: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_PROTOCOL_INIT");
    break;
  }
  case LWS_CALLBACK_FILTER_NETWORK_CONNECTION: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_FILTER_NETWORK_CONNECTION");
    break;
  }
  case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED");
    break;
  }
  case LWS_CALLBACK_EVENT_WAIT_CANCELLED: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_EVENT_WAIT_CANCELLED");
    break;
  }
  case LWS_CALLBACK_FILTER_HTTP_CONNECTION: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_FILTER_HTTP_CONNECTION");
    break;
  }
  case LWS_CALLBACK_HTTP_BIND_PROTOCOL: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_HTTP_BIND_PROTOCOL");
    break;
  }
  case LWS_CALLBACK_CHECK_ACCESS_RIGHTS: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_CHECK_ACCESS_RIGHTS");
    break;
  }
  case LWS_CALLBACK_HTTP: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_HTTP");
    break;
  }
  case LWS_CALLBACK_HTTP_BODY: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_HTTP_BODY");
    break;
  }
  case LWS_CALLBACK_HTTP_BODY_COMPLETION: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_HTTP_BODY_COMPLETION");
    break;
  }
  case LWS_CALLBACK_HTTP_WRITEABLE: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_HTTP_WRITEABLE");
    break;
  }
  case LWS_CALLBACK_HTTP_DROP_PROTOCOL: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_HTTP_DROP_PROTOCOL");
    break;
  }
  case LWS_CALLBACK_CLOSED_HTTP: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_CLOSED_HTTP");
    break;
  }
  case LWS_CALLBACK_PROTOCOL_DESTROY: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_PROTOCOL_DESTROY");
    break;
  }
  case LWS_CALLBACK_WSI_DESTROY: {
    mn_trace(MN_TRACE_LWS_CALLBACK, "LWS_CALLBACK_WSI_DESTROY");
    break;
  }
  default: {
    mn_log_warn("Unmanaged lws callback %u", reason);
    break;
  }
  }
}

static int lws_http_callback(
    struct lws *const wsi,
    enum lws_callback_reasons lws_reason,
    void *lws_user,
    void *lws_in,
    size_t lws_len)
{
  static long PAGE_SIZE = 0;

  if (PAGE_SIZE <= 0) {
    errno = 0;
    PAGE_SIZE = sysconf(_SC_PAGESIZE);
    if (PAGE_SIZE == -1 && errno != 0) {
      mn_perror("sysconf");
    }
    // Impossible to recover. Aborting is the only sensible choice.
    mn_assert(PAGE_SIZE > 0);
  }

  struct mn_pss *pss = lws_user;
  struct mn_request *const req = &pss->request;
  struct mn_response *const res = &pss->response;

  mn_trace_callback(lws_reason);

  // Ordered in roughly the same order the callbacks are triggered.
  switch (lws_reason) {
  case LWS_CALLBACK_HTTP: {
    memset(pss, 0, sizeof(struct mn_pss));

    struct lws_protocols const *proto = lws_get_protocol(wsi);
    struct mn_server const *const server = proto->user;

    { // --- Parse request ---------------------------------------------------------
      int n = 0;
      int count = 0;

      { // --- Method & Path
        MN_PAIR(enum lws_token_indexes, enum mn_method)
        methods[] = {
            {.fst = WSI_TOKEN_GET_URI, .snd = MN_METHOD_GET},
        };

        for (size_t i = 0; i <= MN_ARR_SIZE(methods); ++i) {
          n = lws_hdr_copy(
              wsi, req->buffer_ + count, MN_REQUEST_MAX_PATH_LEN - count,
              methods[i].fst);

          if (n > 0) {
            req->method = methods[i].snd;
            req->path = mn_view_ref(req->buffer_ + count, n);
            count += n;
            break;
          } else if (n == -1) {
            // TODO: Return 414
            mn_log_warn("Request URI too large: %d >= %d", n, MN_REQUEST_MAX_PATH_LEN);
            return LWS_CLOSE;
          }
        }

        if (n == 0) {
          // TODO: Return 501
          mn_log_warn("Encountered unsupported HTTP method");
          return LWS_CLOSE;
        }
      }

      { // --- Query Params
        do {
          n = lws_hdr_copy_fragment(
              // Offset by 1 so we can insert a delimiter. Insert after this call
              // so we are notified if there is room left in our buffer first.
              wsi, req->buffer_ + count + 1, MN_REQUEST_MAX_PATH_LEN - count - 1,
              WSI_TOKEN_HTTP_URI_ARGS, req->query_count);

          if (n == -2) {
            // TODO: Return 414
            mn_log_warn("Request URI too large: %d >= %d", n, MN_REQUEST_MAX_PATH_LEN);
            return LWS_CLOSE;
          }

          if (n > 0) {
            if (req->query_count + 1 >= MN_REQUEST_MAX_QUERY_PARAMS) {
              // TODO: Return 501
              mn_log_warn(
                  "Request URI too large: %d >= %d", n, MN_REQUEST_MAX_PATH_LEN);
              return LWS_CLOSE;
            }

            req->buffer_[count] = req->query_count == 0 ? '?' : '&';

            // Find key/value separator.
            size_t offset = count + 1;
            while (offset < n + count + 1 && req->buffer_[offset] != '=') {
              offset += 1;
            }

            req->query[req->query_count].key =
                mn_view_ref(req->buffer_ + count + 1, offset - count - 1);
            req->query[req->query_count].value =
                mn_view_ref(req->buffer_ + offset + 1, n - offset + 1);

            req->query_count += 1;
            count += n + 1;
          }
        } while (n >= 0);
      }

      // Find the route that corresponds to our request. Also sets captures if
      // the route's pattern includes them.
      struct mn_route const *const route = mn_route_match(server, req);

      if (route == nullptr) {
        // TODO: Return 404
        return LWS_CLOSE;
      }

      if (!route->handler) {
        // TODO: Return 204?
        mn_log_warn("No handler registered for %s", (char const *){lws_in});
        return LWS_CLOSE;
      }

      pss->handler = route->handler;
      res->wsi = wsi;
      res->status = MN_FAILURE(MN_ERROR_SYSTEM);
      res->state = MN_RESPONSE_STATE_HEADER;
    }

    { // --- Connect descriptors ---------------------------------------------------
      int pipefd[2] = {};
      if (pipe2(pipefd, O_NONBLOCK) == -1) {
        // TODO: This should return a 500.
        mn_perror("pipe2");
        return LWS_CLOSE;
      }

      res->fd_read = pipefd[0];
      res->fd_write = pipefd[1];
    }

    { // --- Setup the coroutine ---------------------------------------------------

      // Saves the content of the registers, signal mask, and the stack.
      if (getcontext(&res->context) == -1) { // TODO: Return a 500.
        mn_perror("getcontext");
        return LWS_CLOSE;
      }

      size_t const stack_size = (PAGE_SIZE + 1) * server->config.coro_pages;

      // Allocate an additional page for use as a guard. As an extra precaution,
      // probably a good idea to pass -fstack-clas-protected enabled on
      // environments that support it.
      void *const stack = mmap(
          nullptr, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,
          0);

      if (stack == MAP_FAILED) {
        mn_perror("mmap");
        return LWS_CLOSE;
      }

      // Set before protection potentially fails so we munmap correctly.
      res->coro_stack = stack;

      if (mprotect(stack, PAGE_SIZE, PROT_NONE) == -1) {
        mn_perror("mprotect");
        return LWS_CLOSE;
      }

      sigemptyset(&res->context.uc_sigmask);
      res->context.uc_stack.ss_sp = stack + PAGE_SIZE;
      res->context.uc_stack.ss_size = stack_size - PAGE_SIZE;
      res->context.uc_link = &context_server;
      makecontext(&res->context, coro_start, 0);
    }

    { // --- Trigger first context switch ------------------------------------------
      coro_arg = pss; // Set before context switch.
      res->status = mn_response_resume(res);
      if (res->status.error) {
        mn_log_error("Could not resume coroutine");
        return LWS_CLOSE;
      }
    }

    { // --- Queue BODY and WRITEABLE callbacks ------------------------------------
      lws_callback_on_writable(wsi);
    }

    return LWS_CONTINUE;
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
    if (res->status.error) {
      return LWS_CLOSE;
    }
    // The event loop may trigger spurious writeable callbacks for internal
    // reasons. If our status is failed or state is closed, then we have already
    // cleaned up resources and there should be nothing left to do.
    if (res->state == MN_RESPONSE_STATE_CLOSED) {
      return LWS_CLOSE;
    }

    uint8_t buffer[MN_RESPONSE_BODY_THRESHOLD]; // HTTP does not need LWS_PRE
    uint8_t *start = buffer;
    uint8_t *p = start;
    uint8_t *end = buffer + sizeof(buffer) - 1;

    switch (res->state) {
    case MN_RESPONSE_STATE_HEADER:
    case MN_RESPONSE_STATE_HEADER_FLUSH: {
      { // --- Check if LWS "common" fields should be written ----------------------
        if (!res->common_flushed &&
            (res->state == MN_RESPONSE_STATE_HEADER_FLUSH ||
             (res->common_code != 0 && res->common_type.len > 0 &&
              res->common_length.len > 0))) {
          res->status = mn_write_lws_common(wsi, res, &p, end);
          if (res->status.error) {
            mn_log_error("Could not write common headers");
            return LWS_CLOSE;
          }
          res->common_flushed = true;
        }
      }

      { // --- Flush pending headers -----------------------------------------------
        for (size_t i = 0; i < res->pending_headers_count; ++i) {
          struct mn_str header = res->pending_headers[i].key;
          struct mn_str value = res->pending_headers[i].val;
          if (lws_add_http_header_by_name(
                  wsi, (unsigned char const *)header.ss,
                  (unsigned char const *)value.ss, value.len, &p, end)) {
            res->status =
                MN_ERROR_EMIT(MN_ERROR_SYSTEM, "Could not write header %s", header.ss);
            return LWS_CLOSE;
          }
        }
        res->pending_headers_count = 0;
      }

      { // --- Advance the state machine -------------------------------------------
        if (res->state == MN_RESPONSE_STATE_HEADER_FLUSH) {
          if (lws_finalize_write_http_header(wsi, start, &p, end)) {
            res->status =
                MN_ERROR_EMIT(MN_ERROR_SYSTEM, "Could not finalize http headers");
            return LWS_CLOSE;
          }
          mn_trace(MN_TRACE_RESPONSE_STATE, "(%p) HEADER_FLUSH -> BODY", (void *){res});
          res->state = MN_RESPONSE_STATE_BODY;
        }
      }

      { // --- Resume the coroutine ------------------------------------------------
        res->status = mn_response_resume(res);
        if (res->status.error) {
          mn_log_error("Could not resume coroutine");
          return LWS_CLOSE;
        }
      }

      { // --- Queue up another LWS_CALLBACK_HTTP_WRITEABLE ------------------------
        lws_callback_on_writable(wsi);
      }

      return LWS_CONTINUE;
    }
    case MN_RESPONSE_STATE_BODY:
    case MN_RESPONSE_STATE_BODY_FLUSH: {
      { // --- Attempt to read HTTP content ----------------------------------------
        ssize_t n = read(res->fd_read, p, MN_RESPONSE_BODY_THRESHOLD);

        if (n == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
          mn_perror("read");
          res->status = MN_FAILURE(MN_ERROR_SYSTEM);
          return LWS_CLOSE;
        }

        if (n == 0) {
          res->status = MN_ERROR_EMIT(MN_ERROR_SYSTEM, "Coroutine pipe closed");
          return LWS_CLOSE;
        }

        if (n > 0) {
          p += n;
          if (lws_write(wsi, start, lws_ptr_diff_size_t(p, start), LWS_WRITE_HTTP) !=
              lws_ptr_diff(p, start)) {
            res->status = MN_ERROR_EMIT(MN_ERROR_SYSTEM, "Could not write to body");
            return LWS_CLOSE;
          }
          lws_callback_on_writable(wsi); // Immediately re-enter event loop.
          return LWS_CONTINUE;
        }
      }

      { // --- Advance the state machine -------------------------------------------
        if (res->state == MN_RESPONSE_STATE_BODY_FLUSH) {
          if (lws_write(
                  wsi, start, lws_ptr_diff_size_t(p, start), LWS_WRITE_HTTP_FINAL) !=
              lws_ptr_diff(p, start)) {
            res->status = MN_ERROR_EMIT(MN_ERROR_SYSTEM, "Could not write to body");
            return LWS_CLOSE;
          }
          mn_trace(
              MN_TRACE_RESPONSE_STATE, "(%p) BODY_FLUSH -> CLOSING", (void *){res});
          res->state = MN_RESPONSE_STATE_CLOSING;
        }
      }

      { // --- Resume the coroutine ------------------------------------------------
        res->status = mn_response_resume(res);
        if (res->status.error) {
          mn_log_error("Could not resume coroutine");
          return LWS_CLOSE;
        }
      }

      { // --- Queue up another LWS_CALLBACK_HTTP_WRITEABLE ------------------------
        lws_callback_on_writable(wsi);
      }

      return LWS_CONTINUE;
    }
    case MN_RESPONSE_STATE_CLOSING: {
      if (lws_http_transaction_completed(wsi)) {
        mn_trace(MN_TRACE_RESPONSE_STATE, "(%p) CLOSING -> CLOSED", (void *){res});
        res->state = MN_RESPONSE_STATE_CLOSED;
      } else {
        // Otherwise the connection remains open. LWS is responsible for
        // logically separating each transaction over the connection and thereby
        // resetting our state machine.
      }
      [[fallthrough]];
    }
    case MN_RESPONSE_STATE_CLOSED: {
      break;
    }
    }

    if (res->status.error || res->state == MN_RESPONSE_STATE_CLOSED) {
      return LWS_CLOSE;
    }

    return LWS_CONTINUE;
  }

  case LWS_CALLBACK_CLOSED_HTTP: {
    if (res->fd_read) {
      if (close(res->fd_read) == -1) {
        mn_perror("close");
      }
      res->fd_read = 0;
    }
    if (res->fd_write) {
      if (close(res->fd_write) == -1) {
        mn_perror("close");
      }
      res->fd_write = 0;
    }
    if (res->coro_stack) {
      struct lws_protocols const *proto = lws_get_protocol(wsi);
      struct mn_server const *const server = proto->user;
      size_t const stack_size = (PAGE_SIZE + 1) * server->config.coro_pages;
      if (munmap(res->coro_stack, stack_size) == -1) {
        mn_perror("munmap"); // Indicate the leak but don't abort.
      }
      res->coro_stack = nullptr;
    }
    return LWS_CONTINUE;
  }

  default: {
    break;
  }
  }

  // TODO: Add a better 404 handler.
  return lws_callback_http_dummy(wsi, lws_reason, lws_user, lws_in, lws_len);
}

// =================================================================================
// Entrypoint

struct mn_status mn_server_run(struct mn_server s[static 1])
{
  struct sigaction const act = {.sa_handler = server_signal_handler};
  if (sigaction(SIGINT, &act, nullptr) == -1) {
    mn_perror("sigaction");
    return MN_ERROR_EMIT(MN_ERROR_SYSTEM, "Could not install signal handler");
  };

  struct lws_protocols const http_protocol = {
      .name = "http",
      .callback = lws_http_callback,
      .id = 0,
      .user = s,
      .per_session_data_size = sizeof(struct mn_pss),
      .rx_buffer_size = 0,
      .tx_packet_size = 0};

  struct lws_protocols const *pprotocols[] = {&http_protocol, nullptr};

  // The inclusion (or lack thereof) of a trailing / in the request path
  // yields two different paths. The only exception is at root. For example,
  // localhost:8000 and localhost:8000/ both have path /.
  struct lws_http_mount const http_mount = {
      .protocol = "http",
      .mountpoint = "",
      .mountpoint_len = 0,
      .origin_protocol = LWSMPRO_CALLBACK,
  };

  struct lws_context_creation_info info;
  lws_context_info_defaults(&info, nullptr);
  info.mounts = &http_mount;
  info.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
  info.port = s->config.port;
  info.pprotocols = pprotocols;
  info.server_string = "makinori";
  info.vhost_name = "localhost";

  struct lws_context *context = lws_create_context(&info);
  if (!context) {
    return MN_ERROR_EMIT(MN_ERROR_SYSTEM, "Could not create lws context");
  }

  struct lws_vhost *vh = lws_create_vhost(context, &info);
  if (!vh) {
    return MN_ERROR_EMIT(MN_ERROR_SYSTEM, "Could not create lws vhost");
  }

  mn_log_notice("Starting server on port %ld", s->config.port);

  int status = 0;
  while (status >= 0 && SERVER_RUNNING) {
    status = lws_service(context, /* unused */ 0);
  }

  mn_log_notice("Stopping server");
  lws_context_destroy(context);

  return MN_SUCCESS;
}
