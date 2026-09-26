Getting Started
===============

Build
-----

We only support building **makinori** from source. Clone the repository at your
location of choice:

.. code-block:: sh

   $ git clone https://git.jrpotter.com/makinori <directory>

To build, you will need both `libwebsockets`_ (>= v4.5) and `lua`_ (>= v5.1).
Install both libraries and adjust the *Configuration* section within the
``Makefile`` to correctly locate them. Once your ``Makefile`` is updated,
run ``make release``. You will find a new ``server`` binary under the ``bin``
directory.

.. _libwebsockets: https://libwebsockets.org/
.. _lua: https://www.lua.org/download.html

Minimal Example
---------------

Test your build against the following minimal ``main.c`` example. Once running,
send an HTTP GET request to ``localhost:8000``. You will receive an HTTP 200 OK
response with body ``Hello, world``.

.. code-block:: c

   #include "makinori.h"

   static struct mn_status
   handler_index(struct mn_request req, struct mn_response *const res)
   {
     return mn_response_write(res, mn_str_lit("Hello, world"));
   }

   static struct mn_route route_index = {
       .method = MN_METHOD_GET,
       .pattern = mn_str_lit("/"),
       .handler = handler_index};

   int main(void)
   {
     struct mn_config config = {};
     mn_config_load(runtime, &config);

     struct mn_server server = {.config = config, .route = route_index};
     mn_server_run(&server);

     mn_config_unload(&config);
   }
