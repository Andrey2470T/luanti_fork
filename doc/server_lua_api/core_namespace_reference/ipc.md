IPC
===

The engine provides a generalized mechanism to enable sharing data between the
different Lua environments (main, mapgen and async).
It is essentially a shared in-memory key-value store.

* `core.ipc_get(key)`:
  * Read a value from the shared data area.
  * `key`: string, should use the `"modname:thing"` convention to avoid conflicts.
  * returns an arbitrary Lua value, or `nil` if this key does not exist
* `core.ipc_set(key, value)`:
  * Write a value to the shared data area.
  * `key`: as above
  * `value`: an arbitrary Lua value, cannot be or contain userdata.

Interacting with the shared data will perform an operation comparable to
(de)serialization on each access.
For that reason modifying references will not have any effect, as in this example:
```lua
core.ipc_set("test:foo", {})
core.ipc_get("test:foo").subkey = "value" -- WRONG!
core.ipc_get("test:foo") -- returns an empty table
```

**Advanced**:

* `core.ipc_cas(key, old_value, new_value)`:
  * Write a value to the shared data area, but only if the previous value
    equals what was given.
    This operation is called Compare-and-Swap and can be used to implement
    synchronization between threads.
  * `key`: as above
  * `old_value`: value compared to using `==` (`nil` compares equal for non-existing keys)
  * `new_value`: value that will be set
  * returns: true on success, false otherwise
* `core.ipc_poll(key, timeout)`:
  * Do a blocking wait until a value (other than `nil`) is present at the key.
  * **IMPORTANT**: You usually don't need this function. Use this as a last resort
    if nothing else can satisfy your use case! None of the Lua environments the
    engine has are safe to block for extended periods, especially on the main
    thread any delays directly translate to lag felt by players.
  * `key`: as above
  * `timeout`: maximum wait time, in milliseconds (positive values only)
  * returns: true on success, false on timeout
