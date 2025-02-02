Lua Voxel Manipulator
=====================

About VoxelManip
----------------

VoxelManip is a scripting interface to the internal 'Map Voxel Manipulator'
facility. The purpose of this object is for fast, low-level, bulk access to
reading and writing Map content. As such, setting map nodes through VoxelManip
will lack many of the higher level features and concepts you may be used to
with other methods of setting nodes. For example, nodes will not have their
construction and destruction callbacks run, and no rollback information is
logged.

It is important to note that VoxelManip is designed for speed, and *not* ease
of use or flexibility. If your mod requires a map manipulation facility that
will handle 100% of all edge cases, or the use of high level node placement
features, perhaps `core.set_node()` is better suited for the job.

In addition, VoxelManip might not be faster, or could even be slower, for your
specific use case. VoxelManip is most effective when setting large areas of map
at once - for example, if only setting a 3x3x3 node area, a
`core.set_node()` loop may be more optimal. Always profile code using both
methods of map manipulation to determine which is most appropriate for your
usage.

A recent simple test of setting cubic areas showed that `core.set_node()`
is faster than a VoxelManip for a 3x3x3 node cube or smaller.

Using VoxelManip
----------------

A VoxelManip object can be created any time using either:
`VoxelManip([p1, p2])`, or `core.get_voxel_manip([p1, p2])`.

If the optional position parameters are present for either of these routines,
the specified region will be pre-loaded into the VoxelManip object on creation.
Otherwise, the area of map you wish to manipulate must first be loaded into the
VoxelManip object using `VoxelManip:read_from_map()`.

Note that `VoxelManip:read_from_map()` returns two position vectors. The region
formed by these positions indicate the minimum and maximum (respectively)
positions of the area actually loaded in the VoxelManip, which may be larger
than the area requested. For convenience, the loaded area coordinates can also
be queried any time after loading map data with `VoxelManip:get_emerged_area()`.

Now that the VoxelManip object is populated with map data, your mod can fetch a
copy of this data using either of two methods. `VoxelManip:get_node_at()`,
which retrieves an individual node in a MapNode formatted table at the position
requested is the simplest method to use, but also the slowest.

Nodes in a VoxelManip object may also be read in bulk to a flat array table
using:

* `VoxelManip:get_data()` for node content (in Content ID form, see section
  [Content IDs]),
* `VoxelManip:get_light_data()` for node light levels, and
* `VoxelManip:get_param2_data()` for the node type-dependent "param2" values.

See section [Flat array format] for more details.

It is very important to understand that the tables returned by any of the above
three functions represent a snapshot of the VoxelManip's internal state at the
time of the call. This copy of the data will not magically update itself if
another function modifies the internal VoxelManip state.
Any functions that modify a VoxelManip's contents work on the VoxelManip's
internal state unless otherwise explicitly stated.

Once the bulk data has been edited to your liking, the internal VoxelManip
state can be set using:

* `VoxelManip:set_data()` for node content (in Content ID form, see section
  [Content IDs]),
* `VoxelManip:set_light_data()` for node light levels, and
* `VoxelManip:set_param2_data()` for the node type-dependent `param2` values.

The parameter to each of the above three functions can use any table at all in
the same flat array format as produced by `get_data()` etc. and is not required
to be a table retrieved from `get_data()`.

Once the internal VoxelManip state has been modified to your liking, the
changes can be committed back to the map by calling `VoxelManip:write_to_map()`

### Flat array format

Let
    `Nx = p2.X - p1.X + 1`,
    `Ny = p2.Y - p1.Y + 1`, and
    `Nz = p2.Z - p1.Z + 1`.

Then, for a loaded region of p1..p2, this array ranges from `1` up to and
including the value of the expression `Nx * Ny * Nz`.

Positions offset from p1 are present in the array with the format of:

    [
        (0, 0, 0),   (1, 0, 0),   (2, 0, 0),   ... (Nx, 0, 0),
        (0, 1, 0),   (1, 1, 0),   (2, 1, 0),   ... (Nx, 1, 0),
        ...
        (0, Ny, 0),  (1, Ny, 0),  (2, Ny, 0),  ... (Nx, Ny, 0),
        (0, 0, 1),   (1, 0, 1),   (2, 0, 1),   ... (Nx, 0, 1),
        ...
        (0, Ny, 2),  (1, Ny, 2),  (2, Ny, 2),  ... (Nx, Ny, 2),
        ...
        (0, Ny, Nz), (1, Ny, Nz), (2, Ny, Nz), ... (Nx, Ny, Nz)
    ]

and the array index for a position p contained completely in p1..p2 is:

`(p.Z - p1.Z) * Ny * Nx + (p.Y - p1.Y) * Nx + (p.X - p1.X) + 1`

Note that this is the same "flat 3D array" format as
`PerlinNoiseMap:get3dMap_flat()`.
VoxelArea objects (see section [`VoxelArea`]) can be used to simplify calculation
of the index for a single point in a flat VoxelManip array.

### Content IDs

