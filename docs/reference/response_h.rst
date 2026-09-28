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

Status Codes
------------

.. c:enum:: mn_http_code

   An HTTP status code.

   .. c:member:: unsigned int MN_HTTP_CONTINUE = 100
   .. c:member:: unsigned int MN_HTTP_SWITCHING_PROTOCOLS = 101
   .. c:member:: unsigned int MN_HTTP_PROCESSING = 102
   .. c:member:: unsigned int MN_HTTP_EARLY_HINTS = 103
   .. c:member:: unsigned int MN_HTTP_CODE_OK = 200
   .. c:member:: unsigned int MN_HTTP_CODE_CREATED = 201
   .. c:member:: unsigned int MN_HTTP_ACCEPTED = 202
   .. c:member:: unsigned int MN_HTTP_NON_AUTH_INFO = 203
   .. c:member:: unsigned int MN_HTTP_NO_CONTENT = 204
   .. c:member:: unsigned int MN_HTTP_RESET_CONTENT = 205
   .. c:member:: unsigned int MN_HTTP_PARTIAL_CONTENT = 206
   .. c:member:: unsigned int MN_HTTP_MULTI_STATUS = 207
   .. c:member:: unsigned int MN_HTTP_ALREADY_REPORTED = 208
   .. c:member:: unsigned int MN_HTTP_IM_USED = 226
   .. c:member:: unsigned int MN_HTTP_MULTIPLE_CHOICES = 300
   .. c:member:: unsigned int MN_HTTP_MOVED_PERMANENTLY = 301
   .. c:member:: unsigned int MN_HTTP_FOUND = 302
   .. c:member:: unsigned int MN_HTTP_SEE_OTHER = 303
   .. c:member:: unsigned int MN_HTTP_NOT_MODIFIED = 304
   .. c:member:: unsigned int MN_HTTP_USE_PROXY = 305
   .. c:member:: unsigned int MN_HTTP_TEMPORARY_REDIRECT = 307
   .. c:member:: unsigned int MN_HTTP_PERMANENT_REDIRECT = 308
   .. c:member:: unsigned int MN_HTTP_BAD_REQUEST = 400
   .. c:member:: unsigned int MN_HTTP_UNAUTHORIZED = 401
   .. c:member:: unsigned int MN_HTTP_PAYMENT_REQUIRED = 402
   .. c:member:: unsigned int MN_HTTP_FORBIDDEN = 403
   .. c:member:: unsigned int MN_HTTP_NOT_FOUND = 404
   .. c:member:: unsigned int MN_HTTP_METHOD_NOT_ALLOWED = 405
   .. c:member:: unsigned int MN_HTTP_NOT_ACCEPTABLE = 406
   .. c:member:: unsigned int MN_HTTP_PROXY_AUTH_REQUIRED = 407
   .. c:member:: unsigned int MN_HTTP_REQUEST_TIMEOUT = 408
   .. c:member:: unsigned int MN_HTTP_CONFLICT = 409
   .. c:member:: unsigned int MN_HTTP_GONE = 410
   .. c:member:: unsigned int MN_HTTP_LENGTH_REQUIRED = 411
   .. c:member:: unsigned int MN_HTTP_PRECONDITION_FAILED = 412
   .. c:member:: unsigned int MN_HTTP_CONTENT_TOO_LARGE = 413
   .. c:member:: unsigned int MN_HTTP_URI_TOO_LONG = 414
   .. c:member:: unsigned int MN_HTTP_UNSUPPORTED_MEDIA_TYPE = 415
   .. c:member:: unsigned int MN_HTTP_RANGE_NOT_SATISFIABLE = 416
   .. c:member:: unsigned int MN_HTTP_EXPECTATION_FAILED = 417
   .. c:member:: unsigned int MN_HTTP_IM_A_TEAPOT = 418
   .. c:member:: unsigned int MN_HTTP_MISDIRECTED_REQUEST = 421
   .. c:member:: unsigned int MN_HTTP_UNPROCESSABLE_CONTENT = 422
   .. c:member:: unsigned int MN_HTTP_LOCKED = 423
   .. c:member:: unsigned int MN_HTTP_FAILED_DEPENDENCY = 424
   .. c:member:: unsigned int MN_HTTP_TOO_EARLY = 425
   .. c:member:: unsigned int MN_HTTP_UPGRADE_REQUIRED = 426
   .. c:member:: unsigned int MN_HTTP_PRECONDITION_REQUIRED = 428
   .. c:member:: unsigned int MN_HTTP_TOO_MANY_REQUESTS = 429
   .. c:member:: unsigned int MN_HTTP_FIELDS_TOO_LARGE = 432
   .. c:member:: unsigned int MN_HTTP_UNAVAILABLE_LEGAL = 451
   .. c:member:: unsigned int MN_HTTP_INTERNAL_SERVER_ERROR = 500
   .. c:member:: unsigned int MN_HTTP_NOT_IMPLEMENTED = 501
   .. c:member:: unsigned int MN_HTTP_BAD_GATEWAY = 502
   .. c:member:: unsigned int MN_HTTP_SERVICE_UNAVAILABLE = 503
   .. c:member:: unsigned int MN_HTTP_GATEWAY_TIMEOUT = 504
   .. c:member:: unsigned int MN_HTTP_VERSION_NOT_SUPPORTED = 505
   .. c:member:: unsigned int MN_HTTP_VARIANT_ALSO_NEGOTIATES = 506
   .. c:member:: unsigned int MN_HTTP_INSUFFICIENT_STORAGE = 507
   .. c:member:: unsigned int MN_HTTP_LOOP_DETECTED = 508
   .. c:member:: unsigned int MN_HTTP_NOT_EXTENDED = 510
   .. c:member:: unsigned int MN_HTTP_NETWORK_AUTH_REQUIRED = 511

Header
------

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
