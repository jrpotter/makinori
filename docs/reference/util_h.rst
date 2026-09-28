util.h
======

The ``util.h`` module contains additional functionality generally used
throughout a **makinori** project.

API
---

.. container:: api

  .. c:enum:: mn_error

     A representation of some error encountered during normal operations.

     .. c:member:: unsigned int MN_ERROR_NONE = 0

        No error.

     .. c:member:: unsigned int MN_ERROR_CONFIG = 1

        An error was encountered when either loading or executing a Lua config.

     .. c:member:: unsigned int MN_ERROR_INVALID_ARG = 2

        An invalid argument was supplied to an API function.

     .. c:member:: unsigned int MN_ERROR_IMMUTABLE = 3

        An attempt was made to update an immutable object.

     .. c:member:: unsigned int MN_ERROR_DUPLICATE = 4

        An attempt was made to create another instance of some object.

     .. c:member:: unsigned int MN_ERROR_SYSTEM = 900

        A generic error, internal to **makinori**, occurred. Please file an issue.

     .. c:member:: unsigned int MN_ERROR_NOMEM = 901

        Memory was requested that could not be allocated.

  .. c:struct:: mn_status

     A general status indicator. Many API functions provided by **makinori**
     prefer to type over e.g. integer status codes for clarity or additional
     context.

     .. c:member:: enum mn_error error

        The error code associated with the status. Considered successful if and only
        if :c:member:`~mn_status.error` is :c:member:`MN_ERROR_NONE`.

     .. c:member:: struct mn_str file

        The file this status instance was generated from.

     .. c:member:: struct mn_str line

        The line number of the file this status instance was generated from.

   .. c:macro:: MN_SUCCESS

      Utility for generating a :c:struct:`mn_status` instance with
      :c:member:`~mn_status.error` :c:member:`MN_ERROR_NONE`.

   .. c:macro:: MN_FAILURE(err)

      Utility for generating a :c:struct:`mn_status` instance with
      :c:member:`~mn_status.error` set to ``err``.

   .. c:macro:: MN_FAILURE_EMIT(err, lvl, msg, ...)

      Utility for :doc:`logging <logger_h>` a ``msg`` at log level
      ``lvl`` and then returning a :c:struct:`mn_status` instance with
      :c:member:`~mn_status.error` set to ``err``.

   .. c:macro:: MN_WARN_EMIT(err, msg, ...)

      Utility for :doc:`logging <logger_h>` a ``msg`` at log level
      :c:member:`MN_LOG_LEVEL_WARN` and then returning a :c:struct:`mn_status`
      instance with :c:member:`~mn_status.error` set to ``err``.

   .. c:macro:: MN_ERROR_EMIT(err, msg, ...)

      Utility for :doc:`logging <logger_h>` a ``msg`` at log level
      :c:member:`MN_LOG_LEVEL_ERROR` and then returning a :c:struct:`mn_status`
      instance with :c:member:`~mn_status.error` set to ``err``.

   .. c:macro:: mn_assert(condition)

      Asserts the provided ``condition`` at runtime. Calls ``abort()`` if
      ``false``. Generally prefer this to ``assert`` since it outputs error
      messages in the same format as other logs.

      Does nothing if ``NDEBUG`` is defined.
