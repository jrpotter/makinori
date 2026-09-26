Reference
=========

All API functions are prefixed with ``mn_``. Every header except for
:doc:`cmdline_h` (unless :c:macro:`MN_CMDLINE_SOURCE` is defined) is
automatically pulled in using:

.. code-block:: c

   #include "makinori.h"

Fields ending in an underscore (e.g. ``lua_``) should not be accessed directly.
If you come across a use case where it is necessary to do so, please file an
issue.

.. toctree::
   :maxdepth: 1

   cmdline_h
   config_h
   logger_h
   macro_h
   request_h
   response_h
   server_h
   string_h
   util_h
