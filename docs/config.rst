Configuration
=============

The **makinori** framework is designed to be configurable. When possible,
configuration options are made available at runtime but options that are more
fundamentally baked into the framework require a build. We touch on the latter
first:

Build Options
-------------

A number of predefined macros are :ref:`available <ref-config_h-build-options>`
that adjust basic compile-time settings. Each of these options should be set
using your compiler's ``-D`` flag or by calling ``#define`` *before* including
**makinori**, e.g.:

   .. code-block:: c

      #define MN_REQUEST_MAX_QUERY_PARAMS 48
      #include "makinori.h"

In addition, when building **makinori** you can pass a ``BUILD_TYPE`` to
``make`` to tweak certain build flags used. Make sure to run ``make clean``
before swapping between the different options:

.. code-block:: sh

   $ make BUILD_TYPE=Debug

The following table describes how each ``BUILD_TYPE`` differs:

==============   ============   ==========
``BUILD_TYPE``   Optimization   Assertions
==============   ============   ==========
Unspecified      Unspecified    Enabled
``Debug``        ``-O0``        Enabled
``Release``      ``-O2``        Disabled
==============   ============   ==========

Runtime
-------

Before :c:func:`starting <mn_server_run>` an :c:struct:`mn_server`, we must load
a valid configuration. Though a default configuration is loaded automatically,
you will more likely than not want to adjust these settings. There are two ways
to go about it.

First, after a call to :c:func:`mn_config_load`, simply update any members
in the newly populated :c:struct:`mn_config` instance before the call to
:c:func:`mn_server_run`. This strategy is simple and effective but tends to be
less flexible than the second approach:

.. code-block:: c

   int main(void)
   {
     struct mn_config config = {};
     auto status = mn_config_load(&config);
     if (status.error) {
       return EXIT_FAILURE;
     }

     // Update configuration object
     config.port = 1314;

     struct mn_server server = {.config = config, .route = route_main};
     status = mn_server_run(&server);

     mn_config_unload(&config);
     return status.error ? EXIT_FAILURE : EXIT_SUCCESS;
   }

The second approach defines a custom ``.lua`` file and sets global values
corresponding to each option. For instance, suppose we write the following in a
file named ``./release.lua``:

.. code-block:: lua

   COROUTINE_PAGES = 8
   LOG_LEVEL = 'notice'

We could then populate an :c:struct:`mn_config` with these values by calling
:c:func:`mn_config_load_with` like so:

.. code-block:: c

   struct mn_config config = {};
   mn_config_load_with(mn_str_lit("./release.lua"), &config);

This approach benefits from additional runtime checks made against the
values. It also easily enables running the server with different settings per
"environment". For example, a similarly defined ``./debug.lua`` file can be
loaded instead when writing your server (e.g. with a much noisier log level
set).

Settings
^^^^^^^^

A default configuration containing the following variables is always loaded. A
call to :c:func:`mn_config_load_with` will run the specified chunk afterward.

.. _ref-config-coroutine-pages:

``COROUTINE_PAGES``
"""""""""""""""""""

Defaults to ``4``. The number of virtual memory pages allocated to each
coroutine. In actuality, one additional page is allocated as a guard to protect
against stack overflows.

Must be an ``integer`` greater than ``0``.

.. _ref-config-log-level:

``LOG_LEVEL``
"""""""""""""

Defaults to ``'debug'``. The minimum log level that emitted when running.

Must be a ``string`` with value one of:

* ``'debug'``
* ``'info'``
* ``'notice'``
* ``'warn'``
* ``'error'``

.. _ref-config-port:

``PORT``
""""""""

Defaults to ``1314``. The port that the server will be listening on.

Must be an ``integer`` with value between ``1`` and ``65535`` inclusive.
