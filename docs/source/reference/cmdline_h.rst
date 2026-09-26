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

By defining instances of :c:struct:`mn_flag` and setting the
:c:var:`MN_FLAG_OPTIONS` variable, you can automatically parse command lines
like the one above. Here is a full example to demonstrate usage. Notice you must
define :c:macro:`MN_CMDLINE_SOURCE` before including **makinori**.

.. code-block:: c

   #include <stdlib.h>

   #define MN_CMDLINE_SOURCE
   #include "makinori.h"

   static struct mn_flag FLAG_HELP = {
       .sflag = mn_str_lit("h"),
       .lflag = mn_str_lit("help"),
       .arity = MN_FLAG_ARITY_ZERO,
   };

   static struct mn_flag FLAG_CONFIG = {
       .sflag = mn_str_lit("c"),
       .lflag = mn_str_lit("config"),
       .arity = MN_FLAG_ARITY_ONE,
   };

   struct mn_flag *MN_FLAG_OPTIONS[] = {
       &FLAG_HELP,
       &FLAG_CONFIG,
       nullptr,
   };

   int main(int argc, char const *argv[argc])
   {
     struct mn_str action = {};
     auto status = mn_cmdline_parse(argc, argv, &action);
     if (status.error) {
       return EXIT_FAILURE;
     }

     if (FLAG_HELP.set) {
       // printf help documentation
       return EXIT_SUCCESS;
     }

     if (mn_str_eq(action, mn_str_lit("run"))) {
       // run server
     } else {
       // printf unknown action
       return EXIT_FAILURE;
     }

     return EXIT_FAILURE;
   }

API
---

.. c:macro:: MN_CMDLINE_SOURCE

   Must be defined before utilities are made available.

.. c:enum:: mn_flag_arity

   The arity of a command line flag.

   .. c:member:: unsigned int MN_FLAG_ARITY_ZERO = 0
   .. c:member:: unsigned int MN_FLAG_ARITY_ONE = 1
   .. c:member:: unsigned int MN_FLAG_ARITY_TWO = 2
   .. c:member:: unsigned int MN_FLAG_ARITY_THREE = 3
   .. c:member:: unsigned int MN_FLAG_ARITY_FOUR = 4
   .. c:member:: unsigned int MN_FLAG_ARITY_FIVE = 5
   .. c:member:: unsigned int MN_FLAG_ARITY_SIX = 6
   .. c:member:: unsigned int MN_FLAG_ARITY_SEVEN = 7

.. c:struct:: mn_flag

   A representation of a command line flag. These should be supplied to
   :c:var:`MN_FLAG_OPTIONS`.

   .. c:member:: struct mn_str vals[MN_FLAG_ARITY_MAX]

      The values following the flag. After a call to :c:func:`mn_cmdline_parse`,
      ``vals[0]`` will contain the first value, ``vals[1]`` contains the second
      value, and so on.

   .. c:member:: struct mn_str sflag

      The "short" flag, e.g. ``h``. An empty string means no short flag exists.

   .. c:member:: struct mn_str lflag

      The "long" flag, e.g. ``help``. An empty string means no long flag exists.

   .. c:member:: enum mn_flag_arity arity

      The number of arguments to expect. The argument count is exact; too few or
      too many and the parser will complain.

   .. c:member:: bool set

      Whether the flag was set. Mostly useful in the case of a flag with
      :c:enum:`mn_flag_arity` zero.

.. c:var:: extern struct mn_flag *MN_FLAG_OPTIONS[]

   Set by the user. A ``nullptr``-terminated list of :c:struct:`mn_flag`
   instances. If two flags have the same :c:member:`sflag` or :c:member:`lflag`,
   the first in the list takes priority.

   Call :c:func:`mn_cmdline_parse` only *after* this array has been set.

.. c:function:: struct mn_status mn_cmdline_parse( \
                    int const argc, \
                    char const *argv[const argc], \
                    struct mn_str action[static 1])

   Parses the command line.

   Call this function only *after* :c:var:`MN_FLAG_OPTIONS` is set.

   :param argc: The ``argc`` as supplied to ``main``.
   :param argv: The ``argv`` as supplied to ``main``.
   :param action: A pointer to the :c:struct:`mn_str` to populate. Only updated
                  if an action is supplied.
