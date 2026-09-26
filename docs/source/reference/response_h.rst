response.h
==========

The ``response.h`` module provides functions for interacting with
:c:struct:`mn_response` objects. Learn more at :ref:`ref-concepts-responses`.

API
---

.. container:: api

   .. c:enum:: mn_http_code

      An HTTP status code.

      .. c:member:: unsigned int MN_HTTP_CODE_OK = 200
      .. c:member:: unsigned int MN_HTTP_CODE_CREATED = 201

   .. c:struct:: mn_response

      An opaque object representing the response to send back to the client.

   .. c:function:: struct mn_status mn_response_set_code( \
                       struct mn_response *const, \
                       enum mn_http_code code)

      Set the HTTP status code on the provided response. If not called, the
      :c:struct:`mn_response` defaults to returning an HTTP 200 OK status.

      It is an error to call this on a :c:struct:`mn_response` more than once.
      It is an error to call this function after any write to the HTTP body.

   .. c:function:: struct mn_status mn_response_set_header( \
                       struct mn_response *const, \
                       struct mn_str header, \
                       struct mn_str value)

      Set an HTTP header with a value.

      For internal reasons, the ``"Content-Type"`` and ``"Content-Length"`` header
      are treated specially. Neither of these headers may be specified more than
      once on a :c:struct:`mn_response`.

      It is an error to call this function after any write to the HTTP body.

   .. c:function:: struct mn_status mn_response_write( \
                       struct mn_response *const, \
                       struct mn_str output)

      Write the contents of ``output`` to the :c:struct:`mn_response`.

   .. c:function:: struct mn_status mn_response_write_file( \
                       struct mn_response *const, \
                       char const buffer[const static 1], \
                       size_t const len)

      Write the entirety of the file at ``path`` to the :c:struct:`mn_response`.

   .. c:function:: struct mn_status mn_response_write_buffer( \
                       struct mn_response *const, \
                       char const buffer[const static 1], \
                       size_t const len)

      Write the contents of ``buffer`` to the :c:struct:`mn_response`. Stop after
      writing ``len`` bytes.
