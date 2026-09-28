#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "./response.h"
#include "makinori/logger.h"
#include "makinori/response.h"
#include "makinori/util.h"

// =================================================================================
// Coroutines

thread_local ucontext_t context_server = {};

struct mn_status mn_response_suspend(struct mn_response *const res)
{
  mn_trace(MN_TRACE_RESPONSE_STATE, "(%p) SUSPENDED", (void *){res});

  // Expect callers to log the error so we know the correct location.
  if (swapcontext(&res->context, &context_server) == -1) {
    return MN_FAILURE(MN_ERROR_SYSTEM);
  }

  return MN_SUCCESS;
}

struct mn_status mn_response_resume(struct mn_response *const res)
{
  mn_trace(MN_TRACE_RESPONSE_STATE, "(%p) RESUMED", (void *){res});

  // Expect callers to log the error so we know the correct location.
  if (swapcontext(&context_server, &res->context) == -1) {
    return MN_FAILURE(MN_ERROR_SYSTEM);
  }

  return MN_SUCCESS;
}

// =================================================================================
// Header

struct mn_status
mn_response_set_code(struct mn_response *const res, enum mn_http_code code)
{
  if (code < MN_HTTP_CODE_OK) {
    return MN_FAILURE(MN_ERROR_INVALID_ARG);
  }

  if (res->state != MN_RESPONSE_STATE_HEADER) {
    return MN_FAILURE(MN_ERROR_IMMUTABLE);
  }

  // Cannot set the HTTP status code more than once.
  if (res->common_code) {
    return MN_FAILURE(MN_ERROR_DUPLICATE);
  }

  res->common_code = code;
  return MN_SUCCESS;
}

struct mn_status mn_response_set_header(
    struct mn_response *const r,
    struct mn_str header,
    struct mn_str value)
{
  if (header.len == 0 || value.len == 0) {
    return MN_FAILURE(MN_ERROR_INVALID_ARG);
  }

  if (r->state != MN_RESPONSE_STATE_HEADER) {
    return MN_FAILURE(MN_ERROR_IMMUTABLE);
  }

  if (mn_view_ieq(header.view, mn_str_lit("Content-Type").view)) {
    if (r->common_type.len > 0) {
      return MN_FAILURE(MN_ERROR_DUPLICATE);
    }
    r->common_type = value;
    return MN_SUCCESS;
  }

  if (mn_view_ieq(header.view, mn_str_lit("Content-Length").view)) {
    if (r->common_length.len > 0) {
      return MN_FAILURE(MN_ERROR_DUPLICATE);
    }
    r->common_length = value;
    return MN_SUCCESS;
  }

  // We technically shouldn't allow users to set headers more than once if we
  // want to be consistent. But certain headers do actually allow repeating and
  // we lost this information anyways when we last flushed.

  mn_assert(r->pending_headers_count < MN_RESPONSE_HEADER_THRESHOLD);
  r->pending_headers[r->pending_headers_count].key = header;
  r->pending_headers[r->pending_headers_count].val = value;
  r->pending_headers_count += 1;

  // Must suspend at this point to flush pending headers.
  if (r->pending_headers_count == MN_RESPONSE_HEADER_THRESHOLD) {
    struct mn_status status = mn_response_suspend(r);
    if (status.error) {
      mn_log_error("Could not suspend coroutine");
      return status;
    }
  }

  return MN_SUCCESS;
}

// =================================================================================
// Body

struct mn_status
mn_response_write(struct mn_response *const res, struct mn_str const content)
{
  return mn_response_write_buffer(res, content.ss, content.len);
}

struct mn_status
mn_response_write_file(struct mn_response *const res, struct mn_str const path)
{
  if (path.len == 0) {
    return MN_FAILURE(MN_ERROR_INVALID_ARG);
  }

  int fd = open(path.ss, O_RDONLY | O_NONBLOCK);

  if (fd == -1) {
    mn_perror("open");
    if (errno == EINTR || errno == EMFILE || errno == ENFILE) {
      return MN_FAILURE(MN_ERROR_SYSTEM);
    } else if (errno == ENOMEM) {
      return MN_FAILURE(MN_ERROR_NOMEM);
    } else {
      return MN_FAILURE(MN_ERROR_INVALID_ARG);
    }
  }

  ssize_t n = 0;
  char buffer[2048] = {};
  struct mn_status status = MN_SUCCESS;

  while (true) {
    n = read(fd, buffer, sizeof(buffer));

    if (n == -1) {
      // Needing to pause at this point should rarely happen. Relinquish control
      // back to the main context and have it resume this for another try later.
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        mn_log_warn("read blocked");
        status = mn_response_suspend(res);
        if (status.error) {
          mn_log_error("Could not suspend coroutine");
          goto cleanup;
        }
        continue;
      }

      mn_perror("read");
      if (errno == EISDIR) {
        status = MN_FAILURE(MN_ERROR_INVALID_ARG);
      } else {
        status = MN_FAILURE(MN_ERROR_SYSTEM);
      }
      goto cleanup;
    }

    if (n == 0) {
      goto cleanup;
    }

    status = mn_response_write_buffer(res, buffer, n);
  }

cleanup:
  if (close(fd) == -1) {
    mn_perror("close");
  }
  return status;
}

struct mn_status mn_response_write_buffer(
    struct mn_response *const res,
    char const buffer[const static 1],
    size_t const len)
{
  struct mn_status status = MN_SUCCESS;

  // The first time we write in a given request, we transition our state machine.
  // The user can no longer write headers.
  if (res->state == MN_RESPONSE_STATE_HEADER) {
    mn_trace(MN_TRACE_RESPONSE_STATE, "(%p) HEADER -> HEADER_FLUSH", (void *){res});
    res->state = MN_RESPONSE_STATE_HEADER_FLUSH;
    status = mn_response_suspend(res);
    if (status.error) {
      mn_log_error("Could not suspend coroutine");
      return status;
    }
    mn_assert(res->state == MN_RESPONSE_STATE_BODY);
  }

  if (res->state != MN_RESPONSE_STATE_BODY) {
    return MN_FAILURE(MN_ERROR_IMMUTABLE);
  }

  size_t count = 0;
  while (count < len) {
    // Write as much as we can in one go. The main context is responsible for
    // buffering content appropriately.
    ssize_t n = write(res->fd_write, buffer + count, len - count);

    if (n == -1) {
      // Needing to pause at this point should rarely happen. Relinquish control
      // back to the main context and have it resume this for another try later.
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        mn_log_warn("write blocked");
        status = mn_response_suspend(res);
        if (status.error) {
          mn_log_error("Could not suspend coroutine");
          return status;
        }
        continue;
      }

      mn_perror("write");
      return MN_FAILURE(MN_ERROR_SYSTEM);
    }

    // Let the main context read in what we just wrote out.
    if (n > 0) {
      status = mn_response_suspend(res);
      if (status.error) {
        mn_log_error("Could not suspend coroutine");
        return status;
      }
    }

    count += n;
  }

  return MN_SUCCESS;
}
