-- The server runs each user-registered callback as a coroutine. This dictates
-- the size of each such coroutine. Generally this doesn't need to be large.
-- If you have deeply nested callbacks with lots of locals though, it may be
-- helpful to bump this up.
COROUTINE_STACK = 256 * 64

-- The event loop library used. One of poll.
EVENT_LOOP = 'poll'

-- Emit logs at this level or higher. One of debug, info, notice, warn, or error.
LOG_LEVEL = 'debug'

-- The port the server binds to and receives HTTP requests on.
PORT = 8000
