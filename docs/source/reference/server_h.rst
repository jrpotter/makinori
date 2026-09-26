server.h
========

The ``server.h`` module provides the entrypoint for running a
:c:struct:`mn_server` instance.

API
---

.. container:: api

   .. c:struct:: mn_server

      .. c:member:: struct mn_config config

         The runtime configuration settings needed by the :c:struct:`mn_server`.

      .. c:member:: struct mn_route route

         The head of the :c:struct:`mn_route` linked list to compare during
         :ref:`pattern matching <ref-concepts-patterns>`.

   .. c:function:: struct mn_status mn_server_run(struct mn_server[static 1])

      Spawn an :ref:`event loop <ref-concepts-event-loop>` that waits
      for new :c:struct:`mn_request` instances. Passes these, along
      with a :c:struct:`mn_response` instance, to the user-defined
      :c:type:`mn_route_handler_t` of a :c:struct:`mn_route` that matches the
      request.
