Getting Started
===============

Prerequisites
-------------

We only support building **makinori** from source. To do so, first make sure
the following prerequisites are installed:

1. `git <https://git-scm.com/>`__ to clone the **makinori** repository,
2. `make <https://www.gnu.org/software/make/>`__ to build **makinori**, and
3. `CMake <https://cmake.org/>`__ to build the ``libwebsockets`` submodule.

New Projects
------------

If starting a new project or just looking to experiment with **makinori**, the
easiest way to get started is by cloning the ``scaffold`` project:

.. code-block:: sh

   $ git clone --recurse-submodules https://github.com/makinori-dev/scaffold
   $ cd scaffold && make

This will produce a new top-level ``manage`` executable that you can then use to
start your server, run database migrations (pending), etc.

Libraries
---------

If you instead want to get a copy of the **makinori** library, clone and build
against the ``makinori`` repository:

.. code-block:: sh

   $ git clone --recurse-submodules https://github.com/makinori-dev/makinori.git
   $ cd makinori && make [BUILD_TYPE=Debug|Release]

After ``make`` finishes, you will find the build artifacts under the newly
created ``build`` directory. A copy of the `lua <https://www.lua.org>`__
(v5.5.1) and `libwebsockets <https://libwebsockets.org/>`__ (v4.5) static
libraries are included for convenience.
