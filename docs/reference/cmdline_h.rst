cmdline.h
=========

The ``cmdline.h`` module provides an opinionated set of convenience functions
for parsing the command line. In particular, it assumes the presence of zero or
more **flags** and at most one **action**.

* A flag is an argument with a leading hyphen (``-``). It expects zero or more
  values. The number of values it accepts is called its **arity**.
* An action is a positional argument. It accepts *no* values.

As an example, the following command has action ``run`` and flags ``--port`` and
``-c``, both with arity one:

.. code-block:: sh

   $ ./server --port 8000 run -c config.lua

Here is a full example to demonstrate usage.

.. code-block:: c

   #include <stdlib.h>

   #include "makinori.h"

   int main(int argc, char const *argv[argc])
   {
     struct mn_flag flag_help = {
         .sflag = mn_str_lit("h"),
         .lflag = mn_str_lit("help"),
         .arity = 0,
     };

     struct mn_flag flag_config = {
         .sflag = mn_str_lit("c"),
         .lflag = mn_str_lit("config"),
         .arity = 1,
     };

     struct mn_cmdline cl = {.flags = {&flag_help, &flag_config}};

     auto status = mn_cmdline_parse(argc, argv, &cl);
     if (status.error) {
       return EXIT_FAILURE;
     }

     if (flag_help.set) {
       // printf help documentation
       return EXIT_SUCCESS;
     }

     if (mn_str_eq(cl.action, mn_str_lit("run"))) {
       // run server
     } else {
       // printf unknown action
       return EXIT_FAILURE;
     }

     return EXIT_SUCCESS;
   }

Options
-------

.. c:macro:: MN_CMDLINE_MAX_FLAGS

   Defaults to 16. The maximum number of flags that can be parsed. To update,
   define *before* including **makinori**:

   .. code-block:: c

      #define MN_CMDLINE_MAX_FLAGS 32
      #include "makinori.h"

.. c:macro:: MN_CMDLINE_MAX_ARITY

   Defaults to 8. The maximum number of values any one flag can have. To
   update, define *before* including **makinori**:

   .. code-block:: c

      #define MN_CMDLINE_MAX_ARITY 16
      #include "makinori.h"

API
---

.. c:struct:: mn_flag

   A representation of a command line flag.

   .. c:member:: struct mn_str sflag

      The "short" flag, e.g. ``h``. An empty string means no short flag exists.

   .. c:member:: struct mn_str lflag

      The "long" flag, e.g. ``help``. An empty string means no long flag exists.

   .. c:member:: unsigned int arity

      The number of arguments to expect. The argument count is exact; too few or
      too many and the parser will complain.

   .. c:member:: bool set

      Whether the flag was set. Mostly useful in the case of a flag with
      :c:enum:`mn_flag_arity` zero.

   .. c:member:: struct mn_str vals[MN_FLAG_ARITY_MAX]

      The values following the flag. After a call to :c:func:`mn_cmdline_parse`,
      :c:expr:`vals[0]` will contain the first value, :c:expr:`vals[1]` contains
      the second value, and so on.

.. c:struct:: mn_cmdline

   The object populated after a successful call to :c:func:`mn_cmdline_parse`.

   .. c:member:: struct mn_str action

      Either an empty string (if no action is specified) or the single
      positional argument supplied in the command.

   .. c:member:: struct mn_flag *flags

      A list of flags to search the command line for. If two flags have
      the same :c:member:`~mn_flag.sflag` or :c:member:`~mn_flag.lflag`,
      the first in the list takes priority. You must not specify more than
      :c:macro:`MN_CMDLINE_MAX_FLAGS` entries.

      Call :c:func:`mn_cmdline_parse` only *after* this array has been set.

.. c:function:: struct mn_status mn_cmdline_parse( \
                    int const argc, \
                    char const *argv[const argc], \
                    struct mn_cmdline cl[static 1])

   Parses the command line.

   :arg argc: The ``argc`` as supplied to ``main``.
   :arg argv: The ``argv`` as supplied to ``main``.
   :arg cl: A pointer to the :c:struct:`mn_cmdline` to populate.

   :return: | An :c:struct:`mn_status` with value:
            | - :c:member:`MN_ERROR_NONE` on success;
            | - :c:member:`MN_ERROR_CONFIG` on failure.
