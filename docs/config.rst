Configuration
=============

Before :c:func:`starting <mn_server_run>` an :c:struct:`mn_server`, we must load
a valid configuration. Though a default configuration is loaded automatically,
you will more likely than not want to adjust these settings. To do so, define
your own custom ``.lua`` file and set globals corresponding to each option.

For instance, suppose we write the following in a file named ``./release.lua``:

.. code-block:: lua

   COROUTINE_PAGES = 8

   LOG_LEVEL = 'notice'

We could then load an :c:struct:`mn_config` with the overrides specified in
``.release.lua`` by calling :c:func:`mn_config_load_with` like so:

.. code-block:: c

   struct mn_config config = {};
   mn_config_load_with(mn_str_lit("./release.lua"), &config);

This mechanism easily enables running the server with different settings per
"environment". For example, a similarly defined ``./debug.lua`` file can be
loaded instead when writing your server (e.g. with a much noisier log level
set).

Options
-------

.. _ref-config-coroutine-pages:

COROUTINE_PAGES
^^^^^^^^^^^^^^^

The number of virtual memory pages allocated to each coroutine. In actuality,
one additional page is allocated as a guard to protect against stack overflows.

**Conditions**

Must be an ``integer`` greater than ``0``.

.. _ref-config-event-loop:

EVENT_LOOP
^^^^^^^^^^

The :ref:`event loop <ref-concepts-event-loop>` to use. Currently the only
supported value is ``'poll'``.

**Conditions**

Must be a ``string`` with value ``'poll'``.

.. _ref-config-log-level:

LOG_LEVEL
^^^^^^^^^

The minimum log level that should be emitted when running.

**Conditions**

Must be a ``string`` with value one of: ``'debug'``, ``'info'``, ``'notice'``,
``'warn'``, or ``'error'``.

.. _ref-config-port:

PORT
^^^^

The port that the server will be listening on.

**Conditions**

Must be an ``integer`` with value between ``1`` and ``65535`` inclusive.
