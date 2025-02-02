Mapgen environment
==================

The engine runs the map generator on separate threads, each of these also has
a Lua environment. Its primary purpose is to allow mods to operate on newly
generated parts of the map to e.g. generate custom structures.
Internally it is referred to as "emerge environment".

Refer to [Async environment] for the usual disclaimer on what environment isolation entails.

The map generator threads, which also contain the above mentioned Lua environment,
are initialized after all mods have been loaded by the server. After that the
registered scripts (not all mods!) - see below - are run during initialization of
the mapgen environment. After that only callbacks happen. The mapgen env
does not have a global step or timer.

* `core.register_mapgen_script(path)`:
    * Register a path to a Lua file to be imported when a mapgen environment
      is initialized. Run in order of registration.

### List of APIs exclusive to the mapgen env

* `core.register_on_generated(function(vmanip, minp, maxp, blockseed))`
    * Called after the engine mapgen finishes a chunk but before it is written to
      the map.
    * Chunk data resides in `vmanip`. Other parts of the map are not accessible.
      The area of the chunk if comprised of `minp` and `maxp`, note that is smaller
      than the emerged area of the VoxelManip.
      Note: calling `read_from_map()` or `write_to_map()` on the VoxelManipulator object
      is not necessary and is disallowed.
    * `blockseed`: 64-bit seed number used for this chunk
* `core.save_gen_notify(id, data)`
    * Saves data for retrieval using the gennotify mechanism (see [Mapgen objects]).
    * Data is bound to the chunk that is currently being processed, so this function
      only makes sense inside the `on_generated` callback.
    * `id`: user-defined ID (a string)
      By convention these should be the mod name with an optional
      colon and specifier added, e.g. `"default"` or `"default:dungeon_loot"`
    * `data`: any Lua object (will be serialized, no userdata allowed)
    * returns `true` if the data was remembered. That is if `core.set_gen_notify`
      was called with the same user-defined ID before.

### List of APIs available in the mapgen env

Classes:

* `AreaStore`
* `ItemStack`
* `PerlinNoise`
* `PerlinNoiseMap`
* `PseudoRandom`
* `PcgRandom`
* `SecureRandom`
* `VoxelArea`
* `VoxelManip`
    * only given by callbacks; cannot access rest of map
* `Settings`

Functions:

* Standalone helpers such as logging, filesystem, encoding,
  hashing or compression APIs
* `core.get_biome_id`, `get_biome_name`, `get_heat`, `get_humidity`,
  `get_biome_data`, `get_mapgen_object`, `get_mapgen_params`, `get_mapgen_edges`,
  `get_mapgen_setting`, `get_noiseparams`, `get_decoration_id` and more
* `core.get_node`, `set_node`, `find_node_near`, `find_nodes_in_area`,
  `spawn_tree` and similar
    * these only operate on the current chunk (if inside a callback)
* IPC

Variables:

* `core.settings`
* `core.registered_items`, `registered_nodes`, `registered_tools`,
  `registered_craftitems` and `registered_aliases`
    * with all functions and userdata values replaced by `true`, calling any
      callbacks here is obviously not possible
* `core.registered_biomes`, `registered_ores`, `registered_decorations`

Note that node metadata does not exist in the mapgen env, we suggest deferring
setting any metadata you need to the `on_generated` callback in the regular env.
You can use the gennotify mechanism to transfer this information.