A Content ID is a unique integer identifier for a specific node type.
These IDs are used by VoxelManip in place of the node name string for
`VoxelManip:get_data()` and `VoxelManip:set_data()`. You can use
`core.get_content_id()` to look up the Content ID for the specified node
name, and `core.get_name_from_content_id()` to look up the node name string
for a given Content ID.
After registration of a node, its Content ID will remain the same throughout
execution of the mod.
Note that the node being queried needs to have already been been registered.

The following builtin node types have their Content IDs defined as constants:

* `core.CONTENT_UNKNOWN`: ID for "unknown" nodes
* `core.CONTENT_AIR`:     ID for "air" nodes
* `core.CONTENT_IGNORE`:  ID for "ignore" nodes

### Mapgen VoxelManip objects

Inside of `on_generated()` callbacks, it is possible to retrieve the same
VoxelManip object used by the core's Map Generator (commonly abbreviated
Mapgen). Most of the rules previously described still apply but with a few
differences:

* The Mapgen VoxelManip object is retrieved using:
  `core.get_mapgen_object("voxelmanip")`

* This VoxelManip object already has the region of map just generated loaded
  into it; it's not necessary to call `VoxelManip:read_from_map()`.
  Note that the region of map it has loaded is NOT THE SAME as the `minp`, `maxp`
  parameters of `on_generated()`. Refer to `core.get_mapgen_object` docs.
  Once you're done you still need to call `VoxelManip:write_to_map()`

* The `on_generated()` callbacks of some mods may place individual nodes in the
  generated area using non-VoxelManip map modification methods. Because the
  same Mapgen VoxelManip object is passed through each `on_generated()`
  callback, it becomes necessary for the Mapgen VoxelManip object to maintain
  consistency with the current map state. For this reason, calling any of
  `core.add_node()`, `core.set_node()` or `core.swap_node()`
  will also update the Mapgen VoxelManip object's internal state active on the
  current thread.

* After modifying the Mapgen VoxelManip object's internal buffer, it may be
  necessary to update lighting information using either:
  `VoxelManip:calc_lighting()` or `VoxelManip:set_lighting()`.

### Other API functions operating on a VoxelManip

If any VoxelManip contents were set to a liquid node (`liquidtype ~= "none"`),
`VoxelManip:update_liquids()` must be called for these liquid nodes to begin
flowing. It is recommended to call this function only after having written all
buffered data back to the VoxelManip object, save for special situations where
the modder desires to only have certain liquid nodes begin flowing.

The functions `core.generate_ores()` and `core.generate_decorations()`
will generate all registered decorations and ores throughout the full area
inside of the specified VoxelManip object.

`core.place_schematic_on_vmanip()` is otherwise identical to
`core.place_schematic()`, except instead of placing the specified schematic
directly on the map at the specified position, it will place the schematic
inside the VoxelManip.

### Notes

* Attempting to read data from a VoxelManip object before map is read will
  result in a zero-length array table for `VoxelManip:get_data()`, and an
  "ignore" node at any position for `VoxelManip:get_node_at()`.

* If you attempt to use a VoxelManip to read a region of the map that has
  already been generated, but is not currently loaded, that region will be
  loaded from disk. This means that reading a region of the map with a
  VoxelManip has a similar effect as calling `core.load_area` on that
  region.

* If a region of the map has either not yet been generated or is outside the
  map boundaries, it is filled with "ignore" nodes. Writing to regions of the
  map that are not yet generated may result in unexpected behavior. You
  can use `core.emerge_area` to make sure that the area you want to
  read/write is already generated.

* Other mods, or the engine itself, could possibly modify the area of the map
  currently loaded into a VoxelManip object. With the exception of Mapgen
  VoxelManips (see above section), the internal buffers are not updated. For
  this reason, it is strongly encouraged to complete the usage of a particular
  VoxelManip object in the same callback it had been created.

* If a VoxelManip object will be used often, such as in an `on_generated()`
  callback, consider passing a file-scoped table as the optional parameter to
  `VoxelManip:get_data()`, which serves as a static buffer the function can use
  to write map data to instead of returning a new table each call. This greatly
  enhances performance by avoiding unnecessary memory allocations.

Methods
-------

* `read_from_map(p1, p2)`: Loads a chunk of map into the VoxelManip object
  containing the region formed by `p1` and `p2`.
    * returns actual emerged `pmin`, actual emerged `pmax`
    * Note that calling this multiple times will *add* to the area loaded in the
      VoxelManip, and not reset it.
* `write_to_map([light])`: Writes the data loaded from the `VoxelManip` back to
  the map.
    * **important**: data must be set using `VoxelManip:set_data()` before
      calling this.
    * if `light` is true, then lighting is automatically recalculated.
      The default value is true.
      If `light` is false, no light calculations happen, and you should correct
      all modified blocks with `core.fix_light()` as soon as possible.
      Keep in mind that modifying the map where light is incorrect can cause
      more lighting bugs.
* `get_node_at(pos)`: Returns a `MapNode` table of the node currently loaded in
  the `VoxelManip` at that position
