Spatial Vectors
===============

Refer to `server_lua_api/spatial_vectors.md`.

Helper functions
----------------
* `dump2(obj, name="_", dumped={})`
     * Return object serialized as a string, handles reference loops
* `dump(obj, dumped={})`
    * Return object serialized as a string
* `math.hypot(x, y)`
    * Get the hypotenuse of a triangle with legs x and y.
      Useful for distance calculation.
* `math.sign(x, tolerance)`
    * Get the sign of a number.
      Optional: Also returns `0` when the absolute value is within the tolerance (default: `0`)
* `string.split(str, separator=",", include_empty=false, max_splits=-1, sep_is_pattern=false)`
    * If `max_splits` is negative, do not limit splits.
    * `sep_is_pattern` specifies if separator is a plain string or a pattern (regex).
    * e.g. `string:split("a,b", ",") == {"a","b"}`
* `string:trim()`
    * e.g. `string.trim("\n \t\tfoo bar\t ") == "foo bar"`
* `core.wrap_text(str, limit)`: returns a string
    * Adds new lines to the string to keep it within the specified character limit
    * limit: Maximal amount of characters in one line
* `core.pos_to_string({x=X,y=Y,z=Z}, decimal_places))`: returns string `"(X,Y,Z)"`
    * Convert position to a printable string
      Optional: 'decimal_places' will round the x, y and z of the pos to the given decimal place.
* `core.string_to_pos(string)`: returns a position
    * Same but in reverse. Returns `nil` if the string can't be parsed to a position.
* `core.string_to_area("(X1, Y1, Z1) (X2, Y2, Z2)")`: returns two positions
    * Converts a string representing an area box into two positions
* `core.is_yes(arg)`
    * returns whether `arg` can be interpreted as yes
* `core.is_nan(arg)`
    * returns true when the passed number represents NaN.
* `table.copy(table)`: returns a table
    * returns a deep copy of `table`
