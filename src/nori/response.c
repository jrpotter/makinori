#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "nori/internal/response.h"
#include "nori/logger.h"
#include "nori/response.h"

// =================================================================================
// Coroutines

thread_local ucontext_t context_server = {};

struct nori_status nori_response_suspend(struct nori_response *const res)
{
  nori_trace(NORI_TRACE_RESPONSE_STATE, "(%p) SUSPENDED", (void *){res});
  if (swapcontext(&res->nr_context, &context_server) == -1) {
    perror("[nori_response_suspend,swapcontext] ret:-1");
    return NORI_FAILURE(NORI_ERROR_GENERIC);
  }
  return NORI_SUCCESS;
}

struct nori_status nori_response_resume(struct nori_response *const res)
{
  nori_trace(NORI_TRACE_RESPONSE_STATE, "(%p) RESUMED", (void *){res});
  if (swapcontext(&context_server, &res->nr_context) == -1) {
    perror("[nori_response_resume,swapcontext] ret:-1");
    return NORI_FAILURE(NORI_ERROR_GENERIC);
  }
  return NORI_SUCCESS;
}

// =================================================================================
// API

struct nori_status
nori_response_set_code(struct nori_response *const r, enum nori_http_code code)
{
  if (r->nr_state > NORI_RESPONSE_STATE_HEADER) {
    return NORI_ERROR_EMIT(NORI_ERROR_GENERIC, "The header is no longer mutable");
  }

  if (r->nr_common_code == 0) {
    r->nr_common_code = code;
    return NORI_SUCCESS;
  }

  return NORI_ERROR_EMIT(NORI_ERROR_GENERIC, "Cannot specify HTTP code more than once");
}

struct nori_status nori_response_set_header(
    struct nori_response *const r,
    struct nori_str_view header,
    struct nori_str_view value)
{
  if (r->nr_state != NORI_RESPONSE_STATE_HEADER) {
    return NORI_ERROR_EMIT(NORI_ERROR_GENERIC, "The header is no longer mutable");
  }

  if (nori_str_view_ieq(header, NSV("Content-Type"))) {
    if (r->nr_common_type.len > 0) {
      return NORI_ERROR_EMIT(
          NORI_ERROR_GENERIC, "Cannot specify Content-Type more than once");
    }
    r->nr_common_type = value;
    return NORI_SUCCESS;
  }

  if (nori_str_view_ieq(header, NSV("Content-Length"))) {
    if (r->nr_common_length.len > 0) {
      return NORI_ERROR_EMIT(
          NORI_ERROR_GENERIC, "Cannot specify Content-Length more than once");
    }
    r->nr_common_length = value;
    return NORI_SUCCESS;
  }

  nori_assert(r->nr_pending_headers_count < NORI_RESPONSE_HEADER_THRESHOLD);
  r->nr_pending_headers[r->nr_pending_headers_count].nr_key = header;
  r->nr_pending_headers[r->nr_pending_headers_count].nr_val = value;
  r->nr_pending_headers_count += 1;

  // Must suspend at this point to flush headers.
  if (r->nr_pending_headers_count == NORI_RESPONSE_HEADER_THRESHOLD) {
    struct nori_status status = nori_response_suspend(r);
    if (status.ns_error) {
      nori_log_error("Could not suspend coroutine");
      return status;
    }
  }

  return NORI_SUCCESS;
}

struct nori_status nori_response_write(
    struct nori_response *const res,
    char buffer[const static 1],
    size_t const len)
{
  struct nori_status status = NORI_SUCCESS;

  if (res->nr_state == NORI_RESPONSE_STATE_HEADER) {
    nori_trace(NORI_TRACE_RESPONSE_STATE, "(%p) HEADER -> HEADER_FLUSH", (void *){res});
    res->nr_state = NORI_RESPONSE_STATE_HEADER_FLUSH;
    status = nori_response_suspend(res);
    if (status.ns_error) {
      nori_log_error("Could not suspend coroutine");
      return status;
    }
    nori_assert(res->nr_state == NORI_RESPONSE_STATE_BODY);
  }

  if (res->nr_state != NORI_RESPONSE_STATE_BODY) {
    return NORI_ERROR_EMIT(NORI_ERROR_GENERIC, "The body is no longer mutable");
  }

  size_t count = 0;
  while (count < len) {
    ssize_t n = write(res->nr_fd_write, buffer + count, len - count);

    if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      nori_log_warn("nori_response_write blocked");
      status = nori_response_suspend(res);
      if (status.ns_error) {
        nori_log_error("Could not suspend coroutine");
        return status;
      }
      continue;
    }

    if (n == -1) {
      perror("[nori_response_write] write:-1");
      return NORI_ERROR_EMIT(NORI_ERROR_GENERIC, "Could not write from coroutine");
    }

    nori_assert(n >= 0);

    if (count / NORI_RESPONSE_BODY_THRESHOLD <
        (count + n) / NORI_RESPONSE_BODY_THRESHOLD) {
      status = nori_response_suspend(res);
      if (status.ns_error) {
        nori_log_error("Could not suspend coroutine");
        return status;
      }
    }

    count += n;
  }

  return NORI_SUCCESS;
}
