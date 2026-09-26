logger.h
========

**makinori** comes equipped with a logger used to emit periodic server updates.
User code is welcome (and encouraged) to use the same utilities for consistent
logging output.

API
---

.. c:enum:: mn_log_level

   The available log levels. Setting a log level using :c:func:`mn_log_set_level`
   enables logs of the given level and higher.

   .. c:member:: unsigned int MN_LOG_LEVEL_DEBUG = 0
   .. c:member:: unsigned int MN_LOG_LEVEL_INFO = 1
   .. c:member:: unsigned int MN_LOG_LEVEL_NOTICE = 2
   .. c:member:: unsigned int MN_LOG_LEVEL_WARN = 3
   .. c:member:: unsigned int MN_LOG_LEVEL_ERROR = 4

.. c:function:: void mn_log_set_level(enum mn_log_level const)

   Set the *per-thread* log level. The default log level is determined by the
   :doc:`configured <../config>` value.

.. c:macro:: mn_log(level, msg, ...)

   Emit a log with contents ``msg`` at log level ``level``.

.. c:macro:: mn_log_debug(msg, ...)

   Emit a log with contents ``msg`` at the :c:member:`MN_LOG_LEVEL_DEBUG`
   level.

.. c:macro:: mn_log_info(msg, ...)

   Emit a log with contents ``msg`` at the :c:member:`MN_LOG_LEVEL_INFO` level.

.. c:macro:: mn_log_notice(msg, ...)

   Emit a log with contents ``msg`` at the :c:member:`MN_LOG_LEVEL_NOTICE`
   level.

.. c:macro:: mn_log_warn(msg, ...)

   Emit a log with contents ``msg`` at the :c:member:`MN_LOG_LEVEL_WARN` level.

.. c:macro:: mn_log_error(msg, ...)

   Emit a log with contents ``msg`` at the :c:member:`MN_LOG_LEVEL_ERROR`
   level.

.. c:macro:: mn_trace(tag, msg, ...)

   Unlike the level-specific logging functions, this emits a log without
   consulting the current log level. Instead, application code is expected to
   introduce a ``#define`` (or similar) to toggle the log. For example:

   .. code-block:: c

      #define TRACE_REQUEST true

      mn_trace(TRACE_REQUEST, "Received request with ID %d", id);

   Traces are not emitted when :c:expr:`NDEBUG` is set.

.. c:macro:: mn_perror(msg, ...)

   A leaner :c:expr:`perror` alternative that only outputs the status code. This
   is useful for two reasons:

   1. This macro's output is consistent with other logging facilities.
   2. :c:expr:`perror` can consume a lot of stack which is potentially problematic
      in small coroutines.
