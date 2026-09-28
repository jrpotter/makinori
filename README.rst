makinori
========

Welcome to the **makinori** web framework. It is a small but fully-featured
framework, written in C and Lua, in which you can write web applications.
It aspires to be as capable as `Django`_ and `Ruby on Rails`_, but with a
significantly smaller footprint.

You can build the examples in the ``examples`` directory using ``make``:

.. code-block:: sh

   $ make
   $ make bin/cmdline-usage  # To build a single example

Documentation can be viewed `online`_ or generated directly. For the latter,
first install `Sphinx`_. You can then serve the files locally by running:

.. code-block:: sh

   $ make docs
   $ docs/_build/server

.. _Django: https://djangoproject.com
.. _Ruby on Rails: https://rubyonrails.org
.. _online: https://makinori.dev
.. _Sphinx: https://www.sphinx-doc.org/en/master/
