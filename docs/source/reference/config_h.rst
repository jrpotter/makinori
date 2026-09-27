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

         A reference to the :ref:`ref-config-coroutine-pages` option.

      .. c:member:: enum mn_event_loop ev_loop

         A reference to the :ref:`ref-config-event-loop` option.

      .. c:member:: enum mn_log_level log_level

         A reference to the :ref:`ref-config-log-level` option.

      .. c:member:: unsigned long port

         A reference to the :ref:`ref-config-port` option.

   .. c:function:: struct mn_status mn_config_load( \
                       struct mn_config out[const static 1])

      Loads the default configuration.

      If successful, the :c:expr:`out` parameter must be cleaned up using
      :c:func:`mn_config_unload`.

      :arg out: The :c:struct:`mn_config` reference to load.

      :return: | An :c:struct:`mn_status` with value:
               | - :c:member:`MN_ERROR_NONE` on success;
               | - :c:member:`MN_ERROR_CONFIG` on failure.

      :aborts: If space for :c:macro:`MN_REQUEST_MAX_CAPTURES` could not be
               allocated.

   .. c:function:: struct mn_status mn_config_load_with( \
                       struct mn_str const path, \
                       struct mn_config out[const static 1])

      Loads the default configuration and then applies any overridden configuration
      options as defined in the file at :c:expr:`path`.

      If successful, the :c:expr:`out` parameter must be cleaned up using
      :c:func:`mn_config_unload`.

      :arg path: The location of a lua file to execute.

      :arg out: The :c:struct:`mn_config` reference to load.

      :return: | An :c:struct:`mn_status` with value:
               | - :c:member:`MN_ERROR_NONE` on success;
               | - :c:member:`MN_ERROR_CONFIG` on failure.

      :aborts: If space for :c:macro:`MN_REQUEST_MAX_CAPTURES` could not be
               allocated.

   .. c:function:: void mn_config_unload(struct mn_config c[const static 1])

      Unloads a configuration object.

      Should be called once you are finished using a :c:struct:`mn_config`
      object that was successfully loaded by a call to :c:func:`mn_config_load`
      or :c:func:`mn_config_load_with`.

      :arg c: The configuration to unload.
