`Settings`
----------

An interface to read config files in the format of `minetest.conf`.

`core.settings` is a `Settings` instance that can be used to access the
main config file (`minetest.conf`). Instances for other config files can be
created via `Settings(filename)`.

Engine settings on the `core.settings` object have internal defaults that
will be returned if a setting is unset.
The engine does *not* (yet) read `settingtypes.txt` for this purpose. This
means that no defaults will be returned for mod settings.

### Methods

* `get(key)`: returns a value
    * Returns `nil` if `key` is not found.
* `get_bool(key, [default])`: returns a boolean
    * `default` is the value returned if `key` is not found.
    * Returns `nil` if `key` is not found and `default` not specified.
* `get_np_group(key)`: returns a NoiseParams table
    * Returns `nil` if `key` is not found.
* `get_flags(key)`:
    * Returns `{flag = true/false, ...}` according to the set flags.
    * Is currently limited to mapgen flags `mg_flags` and mapgen-specific
      flags like `mgv5_spflags`.
    * Returns `nil` if `key` is not found.
* `get_pos(key)`:
    * Returns a `vector`
    * Returns `nil` if no value is found or parsing failed.
* `set(key, value)`
    * Setting names can't contain whitespace or any of `="{}#`.
    * Setting values can't contain the sequence `\n"""`.
    * Setting names starting with "secure." can't be set on the main settings
      object (`core.settings`).
* `set_bool(key, value)`
    * See documentation for `set()` above.
* `set_np_group(key, value)`
    * `value` is a NoiseParams table.
    * Also, see documentation for `set()` above.
* `set_pos(key, value)`
    * `value` is a `vector`.
    * Also, see documentation for `set()` above.
* `remove(key)`: returns a boolean (`true` for success)
* `get_names()`: returns `{key1,...}`
* `has(key)`:
    * Returns a boolean indicating whether `key` exists.
    * In contrast to the various getter functions, `has()` doesn't consider
      any default values.
    * This means that on the main settings object (`core.settings`),
      `get(key)` might return a value even if `has(key)` returns `false`.
* `write()`: returns a boolean (`true` for success)
    * Writes changes to file.
* `to_table()`: returns `{[key1]=value1,...}`

### Format

The settings have the format `key = value`. Example:

    foo = example text
    bar = """
    Multiline
    value
    """
