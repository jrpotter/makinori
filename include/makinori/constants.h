#pragma once

#include "makinori/string.h"

// =================================================================================
// Headers

struct mn_str constexpr MN_HEADER_CONTENT_TYPE = mn_str_lit("Content-Type");
struct mn_str constexpr MN_HEADER_LOCATION = mn_str_lit("Location");

// =================================================================================
// Media Types

struct mn_str constexpr MN_MEDIA_TYPE_CSS = mn_str_lit("text/css");
struct mn_str constexpr MN_MEDIA_TYPE_HTML = mn_str_lit("text/html");
struct mn_str constexpr MN_MEDIA_TYPE_JAVASCRIPT = mn_str_lit("text/javascript");
