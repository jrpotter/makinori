config.h
========

The ``config.h`` module represents runtime configuration options as outlined
in :doc:`../config`. Refer to that document for more details on what each option
means.

API
---

.. container:: api

   .. c:type:: struct lua_State mn_lua_t

      An alias to the ``lua_State`` object provided by Lua.

   .. c:enum:: mn_event_loop

      The event loop to use. Currently the only supported value is
      :c:member:`MN_EVENT_LOOP_POLL`.

      .. c:member:: unsigned int MN_EVENT_LOOP_POLL = 0

   .. c:struct:: mn_config

      Configuration options supplied to the :c:struct:`mn_server` on initialization.

      .. c:member:: unsigned long long coro_pages

      The number of virtual memory pages allocated to each coroutine. In actuality,
      one additional page is allocated as a guard to protect against stack
      overflows.

      .. c:member:: enum mn_event_loop ev_loop

      The underlying event loop library used by the :c:struct:`mn_server`.

      .. c:member:: enum mn_log_level log_level

      The minimum log level that should be emitted when running.

      .. c:member:: unsigned long port

      The port that the server will be listening on.

   .. c:function:: struct mn_status mn_config_load( \
                       struct mn_config out[const static 1])

      Loads the default configuration.

      If successful, the :c:expr:`out` parameter must be cleaned up using
      :c:func:`mn_config_unload`.

      :param out: The :c:struct:`mn_config` reference to load.

   .. c:function:: struct mn_status mn_config_load_with( \
                       struct mn_str const path, \
                       struct mn_config out[const static 1])

      Loads the default configuration and then applies any overridden configuration
      options as defined in the file at :c:expr:`path`.

      If successful, the :c:expr:`out` parameter must be cleaned up using
      :c:func:`mn_config_unload`.

      :param path: The location of a lua file to execute.
      :param out: The :c:struct:`mn_config` reference to load.

   .. c:function:: struct mn_status mn_config_unload( \
                       struct mn_config[const static 1])

      Unload the configuration object.

      Should be run once finished with a :c:struct:`mn_config` object
      successfully loaded by a call to :c:func:`mn_config_load` or
      :c:func:`mn_config_load_with`.
