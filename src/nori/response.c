#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "./response.h"
#include "nori/logger.h"
#include "nori/response.h"
#include "nori/util.h"

// =================================================================================
// Coroutines

thread_local ucontext_t context_server = {};

struct nori_status nori_response_suspend(struct nori_response *const res)
{
  nori_trace(NORI_TRACE_RESPONSE_STATE, "(%p) SUSPENDED", (void *){res});

  // Expect callers to log the error so we know the correct location.
  if (swapcontext(&res->nr_context, &context_server) == -1) {
    return NORI_FAILURE(NORI_ERROR_SYSTEM);
  }

  return NORI_SUCCESS;
}

struct nori_status nori_response_resume(struct nori_response *const res)
{
  nori_trace(NORI_TRACE_RESPONSE_STATE, "(%p) RESUMED", (void *){res});

  // Expect callers to log the error so we know the correct location.
  if (swapcontext(&context_server, &res->nr_context) == -1) {
    return NORI_FAILURE(NORI_ERROR_SYSTEM);
  }

  return NORI_SUCCESS;
}

// =================================================================================
// API

struct nori_status
nori_response_set_code(struct nori_response *const r, enum nori_http_code code)
{
  if (code < NORI_HTTP_CODE_OK) {
    return NORI_FAILURE(NORI_ERROR_INVALID_ARG);
  }

  if (r->nr_state != NORI_RESPONSE_STATE_HEADER) {
    return NORI_FAILURE(NORI_ERROR_IMMUTABLE);
  }

  // Cannot set the HTTP status code more than once.
  if (r->nr_common_code) {
    return NORI_FAILURE(NORI_ERROR_DUPLICATE);
  }

  r->nr_common_code = code;
  return NORI_SUCCESS;
}

struct nori_status nori_response_set_header(
    struct nori_response *const r,
    struct nori_str header,
    struct nori_str value)
{
  if (header.len == 0 || value.len == 0) {
    return NORI_FAILURE(NORI_ERROR_INVALID_ARG);
  }

  if (r->nr_state != NORI_RESPONSE_STATE_HEADER) {
    return NORI_FAILURE(NORI_ERROR_IMMUTABLE);
  }

  if (nori_str_ieq(header, nori_str_lit("Content-Type"))) {
    if (r->nr_common_type.len > 0) {
      return NORI_FAILURE(NORI_ERROR_DUPLICATE);
    }
    r->nr_common_type = value;
    return NORI_SUCCESS;
  }

  if (nori_str_ieq(header, nori_str_lit("Content-Length"))) {
    if (r->nr_common_length.len > 0) {
      return NORI_FAILURE(NORI_ERROR_DUPLICATE);
    }
    r->nr_common_length = value;
    return NORI_SUCCESS;
  }

  // We technically shouldn't allow users to set headers more than once if we
  // want to be consistent. But certain headers do actually allow repeating and
  // we lost this information anyways when we last flushed.

  nori_assert(r->nr_pending_headers_count < NORI_RESPONSE_HEADER_THRESHOLD);
  r->nr_pending_headers[r->nr_pending_headers_count].nr_key = header;
  r->nr_pending_headers[r->nr_pending_headers_count].nr_val = value;
  r->nr_pending_headers_count += 1;

  // Must suspend at this point to flush pending headers.
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

  // The first time we write in a given request, we transition our state machine.
  // The user can no longer write headers.
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
    return NORI_FAILURE(NORI_ERROR_IMMUTABLE);
  }

  size_t count = 0;
  while (count < len) {
    // Write as much as we can in one go. The main context is responsible for
    // buffering content appropriately.
    ssize_t n = write(res->nr_fd_write, buffer + count, len - count);

    if (n == -1) {
      // Needing to pause at this point should rarely happen. Relinquish control
      // back to the main context and have it resume this for another try later.
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        nori_log_warn("write blocked");
        status = nori_response_suspend(res);
        if (status.ns_error) {
          nori_log_error("Could not suspend coroutine");
          return status;
        }
        continue;
      }

      nori_perror("write");
      return NORI_FAILURE(NORI_ERROR_SYSTEM);
    }

    // Let the main context read in what we just wrote out.
    if (n > 0) {
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

struct nori_status
nori_response_write_file(struct nori_response *const res, struct nori_str path)
{
  if (path.len == 0) {
    return NORI_FAILURE(NORI_ERROR_INVALID_ARG);
  }

  int fd = open(path.ss, O_RDONLY | O_NONBLOCK);

  if (fd == -1) {
    nori_perror("open");
    if (errno == EINTR || errno == EMFILE || errno == ENFILE) {
      return NORI_FAILURE(NORI_ERROR_SYSTEM);
    } else if (errno == ENOMEM) {
      return NORI_FAILURE(NORI_ERROR_NOMEM);
    } else {
      return NORI_FAILURE(NORI_ERROR_INVALID_ARG);
    }
  }

  ssize_t n = 0;
  char buffer[2048] = {};
  struct nori_status status = NORI_SUCCESS;

  while (true) {
    n = read(fd, buffer, sizeof(buffer));

    if (n == -1) {
      // Needing to pause at this point should rarely happen. Relinquish control
      // back to the main context and have it resume this for another try later.
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        nori_log_warn("read blocked");
        status = nori_response_suspend(res);
        if (status.ns_error) {
          nori_log_error("Could not suspend coroutine");
          goto cleanup;
        }
        continue;
      }

      nori_perror("read");
      if (errno == EISDIR) {
        status = NORI_FAILURE(NORI_ERROR_INVALID_ARG);
      } else {
        status = NORI_FAILURE(NORI_ERROR_SYSTEM);
      }
    }

    if (n == 0) {
      goto cleanup;
    }

    status = nori_response_write(res, buffer, n);
  }

cleanup:
  if (close(fd) == -1) {
    nori_perror("close");
  }
  return status;
}
