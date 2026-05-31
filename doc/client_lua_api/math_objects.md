Math objects
================================

### Position/vector

```lua
{x=num, y=num, z=num}
```

For helper functions see "Vector helpers".

### pointed_thing
* `{type="nothing"}`
* `{type="node", under=pos, above=pos}`
* `{type="object", id=ObjectID}`


### Rotations

Luanti provides a proper helper class for working with 3d rotations.
Using vectors of Euler angles instead is discouraged as it is error-prone.
This class was added in Luanti 5.17.0.

The precision of the implementation may change (improve) in the future.
Rotations currently use 32-bit floats (*less* precision than the Lua number type).

Rotations use **left-handed** conventions.

In the current implementation, Rotations are just an abstraction over quaternions.

Constructors
------------

* `Rotation.identity()`: Constructs a no-op rotation.
* `Rotation.quaternion(x, y, z, w)`:
  Constructs a rotation from a quaternion (which need not be normalized).
* `Rotation.axis_angle(axis, angle)`:
  Constructs a rotation around the given axis by the given angle.
  * `axis` is a nonzero vector, which need not be normalized
  * `angle` is in radians
  * Example: `Rotation.axis_angle(vector.new(1, 0, 1), math.pi/2)`
    is a half-turn around the bisector between the X and Z axes.
  * There are shorthands for rotations around the cardinal axes:
    * `Rotation.x(pitch)`
    * `Rotation.y(yaw)`
    * `Rotation.z(roll)`
* `Rotation.euler_xyz(pitch, yaw, roll)`
  * All angles in radians.
  * Uses X-Y-Z rotation order, equivalent to
    `Rotation.compose(Rotation.z(roll), Rotation.y(yaw), Rotation.x(pitch))`.
  * Consistent with the Euler angles that can be used for bones or attachments.
* `Rotation.euler_zxy(pitch, yaw, roll)`
  * Same as `euler_xyz`, but uses Z-X-Y rotation order.
  * This is consistent with the Euler angles that can be used for entities.
    You can do `Rotation.euler_zxy((-rotation):unpack())`
    to convert an entity rotation vector (note the handedness conversion).
* `Rotation.mapsto(dir_from, dir_to)`:
  Construct a rotation that maps `dir_from` to `dir_to`.
  * `dir_from` and `dir_to` are nonzero direction vectors.
  * The given rotation only rotates in the plane spanned by the two vectors. It is thus uniquely defined.
* `Rotation.compose(...)`: Returns the composition of the given rotations.
  * `Rotation.compose()` is an alias for `Rotation.identity()`.
  * `Rotation.compose(rot)` copies the rotation.
  * `Rotation.compose(...)` for at least two rotations composes the given rotations
    in right-to-left order. This means that `Rotation.compose(second, first):apply(v)`
    is equivalent to `second:apply(first:apply(v))`:
    The composed rotation first applies `first`, then `second` to a vector.

Conversions
-----------

Corresponding to the constructors, rotations can be converted
to different representations.
Note that this conversion is not guaranteed to produce the same values you put in.
It is only guaranteed that the values produce an approximately equivalent rotation
when passed to the corresponding constructor.

* `x, y, z, w = rot:to_quaternion()`
  * Returns the normalized quaternion representation.
* `axis, angle = rot:to_axis_angle()`
  * `axis` is a normalized vector that can point in any direction.
  * `angle` is the rotation about this axis as an angle in radians.
* `pitch, yaw, roll = rot:to_euler_xyz()`
  * Angles are all in radians.
  * `pitch`, `yaw`, `roll`: Rotation around the X-, Y-, and Z-axis respectively.
  * Inverse of `Rotation.euler_xyz`.
* `pitch, yaw, roll = rot:to_euler_zxy()`
  * Same as `to_euler_xyz`, except uses Z-X-Y rotation order.
  * To obtain a rotation for an entity, you can do
    `-vector.new(rot:to_euler_xyz())`.

Rotations can also be converted to matrices using `Matrix4.rotation(rot)`.

Methods
-------

* `rot:compose(...)`: Shorthand for `Rotation.compose(rot, ...)`.
* `rot:apply(vec)`: Returns the result of applying the rotation to the given vector.
* `rot:invert()`: Returns the inverse rotation.
* `from:slerp(to, time)`: Interpolate from one rotation to another.
  * `time = 0` is all `from`, `time = 1` is all `to`.
* `rot:angle_to(other)`: Returns the absolute angle between two quaternions.
  * Useful to measure similarity.

Rotations implement `__tostring`. The format is only intended for human-readability,
not serialization, and may thus change in the future.


### Matrices

Luanti uses 4x4 matrices to represent linear transformations of 3D vectors.
For this, 3D vectors are embedded into 4D space.
This class was added in Luanti 5.17.0.

The matrices use column-major conventions.
Vectors are treated as column vectors.
This means the first column is the image of the vector (1, 0, 0, 0),
the second column is the image of (0, 1, 0, 0), and so on.
Thus the translation is in the last column.

You must account for loss of precision in matrix calculations.
Matrices currently use 32-bit floats (*less* precision than the Lua number type).
However, your code should not expect imprecisions either.
Matrices may carry out computations more precisely in the future.

