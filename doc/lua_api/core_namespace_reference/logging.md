Logging
=======

* `core.debug(...)`
    * Equivalent to `core.log(table.concat({...}, "\t"))`
* `core.log([level,] text)`
    * `level` is one of `"none"`, `"error"`, `"warning"`, `"action"`,
      `"info"`, or `"verbose"`.  Default is `"none"`.
