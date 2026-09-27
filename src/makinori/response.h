#pragma once

// Every request spawns a new coroutine (a ucontext). A response object is
// essentially a wrapper around the coroutine stack with some additional
// bookkeeping. The utility functions exposed in the public header wrap I/O
// calls with appropriately timed suspension and resumption calls.

#include <ucontext.h>

#include "makinori/response.h"
#include "makinori/util.h"

// A reference to our main context. Every coroutine links back to this.
extern thread_local ucontext_t context_server;

struct mn_status mn_response_resume(struct mn_response *const res);

// Our choice of libwebsockets means we are bound to some of its design
// decisions. In particular, unless we are willing to hold arbitrary amounts of
// memory "staging" content, the user must write the response header before they
// write the body. Both header and body is written in same callback in the main
// event loop (LWS_CALLBACK_HTTP_WRITEABLE). This enum is used to distinguish
// which actions are valid each time the callback is triggered. For instance,
// once any content to the body is written, this state lets us know any attempts
// to write headers should be met with an error.
enum mn_response_state {
  // Initial state indicating the user is writing headers.
  // Set by the main context.
  MN_RESPONSE_STATE_HEADER = 1,
  // State indicating the user is about to write to the body. Pending headers
  // should be flushed at this point.
  // Set by the user context.
  MN_RESPONSE_STATE_HEADER_FLUSH = 2,
  // State indicating the user is writing to the body.
  // Set by the main context.
  MN_RESPONSE_STATE_BODY = 3,
  // State indicating the user is finished. Pending contents should be flushed
  // at this point.
  // Set by the user context.
  MN_RESPONSE_STATE_BODY_FLUSH = 4,
  // State indicating the user-defined callback has finished.
  // Set by the main context.
  MN_RESPONSE_STATE_CLOSING = 5,
  // State indicating the user-defined callback has finished.
  // Set by the main context.
  MN_RESPONSE_STATE_CLOSED = 6,
};

// Maximum number of headers we store before forcing writing. A balancing act
// between suspending too frequently and making every response a bit bigger.
size_t constexpr MN_RESPONSE_HEADER_THRESHOLD = 8;

// Size of the outbound buffer. Avoid making too large since otherwise LWS in
// turn has to buffer any remaining value, requiring additional heap allocations
// and generally slower processing.
size_t constexpr MN_RESPONSE_BODY_THRESHOLD = 4096;

struct mn_response {
  // The lws context this response is associated with.
  struct lws *wsi;
  // The return status of the user-defined handler.
  struct mn_status status;
  // The docstring for `lws_add_http_common_headers` indicates it should
  // be replaceable using just the LWS public API, but it isn't clear
  // how to do so. This utility seems to update private state that other
  // functions do not touch. As a workaround, save the fields needed by
  // `lws_add_http_common_headers` separately.
  bool common_flushed;
  enum mn_http_code common_code;
  struct mn_str common_type;
  struct mn_str common_length;
  // A reference to the HTTP header that needs to be written out. Switch back to
  // the main context when this buffer is full so we can flush it.
  size_t pending_headers_count;
  struct {
    struct mn_str key;
    struct mn_str val;
  } pending_headers[MN_RESPONSE_HEADER_THRESHOLD];
  // FD of in/out buffers to read/write the response into.
  int fd_read;
  int fd_write;
  // Where in the state machine our coroutine is currently.
  enum mn_response_state state;
  // The stack state, register values, and the (unused) signal state.
  struct ucontext_t context;
  // A memory-mapped region of size COROUTINE_PAGES + 1. The additional page
  // is a guard page, set at the start of the stack, to protect against any
  // accidental stack overflows.
  void *coro_stack;
};
