assert(
  type(COROUTINE_PAGES) == 'number' and
  math.type(COROUTINE_PAGES) == 'integer' and
  COROUTINE_PAGES > 0,
  'COROUTINE_PAGES must be > 0'
)

assert(
  type(LOG_LEVEL) == 'string' and (
    LOG_LEVEL == 'debug' or
    LOG_LEVEL == 'info' or
    LOG_LEVEL == 'notice' or
    LOG_LEVEL == 'warn' or
    LOG_LEVEL == 'error'
  ),
  "LOG_LEVEL must be one of 'debug', 'info', 'notice', 'warn', 'error'"
)

assert(
  type(PORT) == 'number' and
  math.type(PORT) == 'integer' and
  PORT >= 1 and
  PORT <= 65535,
  'Port should be an integer between 1 and 65535')
