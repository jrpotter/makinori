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

Documentation can be viewed `online`_ or built locally. For the latter, you will
first need to install `Sphinx`_. You can then view the HTML documentation in
``docs/_build`` after running:

.. code-block:: sh

   $ make docs MODE=html

.. _Django: https://djangoproject.com
.. _Ruby on Rails: https://rubyonrails.org
.. _online: https://makinori.dev
.. _Sphinx: https://www.sphinx-doc.org/en/master/
