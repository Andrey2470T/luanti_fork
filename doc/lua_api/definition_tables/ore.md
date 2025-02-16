Ore definition
--------------

Used by `core.register_ore`.

See [Ores] section above for essential information.

```lua
{
    ore_type = "",
    -- Supported: "scatter", "sheet", "puff", "blob", "vein", "stratum"

    ore = "",
    -- Ore node to place

    ore_param2 = 0,
    -- Param2 to set for ore (e.g. facedir rotation)

    wherein = "",
    -- Node to place ore in. Multiple are possible by passing a list.

    clust_scarcity = 8 * 8 * 8,
    -- Ore has a 1 out of clust_scarcity chance of spawning in a node.
    -- If the desired average distance between ores is 'd', set this to
    -- d * d * d.

    clust_num_ores = 8,
    -- Number of ores in a cluster

    clust_size = 3,
    -- Size of the bounding box of the cluster.
    -- In this example, there is a 3 * 3 * 3 cluster where 8 out of the 27
    -- nodes are coal ore.

    y_min = -31000,
    y_max = 31000,
    -- Lower and upper limits for ore (inclusive)

    flags = "",
    -- Attributes for the ore generation, see 'Ore attributes' section above

    noise_threshold = 0,
    -- If noise is above this threshold, ore is placed. Not needed for a
    -- uniform distribution.

    noise_params = {
        offset = 0,
        scale = 1,
        spread = {x = 100, y = 100, z = 100},
        seed = 23,
        octaves = 3,
        persistence = 0.7
    },
    -- NoiseParams structure describing one of the perlin noises used for
    -- ore distribution.
    -- Needed by "sheet", "puff", "blob" and "vein" ores.
    -- Omit from "scatter" ore for a uniform ore distribution.
    -- Omit from "stratum" ore for a simple horizontal strata from y_min to
    -- y_max.

    biomes = {"desert", "rainforest"},
    -- List of biomes in which this ore occurs.
    -- Occurs in all biomes if this is omitted, and ignored if the Mapgen
    -- being used does not support biomes.
    -- Can be a list of (or a single) biome names, IDs, or definitions.

    -- Type-specific parameters

    -- "sheet"
    column_height_min = 1,
    column_height_max = 16,
    column_midpoint_factor = 0.5,

    -- "puff"
    np_puff_top = {
        offset = 4,
        scale = 2,
        spread = {x = 100, y = 100, z = 100},
        seed = 47,
        octaves = 3,
        persistence = 0.7
    },
    np_puff_bottom = {
        offset = 4,
        scale = 2,
        spread = {x = 100, y = 100, z = 100},
        seed = 11,
        octaves = 3,
        persistence = 0.7
    },

    -- "vein"
    random_factor = 1.0,

    -- "stratum"
    np_stratum_thickness = {
        offset = 8,
        scale = 4,
        spread = {x = 100, y = 100, z = 100},
        seed = 17,
        octaves = 3,
        persistence = 0.7
    },
    stratum_thickness = 8, -- only used if no noise defined
}
```
