#pragma once

typedef struct lua_State mn_runtime_t;

mn_runtime_t *const mn_runtime_create(void);

void mn_runtime_destroy(mn_runtime_t *);
