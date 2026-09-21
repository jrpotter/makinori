#pragma once

#include "nori/string.h"

/// A representation of supported HTTP methods.
enum nori_method {
  NORI_METHOD_GET,
};

/// An immutable HTTP request object.
///
/// Instances of this `struct` are created by the server and supplied to
/// user-registered callbacks defined in the `struct nori_route`.
struct nori_request {
  enum nori_method nr_method;
  struct nori_str_view nr_path;
};
