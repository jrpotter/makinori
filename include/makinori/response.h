#pragma once

#include "makinori/string.h"

/// A mutable HTTP response object.
///
/// Instances of this `struct` are created by the server and supplied to
/// user-registered callbacks defined in the `struct mn_route`. The user is
/// responsible for updating the response in the handler function.
struct mn_response;

/// A representation of the supported HTTP status codes.
enum mn_http_code {
  MN_HTTP_CODE_OK = 200,
  MN_HTTP_CODE_CREATED = 201,
};

/// Set the HTTP status code.
struct mn_status
mn_response_set_code(struct mn_response *const, enum mn_http_code code);

/// Set an HTTP header value.
struct mn_status mn_response_set_header(
    struct mn_response *const,
    struct mn_str header,
    struct mn_str value);

/// Write buffer into the response body.
struct mn_status mn_response_write(struct mn_response *const, struct mn_str output);

struct mn_status mn_response_write_buffer(
    struct mn_response *const,
    char const buffer[const static 1],
    size_t const len);

/// Write file contents into the response body.
struct mn_status
mn_response_write_file(struct mn_response *const res, struct mn_str path);
