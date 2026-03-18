`NodeTimerRef`
--------------

Node Timers: a high resolution persistent per-node timer.
Can be gotten via `core.get_node_timer(pos)`.

### Methods

* `set(timeout,elapsed)`
    * set a timer's state
    * `timeout` is in seconds, and supports fractional values (0.1 etc)
    * `elapsed` is in seconds, and supports fractional values (0.1 etc)
    * will trigger the node's `on_timer` function after `(timeout - elapsed)`
      seconds.
* `start(timeout)`
    * start a timer
    * equivalent to `set(timeout,0)`
* `stop()`
    * stops the timer
* `get_timeout()`: returns current timeout in seconds
    * if `timeout` equals `0`, timer is inactive
* `get_elapsed()`: returns current elapsed time in seconds
    * the node's `on_timer` function will be called after `(timeout - elapsed)`
      seconds.
* `is_started()`: returns boolean state of timer
    * returns `true` if timer is started, otherwise `false`
