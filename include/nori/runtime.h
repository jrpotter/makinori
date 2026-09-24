#pragma once

typedef struct lua_State nori_runtime_t;

nori_runtime_t *const nori_runtime_create(void);

void nori_runtime_destroy(nori_runtime_t *);
