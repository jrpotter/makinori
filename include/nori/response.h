#pragma once

#include "nori/string.h"

/// A mutable HTTP response object.
///
/// Instances of this `struct` are created by the server and supplied to
/// user-registered callbacks defined in the `struct nori_route`. The user
/// is responsible for updating the response in the callback function.
struct nori_response;

/// A representation of the supported HTTP status codes.
enum nori_http_code {
  NORI_HTTP_CODE_OK = 200,
  NORI_HTTP_CODE_CREATED = 201,
};

/// Set the HTTP status code.
struct nori_status
nori_response_set_code(struct nori_response *const, enum nori_http_code code);

/// Set an HTTP header value.
struct nori_status nori_response_set_header(
    struct nori_response *const,
    struct nori_str header,
    struct nori_str value);

/// Write buffer into the response body.
struct nori_status nori_response_write(
    struct nori_response *const,
    char buffer[const static 1],
    size_t const len);

/// Write file contents into the response body.
struct nori_status
nori_response_write_file(struct nori_response *const res, struct nori_str path);
