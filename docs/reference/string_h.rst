string.h
========

The ``string.h`` module provides utility methods for interacting with strings
in a consistent and safe manner.

API
---

Views
^^^^^

.. c:struct:: mn_view

   A portion of a string. Unlike the :c:struct:`mn_str` type, this type only
   refers to part of a **backing string**. This backing string must remain valid
   for the duration of a :c:struct:`mn_view` instance's lifetime.

   .. c:member:: size_t len

      The length of the referenced portion of some string.

.. c:macro:: PRImnv

   A utility macro for including the contents of a :c:struct:`mn_view` into
   a :c:expr:`printf` call. Should be used with :c:macro:`mn_view_pri`. For
   example:

   .. code-block:: c

      struct mn_view const v = mn_view_ref("hello, world", 5);
      printf("View: " PRImnv "\n", mn_view_pri(v));
      // Outputs: View hello\n

.. c:macro:: mn_view_pri(X)

   A utility macro for including the contents of a :c:struct:`mn_view` into a
   :c:expr:`printf` call. Should be used with :c:macro:`PRImnv`. For example:

   .. code-block:: c

      struct mn_view const v = mn_view_ref("hello, world", 5);
      printf("View: " PRImnv "\n", mn_view_pri(v));
      // Outputs: View hello\n

.. c:macro:: mn_view_lit(X)

   Create a new :c:struct:`mn_view` instance pointing at C-string literal ``X``.
   It is assumed ``X`` does not contain embedded ``NUL`` characters.

   Generally speaking, you should prefer :c:macro:`mn_str_lit`. But this is a
   nice shorthand in cases where you only ever want to access the literal as
   a view.

.. c:function:: struct mn_view const mn_view_ref( \
                    char const ss[static 1], \
                    size_t const len)

   Create a new :c:struct:`mn_view` pointing to ``ss`` with length ``len``.

   :arg ss: The C-string to point to. Must remain valid during the lifetime of
            the :c:struct:`mn_view` instance.
   :arg len: The length of the portion of the string the :c:struct:`mn_view`
             references.

.. c:function:: bool mn_view_eq(struct mn_view const, struct mn_view const)

   Case-sensitive equality checking. Checks two :c:struct:`mn_view` instances
   are equal, byte-per-byte.

.. c:function:: bool mn_view_ieq(struct mn_view const, struct mn_view const)

   Case-insensitive equality checking. Checks two :c:struct:`mn_view` instances
   are equal, ignoring case for ASCII characters. In other words, this only
   works as outlined when restricting attention to just strings in the BMP.

.. c:function:: struct mn_view const mn_view_substr( \
                    struct mn_view const v, \
                    size_t const i, \
                    size_t const j)

   Create a subview of another.

   :arg v: The :c:struct:`mn_view` to take a subview of.
   :arg i: The starting index of the subview, inclusive.
   :arg j: The ending index of the subview, exclusive.

.. c:function:: size_t mn_view_cpy( \
                    char *const dst, \
                    struct mn_view const src, \
                    size_t const count)

   Copy the contents of ``src`` into ``dst``.

   :arg dst: The buffer to copy ``src`` into.
   :arg src: The string to copy into ``dst``.
   :arg count: *OPTIONAL*. The maximum number of characters to copy. Ignored
               if greater than ``src``'s :c:member:`~mn_view.len`. If excluded,
               ``SIZE_MAX`` is assumed.

.. c:function:: size_t mn_view_find( \
                    struct mn_view const haystack, \
                    struct mn_view const needle)

   Find the first index of ``haystack`` in which subview ``needle`` is found.

   :arg haystack: The view to search the ``needle`` in.
   :arg needle: The subview to search for.

   :return: The index the ``needle`` was found starting at. If not present,
            ``haystack.len`` is returned instead.

Strings
^^^^^^^

.. c:struct:: mn_str

   A thin wrapper around a C-style string. Unlike the :c:struct:`mn_view` type,
   this type always refers to the entirety of a **backing string**. This backing
   string must remain valid for the duration of a :c:struct:`mn_str` instance's
   lifetime.

   This type is primarily motivated as a convenient means of referencing C-style
   strings' lengths.

   .. c:member:: char const *ss

      The backing string.

   .. c:member:: size_t len

      The length of the backing string excluding the trailing ``NUL`` character.

.. c:macro:: mn_str_lit(X)

   Create a new :c:struct:`mn_str` instance pointing at C-string literal
   ``X``. It is assumed ``X`` does not contain embedded ``NUL`` characters.

.. c:function:: struct mn_str const mn_str_ref( \
                    char const ss[static 1], \
                    size_t const len)

   Create a new :c:struct:`mn_str` pointing to ``ss`` with length ``len``.
   It is assumed ``ss`` does not contain embedded ``NUL`` characters and is
   ``NUL``-terminated with :c:expr:`ss[len] == '\\0'`.

   :arg ss: The C-string to point to. Must remain valid during the lifetime of
            the :c:struct:`mn_str` instance.
   :arg len: The length of ``ss`` excluding the trailing ``NUL`` character.
