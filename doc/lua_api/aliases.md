Aliases
=======

Aliases of itemnames can be added by using
`core.register_alias(alias, original_name)` or
`core.register_alias_force(alias, original_name)`.

This adds an alias `alias` for the item called `original_name`.
From now on, you can use `alias` to refer to the item `original_name`.

The only difference between `core.register_alias` and
`core.register_alias_force` is that if an item named `alias` already exists,
`core.register_alias` will do nothing while
`core.register_alias_force` will unregister it.

This can be used for maintaining backwards compatibility.

This can also set quick access names for things, e.g. if
you have an item called `epiclylongmodname:stuff`, you could do

    core.register_alias("stuff", "epiclylongmodname:stuff")

and be able to use `/giveme stuff`.

Mapgen aliases
--------------

In a game, a certain number of these must be set to tell core mapgens which
of the game's nodes are to be used for core mapgen generation. For example:

    core.register_alias("mapgen_stone", "default:stone")

### Aliases for non-V6 mapgens

#### Essential aliases

* `mapgen_stone`
* `mapgen_water_source`
* `mapgen_river_water_source`

`mapgen_river_water_source` is required for mapgens with sloping rivers where
it is necessary to have a river liquid node with a short `liquid_range` and
`liquid_renewable = false` to avoid flooding.

#### Optional aliases

* `mapgen_lava_source`

Fallback lava node used if cave liquids are not defined in biome definitions.
Deprecated, define cave liquids in biome definitions instead.

* `mapgen_cobble`

Fallback node used if dungeon nodes are not defined in biome definitions.
Deprecated, define dungeon nodes in biome definitions instead.

### Aliases for Mapgen V6

#### Essential

* `mapgen_stone`
* `mapgen_water_source`
* `mapgen_lava_source`
* `mapgen_dirt`
* `mapgen_dirt_with_grass`
* `mapgen_sand`

* `mapgen_tree`
* `mapgen_leaves`
* `mapgen_apple`

* `mapgen_cobble`

#### Optional

* `mapgen_gravel` (falls back to stone)
* `mapgen_desert_stone` (falls back to stone)
* `mapgen_desert_sand` (falls back to sand)
* `mapgen_dirt_with_snow` (falls back to dirt_with_grass)
* `mapgen_snowblock` (falls back to dirt_with_grass)
* `mapgen_snow` (not placed if missing)
* `mapgen_ice` (falls back to water_source)

* `mapgen_jungletree` (falls back to tree)
* `mapgen_jungleleaves` (falls back to leaves)
* `mapgen_junglegrass` (not placed if missing)
* `mapgen_pine_tree` (falls back to tree)
* `mapgen_pine_needles` (falls back to leaves)

* `mapgen_stair_cobble` (falls back to cobble)
* `mapgen_mossycobble` (falls back to cobble)
* `mapgen_stair_desert_stone` (falls back to desert_stone)

### Setting the node used in Mapgen Singlenode

By default the world is filled with air nodes. To set a different node use e.g.:

    core.register_alias("mapgen_singlenode", "default:stone")
