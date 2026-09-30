request.h
=========

The ``request.h`` module provides functions for interacting with
:c:struct:`mn_request` objects. Learn more at :ref:`ref-concepts-requests`.

API
---

.. c:enum:: mn_method

   An HTTP method.

   .. c:member:: unsigned int MN_METHOD_GET = 0

.. c:struct:: mn_query_param

   A single query param. Given e.g. ``abc=def``, the
   :c:member:`~mn_query_param.key` corresponds to ``abc`` whereas
   :c:member:`~mn_query_param.value` corresponds to ``def``.

   .. c:member:: struct mn_view key

   .. c:member:: struct mn_view value

.. c:struct:: mn_request

   A client request.

   .. c:member:: enum mn_method method

      The HTTP method sent by the client.

   .. c:member:: struct mn_str uri

      The path and query portion of the URI used by the client.

   .. c:member:: struct mn_view path

      The path portion of the URI used by the client.

   .. c:member:: struct mn_query_param query[MN_REQUEST_MAX_QUERY_PARAMS]

      The query parameters sent by the client. There are a total of
      :c:member:`~mn_request.query_count` valid key/value parameters set.

   .. c:member:: size_t query_count

      The number of query parameters sent by the client.

   .. c:member:: struct mn_view captures[MN_REQUEST_MAX_CAPTURES]

      The :ref:`captures <ref-concepts-captures>` pulled from the
      :c:member:`~mn_request.path`.

   .. c:member:: size_t capture_count

      The number of string fragments captured.

.. c:type:: struct mn_status mn_route_handler_t( \
                struct mn_request const, \
                struct mn_response *const)

   A type alias for a user-defined :ref:`handler function
   <ref-concepts-handlers>`.

.. c:struct:: mn_route

   A route. These are pattern-matched against client requests. For more
   information, refer to :ref:`ref-concepts-routing`. The following is a typical
   example of how a router might be defined:

   .. code-block:: c

      static struct mn_route route_root;
      static struct mn_route route_abc;
      static struct mn_route route_def;

      static struct mn_route route_root = {
          .method = MN_METHOD_GET,
          .pattern = mn_str_lit("/"),
          .handler = handle_root,
          .next = &route_abc,
        };

      static struct mn_route route_abc = {
          .method = MN_METHOD_GET,
          .pattern = mn_str_lit("/abc"),
          .handler = handle_abc,
          .next = &route_def,
        };

      static struct mn_route route_def = {
          .method = MN_METHOD_GET,
          .pattern = mn_str_lit("/def"),
          .handler = handle_def,
        };

   .. c:member:: enum mn_method method

      The method used to match against the request.

   .. c:member:: struct mn_str pattern

      The :ref:`pattern <ref-concepts-patterns>` used to match against the
      request.

   .. c:member:: mn_route_handler_t *handler

      The user-defined handler function to be called on a successful match.

   .. c:member:: struct mn_route *next

      The next route to try matching against if this route failed to do so.
