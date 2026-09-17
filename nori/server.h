#pragma once

#include "nori/config.h"

struct nori_request {
  struct nori_str nr_path;
};

struct nori_router {
  struct nori_str nr_path;
  struct nori_router *nr_next;
  void (*nr_callback)(struct nori_request);
};

struct nori_server {
  struct nori_config config;
  struct nori_router router;
};

struct nori_status nori_server_run(struct nori_server server[static 1]);
