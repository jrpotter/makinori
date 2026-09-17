#pragma once

#include "nori/config.h"
#include "nori/string.h"

enum nori_method {
  NORI_METHOD_GET,
  NORI_METHOD_POST,
};

struct nori_request {
  enum nori_method nr_method;
  struct nori_view nr_path;
};

struct nori_response {
  unsigned int nr_status;
  struct nori_view nr_content_type;
  struct nori_buf nr_content;
};

struct nori_router {
  enum nori_method nr_method;
  struct nori_view nr_path;
  struct nori_router *nr_next;
  struct nori_status (*nr_callback)(struct nori_request, struct nori_response *const);
};

struct nori_server {
  struct nori_config config;
  struct nori_router router;
};

struct nori_status nori_server_run(struct nori_server server[static 1]);
