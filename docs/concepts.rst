Concepts
========

Though **makinori** is intended to feel familiar to even first time users, there
remain certain concepts that are less common and worth expanding on.

.. _ref-concepts-routing:

Routing
-------

A client :c:struct:`request <mn_request>` is compared against a number of
user-defined :c:struct:`routes <mn_route>`. These specify a :c:enum:`method
<mn_method>` field and :c:member:`~mn_route.pattern` field to compare the
request path against. Routes are arranged as a linked list and traversed in
order until a **match** is found. Consider the following snippet:

.. code-block:: c

   static struct mn_route route_first;
   static struct mn_route route_second;
   static struct mn_route route_third;

   static struct mn_route route_first = {
       .method = MN_METHOD_GET,
       .pattern = mn_str_lit("/first"),
       .handler = handle_first,
       .next = &route_second};

   static struct mn_route route_second = {
       .method = MN_METHOD_GET,
       .pattern = mn_str_lit("/second"),
       .handler = handle_second,
       .next = &route_third};

   static struct mn_route route_third = {
       .method = MN_METHOD_GET,
       .pattern = mn_str_lit("/third"),
       .handler = handle_third};

   struct mn_server server = {.config = config, .route = route_first};
   mn_server_run(&server);

In this example, the :c:struct:`server <mn_server>` starts running and waits
on the configured port for a client request. Once received, the HTTP method and
path declared in the request is compared against the ``method`` and ``pattern``
specified in ``route_first``. If both equal, the user-defined ``handle_first``
method is invoked . Otherwise ``next`` is traversed and the process repeats. If
no match is found, **makinori** automatically returns an HTTP 404 Not Found.

.. tip::

   The forward declarations made at the top of the snippet introduce a small
   readability improvement, letting us define :c:struct:`mn_route` instances in
   the order they would be traversed. Otherwise they must be listed in reverse
   order.

Many other frameworks introduce a means of defining routes hierarchically whereas
**makinori** shys away from this for a few reasons:

1. Designing an ergonomic, non-macro-heavy, compile-time interface of a URL
   hierarchy is challenging. Because of limitations in C, a hard-coded limit to
   e.g. nesting depth must be introduced at some point.
2. Though URL hierarchies match the intuition one probably has around resource
   nesting, they also end up being quite prescriptive. Often times one is forced
   to rewrite URLs to workaround typical hierarchical matching algorithms.
3. For those who do want a different interface, a simple linked list is an easy
   target for transforming any URL configuration into.

.. _ref-concepts-patterns:

Patterns
^^^^^^^^

In the snippet above, paths of incoming requests have to equal a route's
:c:member:`~mn_route.pattern` field exactly to constitute a match. By leveraging
**patterns**, we can generalize what dictates a match. For example:

* ``%d`` matches any digit;
* ``[abc]*`` matches any string consisting of letters ``a``, ``b``, and ``c``;
* ``....?`` matches any sequence of three or four characters.

In fact, Lua's `pattern matching`_ mechanism is used directly, so any pattern
supported by Lua is suitable for use. Also note that the ``/`` character found
in URIs has no special status. This means a pattern like ``/.*`` will match
every route.

.. _pattern matching: https://www.lua.org/manual/5.5/manual.html#6.5.1

.. note::

   **makinori** automatically introduces leading anchor ``^`` and trailing
   anchor ``$`` to a pattern if it does not include them.

.. tip::

   If the presence or absence of a trailing slash on a path should be treated
   equivalently, include a ``/?`` suffix to the end of your patterns.

.. _ref-concepts-captures:

Captures
^^^^^^^^

Since Lua's pattern matching facilities are imported wholesale, it should be no
surprise that **captures** are also supported. Captures allow easily referencing
certain parts of a path. For example, suppose the following route matched
against path ``/page/14/2024-12``:

.. code-block:: c

   static struct mn_route route_page = {
       .method = MN_METHOD_GET,
       .pattern = mn_str_lit("/page/(%d+)/(%d%d%d%d)-(%d%d)"),
       .handler = handle_page};

Each parenthesized group denotes a capture of which this particular
pattern defines three. The ``handle_page`` handler will be given an
:c:struct:`mn_request`,  say ``req``, satisfying :c:expr:`req.capture_count ==
3` and

* :c:expr:`req.captures[0]` with value ``14``;
* :c:expr:`req.captures[1]` with value ``2024``;
* :c:expr:`req.captures[2]` with value ``12``.

It is still the user's responsibility to understand the order of captures and to
cast them into different data types if necessary.

