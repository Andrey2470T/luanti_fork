`PerlinNoise`
-------------

A perlin noise generator.
It can be created via `PerlinNoise()` or `core.get_perlin()`.
For `core.get_perlin()`, the actual seed used is the noiseparams seed
plus the world seed, to create world-specific noise.

`PerlinNoise(noiseparams)`
`PerlinNoise(seed, octaves, persistence, spread)` (Deprecated).

`core.get_perlin(noiseparams)`
`core.get_perlin(seeddiff, octaves, persistence, spread)` (Deprecated).

### Methods

* `get_2d(pos)`: returns 2D noise value at `pos={x=,y=}`
* `get_3d(pos)`: returns 3D noise value at `pos={x=,y=,z=}`

`PerlinNoiseMap`
----------------

A fast, bulk perlin noise generator.

It can be created via `PerlinNoiseMap(noiseparams, size)` or
`core.get_perlin_map(noiseparams, size)`.
For `core.get_perlin_map()`, the actual seed used is the noiseparams seed
plus the world seed, to create world-specific noise.

Format of `size` is `{x=dimx, y=dimy, z=dimz}`. The `z` component is omitted
for 2D noise, and it must be larger than 1 for 3D noise (otherwise
`nil` is returned).

For each of the functions with an optional `buffer` parameter: If `buffer` is
not nil, this table will be used to store the result instead of creating a new
table.

### Methods

* `get_2d_map(pos)`: returns a `<size.x>` times `<size.y>` 2D array of 2D noise
  with values starting at `pos={x=,y=}`
* `get_3d_map(pos)`: returns a `<size.x>` times `<size.y>` times `<size.z>`
  3D array of 3D noise with values starting at `pos={x=,y=,z=}`.
* `get_2d_map_flat(pos, buffer)`: returns a flat `<size.x * size.y>` element
  array of 2D noise with values starting at `pos={x=,y=}`
* `get_3d_map_flat(pos, buffer)`: Same as `get2dMap_flat`, but 3D noise
* `calc_2d_map(pos)`: Calculates the 2d noise map starting at `pos`. The result
  is stored internally.
* `calc_3d_map(pos)`: Calculates the 3d noise map starting at `pos`. The result
  is stored internally.
* `get_map_slice(slice_offset, slice_size, buffer)`: In the form of an array,
  returns a slice of the most recently computed noise results. The result slice
  begins at coordinates `slice_offset` and takes a chunk of `slice_size`.
  E.g. to grab a 2-slice high horizontal 2d plane of noise starting at buffer
  offset y = 20:
  `noisevals = noise:get_map_slice({y=20}, {y=2})`
  It is important to note that `slice_offset` offset coordinates begin at 1,
  and are relative to the starting position of the most recently calculated
  noise.
  To grab a single vertical column of noise starting at map coordinates
  x = 1023, y=1000, z = 1000:
  `noise:calc_3d_map({x=1000, y=1000, z=1000})`
  `noisevals = noise:get_map_slice({x=24, z=1}, {x=1, z=1})`
