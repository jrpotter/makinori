assert(
  type(COROUTINE_STACK) == 'number' and (
    COROUTINE_STACK >= 1024
  ),
  'COROUTINE_STACK must be >= 1024 bytes'
)

assert(
  type(EVENT_LOOP) == 'string' and (
    EVENT_LOOP == 'poll'
  ),
  "EVENT_LOOP must be one of 'poll'"
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
