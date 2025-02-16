Decoration definition
---------------------

See [Decoration types]. Used by `core.register_decoration`.

```lua
{
    deco_type = "simple",
    -- Type. "simple", "schematic" or "lsystem" supported

    place_on = "default:dirt_with_grass",
    -- Node (or list of nodes) that the decoration can be placed on

    sidelen = 8,
    -- Size of the square (X / Z) divisions of the mapchunk being generated.
    -- Determines the resolution of noise variation if used.
    -- If the chunk size is not evenly divisible by sidelen, sidelen is made
    -- equal to the chunk size.

    fill_ratio = 0.02,
    -- The value determines 'decorations per surface node'.
    -- Used only if noise_params is not specified.
    -- If >= 10.0 complete coverage is enabled and decoration placement uses
    -- a different and much faster method.

    noise_params = {
        offset = 0,
        scale = 0.45,
        spread = {x = 100, y = 100, z = 100},
        seed = 354,
        octaves = 3,
        persistence = 0.7,
        lacunarity = 2.0,
        flags = "absvalue"
    },
    -- NoiseParams structure describing the perlin noise used for decoration
    -- distribution.
    -- A noise value is calculated for each square division and determines
    -- 'decorations per surface node' within each division.
    -- If the noise value >= 10.0 complete coverage is enabled and
    -- decoration placement uses a different and much faster method.

    biomes = {"Oceanside", "Hills", "Plains"},
    -- List of biomes in which this decoration occurs. Occurs in all biomes
    -- if this is omitted, and ignored if the Mapgen being used does not
    -- support biomes.
    -- Can be a list of (or a single) biome names, IDs, or definitions.

    y_min = -31000,
    y_max = 31000,
    -- Lower and upper limits for decoration (inclusive).
    -- These parameters refer to the Y coordinate of the 'place_on' node.

    spawn_by = "default:water",
    -- Node (or list of nodes) that the decoration only spawns next to.
    -- Checks the 8 neighboring nodes on the same height,
    -- and also the ones at the height plus the check_offset, excluding both center nodes.

    check_offset = -1,
    -- Specifies the offset that spawn_by should also check
    -- The default value of -1 is useful to e.g check for water next to the base node.
    -- 0 disables additional checks, valid values: {-1, 0, 1}

    num_spawn_by = 1,
    -- Number of spawn_by nodes that must be surrounding the decoration
    -- position to occur.
    -- If absent or -1, decorations occur next to any nodes.

    flags = "liquid_surface, force_placement, all_floors, all_ceilings",
    -- Flags for all decoration types.
    -- "liquid_surface": Find the highest liquid (not solid) surface under
    --   open air. Search stops and fails on the first solid node.
    --   Cannot be used with "all_floors" or "all_ceilings" below.
    -- "force_placement": Nodes other than "air" and "ignore" are replaced
    --   by the decoration.
    -- "all_floors", "all_ceilings": Instead of placement on the highest
    --   surface in a mapchunk the decoration is placed on all floor and/or
    --   ceiling surfaces, for example in caves and dungeons.
    --   Ceiling decorations act as an inversion of floor decorations so the
    --   effect of 'place_offset_y' is inverted.
    --   Y-slice probabilities do not function correctly for ceiling
    --   schematic decorations as the behavior is unchanged.
    --   If a single decoration registration has both flags the floor and
    --   ceiling decorations will be aligned vertically.

    ----- Simple-type parameters

    decoration = "default:grass",
    -- The node name used as the decoration.
    -- If instead a list of strings, a randomly selected node from the list
    -- is placed as the decoration.

    height = 1,
    -- Decoration height in nodes.
    -- If height_max is not 0, this is the lower limit of a randomly
    -- selected height.

    height_max = 0,
    -- Upper limit of the randomly selected height.
    -- If absent, the parameter 'height' is used as a constant.

    param2 = 0,
    -- Param2 value of decoration nodes.
    -- If param2_max is not 0, this is the lower limit of a randomly
    -- selected param2.

    param2_max = 0,
    -- Upper limit of the randomly selected param2.
    -- If absent, the parameter 'param2' is used as a constant.

    place_offset_y = 0,
    -- Y offset of the decoration base node relative to the standard base
    -- node position.
    -- Can be positive or negative. Default is 0.
    -- Effect is inverted for "all_ceilings" decorations.
    -- Ignored by 'y_min', 'y_max' and 'spawn_by' checks, which always refer
    -- to the 'place_on' node.

    ----- Schematic-type parameters

    schematic = "foobar.mts",
    -- If schematic is a string, it is the filepath relative to the current
    -- working directory of the specified Luanti schematic file.
    -- Could also be the ID of a previously registered schematic.

    schematic = {
        size = {x = 4, y = 6, z = 4},
        data = {
            {name = "default:cobble", param1 = 255, param2 = 0},
            {name = "default:dirt_with_grass", param1 = 255, param2 = 0},
            {name = "air", param1 = 255, param2 = 0},
              ...
        },
        yslice_prob = {
            {ypos = 2, prob = 128},
            {ypos = 5, prob = 64},
              ...
        },
    },
    -- Alternative schematic specification by supplying a table. The fields
    -- size and data are mandatory whereas yslice_prob is optional.
    -- See 'Schematic specifier' for details.

    replacements = {["oldname"] = "convert_to", ...},
    -- Map of node names to replace in the schematic after reading it.

    flags = "place_center_x, place_center_y, place_center_z",
    -- Flags for schematic decorations. See 'Schematic attributes'.

    rotation = "90",
    -- Rotation can be "0", "90", "180", "270", or "random"

    place_offset_y = 0,
    -- If the flag 'place_center_y' is set this parameter is ignored.
    -- Y offset of the schematic base node layer relative to the 'place_on'
    -- node.
    -- Can be positive or negative. Default is 0.
    -- Effect is inverted for "all_ceilings" decorations.
    -- Ignored by 'y_min', 'y_max' and 'spawn_by' checks, which always refer
    -- to the 'place_on' node.

    ----- L-system-type parameters

    treedef = {},
    -- Same as for `core.spawn_tree`.
    -- See section [L-system trees] for more details.
}
```
