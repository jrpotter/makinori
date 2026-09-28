#pragma once

#include "makinori/string.h"

struct mn_response;

struct mn_status mn_response_suspend(struct mn_response *const res);

// =================================================================================
// Headers

enum mn_http_code : unsigned int {
  MN_HTTP_CODE_OK = 200,
  MN_HTTP_CODE_CREATED = 201,
};

struct mn_str constexpr MN_HEADER_CONTENT_TYPE = mn_str_lit("Content-Type");

struct mn_str constexpr MN_MEDIA_TYPE_CSS = mn_str_lit("text/css");
struct mn_str constexpr MN_MEDIA_TYPE_HTML = mn_str_lit("text/html");
struct mn_str constexpr MN_MEDIA_TYPE_JAVASCRIPT = mn_str_lit("text/javascript");

struct mn_status
mn_response_set_code(struct mn_response *const, enum mn_http_code code);

struct mn_status mn_response_set_header(
    struct mn_response *const res,
    struct mn_str header,
    struct mn_str value);

// =================================================================================
// Body

struct mn_status
mn_response_write(struct mn_response *const res, struct mn_str const content);

struct mn_status
mn_response_write_file(struct mn_response *const res, struct mn_str const path);

struct mn_status mn_response_write_buffer(
    struct mn_response *const res,
    char const buffer[const static 1],
    size_t const len);
