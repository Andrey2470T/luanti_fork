Spatial Vectors
===============

Luanti stores 3-dimensional spatial vectors in Lua as tables of 3 coordinates,
and has a class to represent them (`vector.*`), which this chapter is about.
For details on what a spatial vectors is, please refer to Wikipedia:
https://en.wikipedia.org/wiki/Euclidean_vector.

Spatial vectors are used for various things, including, but not limited to:

* any 3D spatial vector (x/y/z-directions)
* Euler angles (pitch/yaw/roll in radians) (Spatial vectors have no real semantic
  meaning here. Therefore, most vector operations make no sense in this use case.)

Note that they are *not* used for:

* n-dimensional vectors where n is not 3 (ie. n=2)
* arrays of the form `{num, num, num}`

The API documentation may refer to spatial vectors, as produced by `vector.new`,
by any of the following notations:

* `(x, y, z)` (Used rarely, and only if it's clear that it's a vector.)
* `vector.new(x, y, z)`
* `{x=num, y=num, z=num}` (Even here you are still supposed to use `vector.new`.)

Compatibility notes
-------------------

Vectors used to be defined as tables of the form `{x = num, y = num, z = num}`.
Since version 5.5.0, vectors additionally have a metatable to enable easier use.
Note: Those old-style vectors can still be found in old mod code. Hence, mod and
engine APIs still need to be able to cope with them in many places.

Manually constructed tables are deprecated and highly discouraged. This interface
should be used to ensure seamless compatibility between mods and the Luanti API.
This is especially important to callback function parameters and functions overwritten
by mods.
Also, though not likely, the internal implementation of a vector might change in
the future.
In your own code, or if you define your own API, you can, of course, still use
other representations of vectors.

Vectors provided by API functions will provide an instance of this class if not
stated otherwise. Mods should adapt this for convenience reasons.

Special properties of the class
-------------------------------

Vectors can be indexed with numbers and allow method and operator syntax.

All these forms of addressing a vector `v` are valid:
`v[1]`, `v[3]`, `v.x`, `v[1] = 42`, `v.y = 13`
Note: Prefer letter over number indexing for performance and compatibility reasons.

Where `v` is a vector and `foo` stands for any function name, `v:foo(...)` does
the same as `vector.foo(v, ...)`, apart from deprecated functionality.

`tostring` is defined for vectors, see `vector.to_string`.

The metatable that is used for vectors can be accessed via `vector.metatable`.
Do not modify it!

All `vector.*` functions allow vectors `{x = X, y = Y, z = Z}` without metatables.
Returned vectors always have a metatable set.

Common functions and methods
----------------------------

For the following functions (and subchapters),
`v`, `v1`, `v2` are vectors,
`p1`, `p2` are position vectors,
`s` is a scalar (a number),
vectors are written like this: `(x, y, z)`:

* `vector.new([a[, b, c]])`:
    * Returns a new vector `(a, b, c)`.
    * Deprecated: `vector.new()` does the same as `vector.zero()` and
      `vector.new(v)` does the same as `vector.copy(v)`
* `vector.zero()`:
    * Returns a new vector `(0, 0, 0)`.
* `vector.random_direction()`:
    * Returns a new vector of length 1, pointing into a direction chosen uniformly at random.
* `vector.copy(v)`:
    * Returns a copy of the vector `v`.
* `vector.from_string(s[, init])`:
    * Returns `v, np`, where `v` is a vector read from the given string `s` and
      `np` is the next position in the string after the vector.
    * Returns `nil` on failure.
    * `s`: Has to begin with a substring of the form `"(x, y, z)"`. Additional
           spaces, leaving away commas and adding an additional comma to the end
           is allowed.
    * `init`: If given starts looking for the vector at this string index.
* `vector.to_string(v)`:
    * Returns a string of the form `"(x, y, z)"`.
    *  `tostring(v)` does the same.
* `vector.direction(p1, p2)`:
    * Returns a vector of length 1 with direction `p1` to `p2`.
    * If `p1` and `p2` are identical, returns `(0, 0, 0)`.
* `vector.distance(p1, p2)`:
    * Returns zero or a positive number, the distance between `p1` and `p2`.
* `vector.length(v)`:
    * Returns zero or a positive number, the length of vector `v`.
* `vector.normalize(v)`:
    * Returns a vector of length 1 with direction of vector `v`.
    * If `v` has zero length, returns `(0, 0, 0)`.
* `vector.floor(v)`:
    * Returns a vector, each dimension rounded down.
* `vector.ceil(v)`:
    * Returns a vector, each dimension rounded up.
