Mapgen objects
==============

A mapgen object is a construct used in map generation. Mapgen objects can be
used by an `on_generated` callback to speed up operations by avoiding
unnecessary recalculations, these can be retrieved using the
`core.get_mapgen_object()` function. If the requested Mapgen object is
unavailable, or `get_mapgen_object()` was called outside of an `on_generated`
callback, `nil` is returned.

The following Mapgen objects are currently available:

### `voxelmanip`

This returns three values; the `VoxelManip` object to be used, minimum and
maximum emerged position, in that order. All mapgens support this object.

### `heightmap`

Returns an array containing the y coordinates of the ground levels of nodes in
the most recently generated chunk by the current mapgen.

### `biomemap`

Returns an array containing the biome IDs of nodes in the most recently
generated chunk by the current mapgen.

### `heatmap`

Returns an array containing the temperature values of nodes in the most
recently generated chunk by the current mapgen.

### `humiditymap`

Returns an array containing the humidity values of nodes in the most recently
generated chunk by the current mapgen.

### `gennotify`

Returns a table. You need to announce your interest in a specific
field by calling `core.set_gen_notify()` *before* map generation happens.

* key = string: generation notification type
* value = list of positions (usually)
   * Exceptions are denoted in the listing below.

Available generation notification types:

* `dungeon`: bottom center position of dungeon rooms
* `temple`: as above but for desert temples (mgv6 only)
* `cave_begin`
* `cave_end`
* `large_cave_begin`
* `large_cave_end`
* `custom`: data originating from [Mapgen environment] (Lua API)
   * This is a table.
   * key = user-defined ID (string)
   * value = arbitrary Lua value
* `decoration#id`: decorations
  * (see below)

Decorations have a key in the format of `"decoration#id"`, where `id` is the
numeric unique decoration ID as returned by `core.get_decoration_id()`.
For example, `decoration#123`.

The returned positions are the ground surface 'place_on' nodes,
not the decorations themselves. A 'simple' type decoration is often 1
node above the returned position and possibly displaced by 'place_offset_y'.
