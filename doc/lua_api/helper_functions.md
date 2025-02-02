Helper functions
================

* `dump2(obj, name, dumped)`: returns a string which makes `obj`
  human-readable, handles reference loops.
    * `obj`: arbitrary variable
    * `name`: string, default: `"_"`
    * `dumped`: table, default: `{}`
* `dump(obj, dumped)`: returns a string which makes `obj` human-readable
    * `obj`: arbitrary variable
    * `dumped`: table, default: `{}`
* `math.hypot(x, y)`
    * Get the hypotenuse of a triangle with legs x and y.
      Useful for distance calculation.
* `math.sign(x, tolerance)`: returns `-1`, `0` or `1`
    * Get the sign of a number.
    * tolerance: number, default: `0.0`
    * If the absolute value of `x` is within the `tolerance` or `x` is NaN,
      `0` is returned.
* `math.factorial(x)`: returns the factorial of `x`
* `math.round(x)`: Returns `x` rounded to the nearest integer.
    * At a multiple of 0.5, rounds away from zero.
* `string.split(str, separator, include_empty, max_splits, sep_is_pattern)`
    * `separator`: string, cannot be empty, default: `","`
    * `include_empty`: boolean, default: `false`
    * `max_splits`: number, if it's negative, splits aren't limited,
      default: `-1`
    * `sep_is_pattern`: boolean, it specifies whether separator is a plain
      string or a pattern (regex), default: `false`
    * e.g. `"a,b":split","` returns `{"a","b"}`
* `string:trim()`: returns the string without whitespace pre- and suffixes
    * e.g. `"\n \t\tfoo bar\t ":trim()` returns `"foo bar"`
* `core.wrap_text(str, limit, as_table)`: returns a string or table
    * Adds newlines to the string to keep it within the specified character
      limit
    * Note that the returned lines may be longer than the limit since it only
      splits at word borders.
    * `limit`: number, maximal amount of characters in one line
    * `as_table`: boolean, if set to true, a table of lines instead of a string
      is returned, default: `false`
* `core.pos_to_string(pos, decimal_places)`: returns string `"(X,Y,Z)"`
    * `pos`: table {x=X, y=Y, z=Z}
    * Converts the position `pos` to a human-readable, printable string
    * `decimal_places`: number, if specified, the x, y and z values of
      the position are rounded to the given decimal place.
* `core.string_to_pos(string)`: returns a position or `nil`
    * Same but in reverse.
    * If the string can't be parsed to a position, nothing is returned.
* `core.string_to_area("(X1, Y1, Z1) (X2, Y2, Z2)", relative_to)`:
    * returns two positions
    * Converts a string representing an area box into two positions
    * X1, Y1, ... Z2 are coordinates
    * `relative_to`: Optional. If set to a position, each coordinate
      can use the tilde notation for relative positions
    * Tilde notation
      * `"~"`: Relative coordinate
      * `"~<number>"`: Relative coordinate plus `<number>`
    * Example: `core.string_to_area("(1,2,3) (~5,~-5,~)", {x=10,y=10,z=10})`
      returns `{x=1,y=2,z=3}, {x=15,y=5,z=10}`
* `core.formspec_escape(string)`: returns a string
    * escapes the characters "[", "]", "\", "," and ";", which cannot be used
      in formspecs.
* `core.is_yes(arg)`
    * returns true if passed 'y', 'yes', 'true' or a number that isn't zero.
* `core.is_nan(arg)`
    * returns true when the passed number represents NaN.
* `core.get_us_time()`
    * returns time with microsecond precision. May not return wall time.
* `table.copy(table)`: returns a table
    * returns a deep copy of `table`
* `table.indexof(list, val)`: returns the smallest numerical index containing
      the value `val` in the table `list`. Non-numerical indices are ignored.
      If `val` could not be found, `-1` is returned. `list` must not have
      negative indices.
* `table.keyof(table, val)`: returns the key containing
      the value `val` in the table `table`. If multiple keys contain `val`,
      it is unspecified which key will be returned.
      If `val` could not be found, `nil` is returned.
* `table.insert_all(table, other_table)`:
    * Appends all values in `other_table` to `table` - uses `#table + 1` to
      find new indices.
* `table.key_value_swap(t)`: returns a table with keys and values swapped
    * If multiple keys in `t` map to the same value, it is unspecified which
      value maps to that key.
* `table.shuffle(table, [from], [to], [random_func])`:
    * Shuffles elements `from` to `to` in `table` in place
    * `from` defaults to `1`
    * `to` defaults to `#table`
    * `random_func` defaults to `math.random`. This function receives two
      integers as arguments and should return a random integer inclusively
      between them.
* `core.pointed_thing_to_face_pos(placer, pointed_thing)`: returns a
  position.
    * returns the exact position on the surface of a pointed node
* `core.get_tool_wear_after_use(uses [, initial_wear])`
    * Simulates a tool being used once and returns the added wear,
      such that, if only this function is used to calculate wear,
      the tool will break exactly after `uses` times of uses
    * `uses`: Number of times the tool can be used
    * `initial_wear`: The initial wear the tool starts with (default: 0)
* `core.get_dig_params(groups, tool_capabilities [, wear])`:
    Simulates an item that digs a node.
    Returns a table with the following fields:
    * `diggable`: `true` if node can be dug, `false` otherwise.
    * `time`: Time it would take to dig the node.
    * `wear`: How much wear would be added to the tool (ignored for non-tools).
    `time` and `wear` are meaningless if node's not diggable
    Parameters:
    * `groups`: Table of the node groups of the node that would be dug
    * `tool_capabilities`: Tool capabilities table of the item
    * `wear`: Amount of wear the tool starts with (default: 0)
* `core.get_hit_params(groups, tool_capabilities [, time_from_last_punch [, wear]])`:
    Simulates an item that punches an object.
    Returns a table with the following fields:
    * `hp`: How much damage the punch would cause (between -65535 and 65535).
    * `wear`: How much wear would be added to the tool (ignored for non-tools).
    Parameters:
    * `groups`: Damage groups of the object
    * `tool_capabilities`: Tool capabilities table of the item
    * `time_from_last_punch`: time in seconds since last punch action
    * `wear`: Amount of wear the item starts with (default: 0)
