Concepts
========

Though **makinori** is intended to feel familiar to even first time users, there
remain certain concepts that are less common and worth expanding on.

.. _ref-concepts-routing:

Routing
-------

.. _ref-concepts-patterns:

Patterns
^^^^^^^^

.. _ref-concepts-captures:

Captures
^^^^^^^^

.. _ref-concepts-handlers:

Handlers
--------

.. _ref-concepts-requests:

Requests
^^^^^^^^

.. _ref-concepts-responses:

Responses
^^^^^^^^^

.. _ref-concepts-event-loop:

Event Loop
----------

TODO: Primary event loop. Based on poll. Each server instance must be single-threaded.

Coroutines
^^^^^^^^^^

TODO: Each route handler is its own stackful coroutine.

Extensions
^^^^^^^^^^

TODO: How to write your own functions and making sure to suspend appropriately.