* `vector.round(v)`:
    * Returns a vector, each dimension rounded to nearest integer.
    * At a multiple of 0.5, rounds away from zero.
* `vector.sign(v, tolerance)`:
    * Returns a vector where `math.sign` was called for each component.
    * See [Helper functions] for details.
* `vector.abs(v)`:
    * Returns a vector with absolute values for each component.
* `vector.apply(v, func, ...)`:
    * Returns a vector where the function `func` has been applied to each
      component.
    * `...` are optional arguments passed to `func`.
* `vector.combine(v, w, func)`:
    * Returns a vector where the function `func` has combined both components of `v` and `w`
      for each component
* `vector.equals(v1, v2)`:
    * Returns a boolean, `true` if the vectors are identical.
* `vector.sort(v1, v2)`:
    * Returns in order minp, maxp vectors of the cuboid defined by `v1`, `v2`.
* `vector.angle(v1, v2)`:
    * Returns the angle between `v1` and `v2` in radians.
* `vector.dot(v1, v2)`:
    * Returns the dot product of `v1` and `v2`.
* `vector.cross(v1, v2)`:
    * Returns the cross product of `v1` and `v2`.
* `vector.offset(v, x, y, z)`:
    * Returns the sum of the vectors `v` and `(x, y, z)`.
* `vector.check(v)`:
    * Returns a boolean value indicating whether `v` is a real vector, eg. created
      by a `vector.*` function.
    * Returns `false` for anything else, including tables like `{x=3,y=1,z=4}`.
* `vector.in_area(pos, min, max)`:
    * Returns a boolean value indicating if `pos` is inside area formed by `min` and `max`.
    * `min` and `max` are inclusive.
    * If `min` is bigger than `max` on some axis, function always returns false.
    * You can use `vector.sort` if you have two vectors and don't know which are the minimum and the maximum.
* `vector.random_in_area(min, max)`:
    * Returns a random integer position in area formed by `min` and `max`
    * `min` and `max` are inclusive.
    * You can use `vector.sort` if you have two vectors and don't know which are the minimum and the maximum.

For the following functions `x` can be either a vector or a number:

* `vector.add(v, x)`:
    * Returns a vector.
    * If `x` is a vector: Returns the sum of `v` and `x`.
    * If `x` is a number: Adds `x` to each component of `v`.
* `vector.subtract(v, x)`:
    * Returns a vector.
    * If `x` is a vector: Returns the difference of `v` subtracted by `x`.
    * If `x` is a number: Subtracts `x` from each component of `v`.
* `vector.multiply(v, s)`:
    * Returns a scaled vector.
    * Deprecated: If `s` is a vector: Returns the Schur product.
* `vector.divide(v, s)`:
    * Returns a scaled vector.
    * Deprecated: If `s` is a vector: Returns the Schur quotient.

Operators
---------

Operators can be used if all of the involved vectors have metatables:

* `v1 == v2`:
    * Returns whether `v1` and `v2` are identical.
* `-v`:
    * Returns the additive inverse of v.
* `v1 + v2`:
    * Returns the sum of both vectors.
    * Note: `+` cannot be used together with scalars.
* `v1 - v2`:
    * Returns the difference of `v1` subtracted by `v2`.
    * Note: `-` cannot be used together with scalars.
* `v * s` or `s * v`:
    * Returns `v` scaled by `s`.
* `v / s`:
    * Returns `v` scaled by `1 / s`.

Rotation-related functions
--------------------------

For the following functions `a` is an angle in radians and `r` is a rotation
vector (`{x = <pitch>, y = <yaw>, z = <roll>}`) where pitch, yaw and roll are
angles in radians.

* `vector.rotate(v, r)`:
    * Applies the rotation `r` to `v` and returns the result.
    * `vector.rotate(vector.new(0, 0, 1), r)` and
      `vector.rotate(vector.new(0, 1, 0), r)` return vectors pointing
      forward and up relative to an entity's rotation `r`.
* `vector.rotate_around_axis(v1, v2, a)`:
    * Returns `v1` rotated around axis `v2` by `a` radians according to
      the right hand rule.
* `vector.dir_to_rotation(direction[, up])`:
    * Returns a rotation vector for `direction` pointing forward using `up`
      as the up vector.
    * If `up` is omitted, the roll of the returned vector defaults to zero.
    * Otherwise `direction` and `up` need to be vectors in a 90 degree angle to each other.

Further helpers
---------------

There are more helper functions involving vectors, but they are listed elsewhere
because they only work on specific sorts of vectors or involve things that are not
vectors.

For example:

* `core.hash_node_position` (Only works on node positions.)
* `core.dir_to_wallmounted` (Involves wallmounted param2 values.)