* `set_node_at(pos, node)`: Sets a specific `MapNode` in the `VoxelManip` at
  that position.
* `get_data([buffer])`: Retrieves the node content data loaded into the
  `VoxelManip` object.
    * returns raw node data in the form of an array of node content IDs
    * if the param `buffer` is present, this table will be used to store the
      result instead.
* `set_data(data)`: Sets the data contents of the `VoxelManip` object
* `update_map()`: Does nothing, kept for compatibility.
* `set_lighting(light, [p1, p2])`: Set the lighting within the `VoxelManip` to
  a uniform value.
    * `light` is a table, `{day=<0...15>, night=<0...15>}`
    * To be used only by a `VoxelManip` object from
      `core.get_mapgen_object`.
    * (`p1`, `p2`) is the area in which lighting is set, defaults to the whole
      area if left out.
* `get_light_data([buffer])`: Gets the light data read into the
  `VoxelManip` object
    * Returns an array (indices 1 to volume) of integers ranging from `0` to
      `255`.
    * Each value is the bitwise combination of day and night light values
      (`0` to `15` each).
    * `light = day + (night * 16)`
    * If the param `buffer` is present, this table will be used to store the
      result instead.
* `set_light_data(light_data)`: Sets the `param1` (light) contents of each node
  in the `VoxelManip`.
    * expects lighting data in the same format that `get_light_data()` returns
* `get_param2_data([buffer])`: Gets the raw `param2` data read into the
  `VoxelManip` object.
    * Returns an array (indices 1 to volume) of integers ranging from `0` to
      `255`.
    * If the param `buffer` is present, this table will be used to store the
      result instead.
* `set_param2_data(param2_data)`: Sets the `param2` contents of each node in
  the `VoxelManip`.
* `calc_lighting([p1, p2], [propagate_shadow])`:  Calculate lighting within the
  `VoxelManip`.
    * To be used only with a `VoxelManip` object from `core.get_mapgen_object`.
    * (`p1`, `p2`) is the area in which lighting is set, defaults to the whole
      area if left out or nil. For almost all uses these should be left out
      or nil to use the default.
    * `propagate_shadow` is an optional boolean deciding whether shadows in a
      generated mapchunk above are propagated down into the mapchunk, defaults
      to `true` if left out.
* `update_liquids()`: Update liquid flow
* `was_modified()`: Returns `true` if the data in the VoxelManip has been modified
   since it was last read from the map. This means you have to call `get_data()` again.
   This only applies to a `VoxelManip` object from `core.get_mapgen_object`,
   where the engine will keep the map and the VM in sync automatically.
   * Note: this doesn't do what you think it does and is subject to removal. Don't use it!
* `get_emerged_area()`: Returns actual emerged minimum and maximum positions.

`VoxelArea`
-----------

A helper class for voxel areas.
It can be created via `VoxelArea(pmin, pmax)` or
`VoxelArea:new({MinEdge = pmin, MaxEdge = pmax})`.
The coordinates are *inclusive*, like most other things in Luanti.

### Methods

* `getExtent()`: returns a 3D vector containing the size of the area formed by
  `MinEdge` and `MaxEdge`.
* `getVolume()`: returns the volume of the area formed by `MinEdge` and
  `MaxEdge`.
* `index(x, y, z)`: returns the index of an absolute position in a flat array
  starting at `1`.
    * `x`, `y` and `z` must be integers to avoid an incorrect index result.
    * The position (x, y, z) is not checked for being inside the area volume,
      being outside can cause an incorrect index result.
    * Useful for things like `VoxelManip`, raw Schematic specifiers,
      `PerlinNoiseMap:get2d`/`3dMap`, and so on.
* `indexp(p)`: same functionality as `index(x, y, z)` but takes a vector.
    * As with `index(x, y, z)`, the components of `p` must be integers, and `p`
      is not checked for being inside the area volume.
* `position(i)`: returns the absolute position vector corresponding to index
  `i`.
* `contains(x, y, z)`: check if (`x`,`y`,`z`) is inside area formed by
  `MinEdge` and `MaxEdge`.
* `containsp(p)`: same as above, except takes a vector
* `containsi(i)`: same as above, except takes an index `i`
* `iter(minx, miny, minz, maxx, maxy, maxz)`: returns an iterator that returns
  indices.
    * from (`minx`,`miny`,`minz`) to (`maxx`,`maxy`,`maxz`) in the order of
      `[z [y [x]]]`.
* `iterp(minp, maxp)`: same as above, except takes a vector

### Y stride and z stride of a flat array

For a particular position in a voxel area, whose flat array index is known,
it is often useful to know the index of a neighboring or nearby position.
The table below shows the changes of index required for 1 node movements along
the axes in a voxel area:

    Movement    Change of index
    +x          +1
    -x          -1
    +y          +ystride
    -y          -ystride
    +z          +zstride
    -z          -zstride

If, for example:

    local area = VoxelArea(emin, emax)

The values of `ystride` and `zstride` can be obtained using `area.ystride` and
`area.zstride`.