You should not rely on the internal representation or type of matrices.
You should only interact with matrices through the interface documented below.
This allows us to replace the implementation in the future.

Matrices are very suitable for constructing, composing and applying
linear transformations; they are not useful for exact storage of
TRS transformations if the properties need to be handled separately:
Decomposition into rotation and scale will be expensive and inexact.
You should instead store the translation, rotation and scale.

Constructors
------------

* `Matrix4.new(r1c1, r1c2, ..., r4c4)`:
  Constructs a matrix from the given 16 numbers in row-major order.
* `Matrix4.identity()`: Constructs an identity matrix.
* `Matrix4.full(number)`: Constructs a matrix where all entries are the given number.
* `Matrix4.translation(vec)`: Constructs a matrix that translates vectors by the given vector.
* `Matrix4.rotation(rot)`: Constructs a matrix that applies the given `Rotation` to vectors.
* `Matrix4.scale(s)`: Constructs a matrix that applies the given
  component-wise scaling factors to vectors.
  `s` can be a vector or a number.
* `Matrix4.trs([t], [r], [s])`: Shorthand for
  `Matrix4.compose(Matrix4.translation(t), Matrix4.rotation(r), Matrix4.scale(s))`.
  All parameters are optional and default to identity transforms.
* `Matrix4.reflection(normal)`: Constructs a matrix that reflects vectors
  at the plane with the given plane normal vector (which need not be normalized).
* `Matrix4.compose(...)`: Variadic composition of the given matrices.
  As is common in mathematics, matrices are applied in left-to-right order.
* `Matrix4.compose(...)`: Returns the composition of the given matrices.
  * `Matrix4.compose()` is an alias for `Matrix4.identity()`.
  * `Matrix4.compose(mat)` copies the matrix.
  * `Matrix4.compose(...)` for at least two rotations composes the given matrices
    in right-to-left order. This means that `Matrix4.compose(second, first):apply(v)`
    is equivalent to `second:apply(first:apply(v))`:
    The composed matrix first applies `first`, then `second` to a vector.

Container utilities:

For all of the below methods, `row` and `col`umn indices range from `1` to `4`.

* `mat:get(row, col)`: Returns the number in the given row and column.
* `mat:set(row, col, number)`: Set the entry in the given row and column to the given number.
* `x, y, z, w = mat:get_row(row)`: Get the 4 numbers in the given row.
* `mat:set_row(row, x, y, z, w)`: Set the 4 numbers in the given row.
* `x, y, z, w = mat:get_col(col)`: Get the 4 numbers in the given column.
* `mat:set_col(col, x, y, z, w)`: Set the 4 numbers in the given column.
* `mat:copy()`: Returns a new matrix containing the same numbers.
* `r1c1, r1c2, ..., r4c4 = mat:unpack()`:
  Get the 16 numbers in the matrix in row-major order.
  `Matrix4.new(mat:unpack())` is thus equivalent to `mat:copy()`.

Linear algebra:

* Vector transformations:
  * `x, y, z, w = mat:transform_4d(x, y, z, w)`: Apply the matrix to a 4d vector.
  * `mat:transform_pos(pos)`:
    * Apply the matrix to a vector representing a position.
    * Applies the transformation as if w = 1 and discards the resulting w component.
      * `mat:transform_dir(dir)`:
    * Apply the matrix to a vector representing a direction.
    * Ignores the fourth row and column; does not apply the translation (w = 0).
* `mat:compose(...)`: Shorthand for `Matrix4.compose(mat, ...)`.
* `mat:determinant()`: Returns the determinant.
* `mat:invert()`: Returns a newly created inverse, or `nil` if the matrix is (close to being) singular.
* `mat:transpose()`: Returns a transposed copy of the matrix.
* `mat:equals(other, [tolerance = 0])`:
  Returns whether all components differ in absolute value at most by the given tolerance.
  * `m1 == m2`: Returns whether `m1` and `m2` are identical (`tolerance = 0`).
* `mat:is_affine_transform([tolerance = 0])`:
  Whether the matrix is an affine transformation in 3d space,
  meaning it is a 3d linear transformation plus a translation.
  (This is the case if the last row is approximately 0, 0, 0, 1.)

For working with affine transforms, the following methods are available:

* `mat:get_translation()`: Returns the translation as a vector.
* `mat:set_translation(vec)`: Sets (overwrites) the translation in the last row.

For TRS transforms specifically,
let `mat = Matrix4.compose(Matrix4.translation(t), Matrix4.rotation(r), Matrix4.scale(s))`.
Then we can decompose `mat` further. Note that `mat` must not shear or reflect.

* `rotation, scale = mat:get_rs()`:
  Extracts a `Rotation` equivalent to `r`,
  along with the corresponding component-wise scaling factors as a vector.

Operators
---------

Similar to vectors, matrices define some element-wise arithmetic operators:

* `m1 + m2`: Returns the sum of both matrices.
* `m1 - m2`: Shorthand for `m1 + (-m2)`.
* `-m`: Returns the additive inverse.
* `m * s` or `s * m`: Returns the matrix `m` scaled by the scalar `s`.
  * Note: *All* entries are scaled, including the last column:
    The matrix may not be an affine transform afterwards.

Matrices also define a `__tostring` metamethod.
This is only intended for human readability and not for serialization.
