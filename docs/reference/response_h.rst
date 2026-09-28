response.h
==========

The ``response.h`` module provides functions for interacting with
:c:struct:`mn_response` objects. Learn more at :ref:`ref-concepts-responses`.

API
---

.. c:struct:: mn_response

   An opaque object representing the response to send back to the client.

.. c:function:: struct mn_status mn_response_suspend(struct mn_response *const res)

   To be invoked within an :c:type:`mn_route_handler_t`. Suspends
   the current handler, yielding control back to the :ref:`event loop
   <ref-concepts-event-loop>`.

Header
------

.. c:enum:: mn_http_code

   An HTTP status code.

   .. c:member:: unsigned int MN_HTTP_CODE_OK = 200
   .. c:member:: unsigned int MN_HTTP_CODE_CREATED = 201

.. c:function:: struct mn_status mn_response_set_code( \
                    struct mn_response *const res, \
                    enum mn_http_code code)

   Set the HTTP status code on the provided response. If not called, the
   :c:struct:`mn_response` defaults to returning an HTTP 200 OK status.

   :arg res: The response object to write to.
   :arg code: The HTTP status code to write to ``res``.

   :return: | An :c:struct:`mn_status` with value:
            | - :c:member:`MN_ERROR_NONE` on success;
            | - :c:member:`MN_ERROR_INVALID_ARG` on an invalid ``code``;
            | - :c:member:`MN_ERROR_IMMUTABLE` if headers are immutable;
            | - :c:member:`MN_ERROR_DUPLICATE` if code was already set.

.. c:function:: struct mn_status mn_response_set_header( \
                    struct mn_response *const res, \
                    struct mn_str header, \
                    struct mn_str value)

   Set an HTTP header with a value.

   For internal reasons, the ``Content-Type`` and ``Content-Length`` headers are
   treated specially. Neither of these headers may be specified more than once
   on a :c:struct:`mn_response`.

   :arg res: The response object to write to.
   :arg header: The HTTP header being set.
   :arg value: The value to assign to the ``header``.

   :return: | An :c:struct:`mn_status` with value:
            | - :c:member:`MN_ERROR_NONE` on success;
            | - :c:member:`MN_ERROR_INVALID_ARG` on an empty ``header`` or
                ``value``;
            | - :c:member:`MN_ERROR_IMMUTABLE` if headers are immutable;
            | - :c:member:`MN_ERROR_DUPLICATE` if ``Content-Type`` or
                ``Content-Length`` were already set.

Body
----

.. c:function:: struct mn_status mn_response_write( \
                    struct mn_response *const res, \
                    struct mn_str const content)

   Write the contents of ``output`` to the :c:struct:`mn_response`.

   :arg res: The response object to write to.
   :arg output: The content to append to the body.

   :return: | An :c:struct:`mn_status` with value:
            | - :c:member:`MN_ERROR_NONE` on success;
            | - :c:member:`MN_ERROR_IMMUTABLE` if the response body is
                immutable.

.. c:function:: struct mn_status mn_response_write_file( \
                    struct mn_response *const, \
                    struct mn_str const path)

   Write the entirety of the file at ``path`` to the :c:struct:`mn_response`.

   :arg res: The response object to write to.
   :arg path: The path of the file to write to the response body.

   :return: | An :c:struct:`mn_status` with value:
            | - :c:member:`MN_ERROR_NONE` on success;
            | - :c:member:`MN_ERROR_INVALID_ARG` if a file could not be
                found at ``path`` or the file could not be opened (e.g. is
                a directory);
            | - :c:member:`MN_ERROR_IMMUTABLE` if the body is immutable.

.. c:function:: struct mn_status mn_response_write_buffer( \
                    struct mn_response *const, \
                    char const buffer[const static 1], \
                    size_t const len)

   Write the contents of ``buffer`` to the :c:struct:`mn_response`.

   :arg res: The response object to write to.
   :arg buffer: The content to append to the response body.
   :arg len: The number of bytes to append to the body from ``buffer``.

   :return: | An :c:struct:`mn_status` with value:
            | - :c:member:`MN_ERROR_NONE` on success;
            | - :c:member:`MN_ERROR_IMMUTABLE` if the body is immutable.
