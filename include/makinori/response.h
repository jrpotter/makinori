#pragma once

#include "makinori/string.h"

enum mn_http_code : unsigned int {
  MN_HTTP_CODE_OK = 200,
  MN_HTTP_CODE_CREATED = 201,
};

struct mn_response;

struct mn_status
mn_response_set_code(struct mn_response *const, enum mn_http_code code);

struct mn_status mn_response_set_header(
    struct mn_response *const,
    struct mn_str header,
    struct mn_str value);

struct mn_status mn_response_write(struct mn_response *const, struct mn_str output);

struct mn_status mn_response_write_file(struct mn_response *const, struct mn_str path);

struct mn_status mn_response_write_buffer(
    struct mn_response *const,
    char const buffer[const static 1],
    size_t const len);
