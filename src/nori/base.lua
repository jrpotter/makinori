-- The server runs each user-registered callback as a coroutine. This dictates
-- the size of each such coroutine. Generally this doesn't need to be large,
-- especially considering one still does have access to the heap. If you have
-- deeply nested callbacks with lots of locals though, it may be helpful to bump
-- this up.
COROUTINE_STACK = 4096

-- The event loop library used. One of: poll.
EVENT_LOOP = 'poll'

-- Emit logs at this level or higher. One of: debug, info, notice, warn, error.
LOG_LEVEL = 'debug'

-- The port the server binds to and serves HTTP requests on.
PORT = 8000
