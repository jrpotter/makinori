Getting Started
===============

Build
-----

We only support building **makinori** from source. To do so, first make sure
the following prerequisites are installed:

1. `git <https://git-scm.com/>`__
1. `make <https://www.gnu.org/software/make/>`__
1. `CMake <https://cmake.org/>`__

You can then clone the repository and build all of the examples by running:

.. code-block:: sh

   $ git clone --recursive-submodules https://github.com/jrpotter/makinori.git
   $ make [BUILD_TYPE=Debug|Release]

You will find the build artifacts under the newly created ``build`` directory.
A copy of the `lua <https://www.lua.org>` (v5.5.1) and `libwebsockets
<https://libwebsockets.org/>` (v4.5) static libraries are included for
convenience.

Minimal Example
---------------

Test your build against the following minimal ``main.c`` example. Once running,
send an HTTP GET request to ``localhost:1314``. You will receive an HTTP 200 OK
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