.. note::

   Captures can be nested, e.g. ``/(%d(%d)%d)`` is a valid pattern. The
   order of captures in these situations is dictated by *left parentheses*.
   In particular, the first left parenthesis found is that corresponding to
   three enclosed ``%d`` symbols. The second is that corresponding to the
   singular ``%d`` symbol. Therefore a path of e.g. ``/123`` will result in
   ``req.captures[0]`` having value ``123`` and ``req.captures[1]`` having value
   ``2``.

.. _ref-concepts-handlers:

Handlers
--------

As briefly described above, every :c:struct:`mn_route` has an associated
:c:struct:`~mn_route.handler` triggered on match. A handler is intentionally
very simply defined:

.. code-block:: c

   struct mn_status mn_route_handler_t( \
       struct mn_request const, \
       struct mn_response *const)

That is to say, a **handler** is any function that takes in a :c:struct:`request
<mn_request>` and a :c:struct:`response <mn_response>`. It also returns an
:c:struct:`mn_status`. If the handler returns a failing status, the connection
is immediately terminated.

.. _ref-concepts-requests:

Requests
^^^^^^^^

Unsurprisingly, a :c:struct:`request <mn_request>` object contains
details surrounding a client's request. A request of form e.g.
``/page/1/2024-12?order=asc`` is decomposed into the following fields:

* :c:member:`~mn_request.uri` contains the full path and query param string.
   * ``/page/1/2024-12?order=asc``
* :c:member:`~mn_request.path` contains just the path.
   *  ``/page/1/2024-12``
* :c:member:`~mn_request.query_count` contains the number of query params.
   * ``1``
* :c:member:`~mn_request.query` contains the parsed query params.
   * ``{ .key = "order", .value = "asc" }``.

Captures are also included in the request if relevant. These were covered
:ref:`earlier <ref-concepts-captures>`.

.. _ref-concepts-responses:

Responses
^^^^^^^^^

**makinori** defaults to streaming HTTP responses to the client when
possible. When writing to a :c:struct:`mn_response` object using methods like
:c:func:`mn_response_set_code` or :c:func:`mn_response_write`, you are also
writing a response directly to the client (outside of a small buffer period).
This feature happens transparently depending on which protocol the client
requested:

* HTTP/1.0. The connection is terminated once the response is finished streaming
  to indicate the end of the content.
* HTTP/1.1. Header ``Transfer-Encoding: chunked`` is automatically included
  in the response and a special terminating chunk is automatically issued when
  finished.
* HTTP/2. Native ``DATA`` frames are issued. An ``END_STREAM`` flag is
  automatically sent in the final frame.

As a consequence, you *must* finish writing the HTTP headers to the response
before you begin writing the body. Since we default to streaming, it is not
necessary to provide a ``Content-Length`` flag in your responses. You can if you
need to, but the value will need to be calculated and set *before* invoking any
method that writes to the body.

.. _ref-concepts-event-loop:

Event Loop
----------

Every :c:struct:`mn_server` instance runs an event loop. For the most part, this
should be a relatively transparent feature of **makinori**, but there are a few
caveats that should be considered:

1. Each :c:struct:`mn_server` instance must reside in its own thread. To be
   clear, you can run multiple threads without issue. But, within any particular
   response handler, you must not spawn a new thread. To do so invokes the wrath
   of *undefined behavior*.
2. Your :c:type:`mn_route_handler_t` handler functions must work together.
   Avoid hogging the CPU or blocking on I/O since the entire event loop
   necessarily waits alongside your handler. This is elaborated on :ref:`below
   <ref-concepts-coroutines>`.

The default event loop is managed using ``poll``. We plan on supporting other
event loops in the future (namely those already supported by `libwebsockets`_),
but doing so is not a priority. Please file an issue if the need arises.

.. _libwebsockets: https://libwebsockets.org/

.. _ref-concepts-coroutines:

Coroutines
^^^^^^^^^^

As it turns out, every :c:type:`mn_route_handler_t` defined within an
:c:struct:`mn_route` runs within its own stackful coroutine. Methods that
perform I/O (e.g. :c:func:`mn_response_write`) will automatically yield control
at opportune moments, relying on the event loop to eventually resume the
suspended handler (ideally once asynchronous I/O operations are finished).

As such, it is important to write *cooperative* handlers. Avoid locking the CPU
indefinitely or running blocking I/O operations since the entire event loop will
otherwise block as well. For cases where the existing API falls shorts, you can
voluntarily suspend your coroutine using :c:func:`mn_response_suspend`. It is up
to the internal scheduler to eventually resume your handler.
