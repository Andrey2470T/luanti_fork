Timing
======

* `core.after(time, func, ...)`: returns job table to use as below.
    * Call the function `func` after `time` seconds, may be fractional
    * Optional: Variable number of arguments that are passed to `func`
    * Jobs set for earlier times are executed earlier. If multiple jobs expire
      at exactly the same time, then they are executed in registration order.
    * `time` is a lower bound. The job is executed in the first server-step that
      started at least `time` seconds after the last time a server-step started,
      measured with globalstep dtime.
    * If `time` is `0`, the job is executed in the next step.

* `job:cancel()`
    * Cancels the job function from being called
