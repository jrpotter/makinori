macro.h
=======

The ``macro.h`` module contains a few generally applicable macros.

API
---

.. c:macro:: MN_ARR_SIZE(X)

   Returns the number of elements in an array.

   .. code-block:: c

      int arr[] = {1, 2, 3};
      static_assert(MN_ARR_SIZE(arr) == 3);

.. c:macro:: MN_MAX(X, Y)

   Return the larger of ``X`` and ``Y``.

   .. code-block:: c

      static_assert(MN_MAX(1, 2) == 2);

.. c:macro:: MN_MIN(X, Y)

   Return the smaller of ``X`` and ``Y``.

   .. code-block:: c

      static_assert(MN_MAX(1, 2) == 1);

.. c:macro:: MN_PAIR(T1, T2)

   Produces an anonymous ``struct`` corresponding to a pair of types.

   .. code-block:: c

      struct {
        unsigned int fst;
        void *snd;
      } x;

      MN_PAIR(unsigned int, void *) y; // Same type as `x`

.. c:macro:: MN_STR_LEN(X)

   Returns the number of characters in a statically allocated string.

   .. code-block:: c

      char const* str = "abc";
      static_assert(MN_STR_LEN(str) == 3);

.. c:macro:: MN_STR_TO(X)

   Tokenizes the input argument, expanding arguments once before doing so.

   .. code-block:: c

      static_assert(MN_STR_TO(__LINE__)[0] == '1');
