-- The server runs each user-registered callback as a coroutine. This dictates
-- the size of each such coroutine. If you have deeply nested callbacks or
-- heavily use the stack, it may be helpful to bump this up.
COROUTINE_PAGES = 4

-- Emit logs at this level or higher. One of: debug, info, notice, warn, error.
LOG_LEVEL = 'debug'

-- The port the server binds to and serves HTTP requests on.
PORT = 1314
