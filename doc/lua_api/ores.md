Ores
====

Ore types
---------

These tell in what manner the ore is generated.

All default ores are of the uniformly-distributed scatter type.

### `scatter`

Randomly chooses a location and generates a cluster of ore.

If `noise_params` is specified, the ore will be placed if the 3D perlin noise
at that point is greater than the `noise_threshold`, giving the ability to
create a non-equal distribution of ore.

### `sheet`

Creates a sheet of ore in a blob shape according to the 2D perlin noise
described by `noise_params` and `noise_threshold`. This is essentially an
improved version of the so-called "stratus" ore seen in some unofficial mods.

This sheet consists of vertical columns of uniform randomly distributed height,
varying between the inclusive range `column_height_min` and `column_height_max`.
If `column_height_min` is not specified, this parameter defaults to 1.
If `column_height_max` is not specified, this parameter defaults to `clust_size`
for reverse compatibility. New code should prefer `column_height_max`.

The `column_midpoint_factor` parameter controls the position of the column at
which ore emanates from.
If 1, columns grow upward. If 0, columns grow downward. If 0.5, columns grow
equally starting from each direction.
`column_midpoint_factor` is a decimal number ranging in value from 0 to 1. If
this parameter is not specified, the default is 0.5.

The ore parameters `clust_scarcity` and `clust_num_ores` are ignored for this
ore type.

### `puff`

Creates a sheet of ore in a cloud-like puff shape.

As with the `sheet` ore type, the size and shape of puffs are described by
`noise_params` and `noise_threshold` and are placed at random vertical
positions within the currently generated chunk.

The vertical top and bottom displacement of each puff are determined by the
noise parameters `np_puff_top` and `np_puff_bottom`, respectively.

### `blob`

Creates a deformed sphere of ore according to 3d perlin noise described by
`noise_params`. The maximum size of the blob is `clust_size`, and
`clust_scarcity` has the same meaning as with the `scatter` type.

### `vein`

Creates veins of ore varying in density by according to the intersection of two
instances of 3d perlin noise with different seeds, both described by
`noise_params`.

`random_factor` varies the influence random chance has on placement of an ore
inside the vein, which is `1` by default. Note that modifying this parameter
may require adjusting `noise_threshold`.

The parameters `clust_scarcity`, `clust_num_ores`, and `clust_size` are ignored
by this ore type.

This ore type is difficult to control since it is sensitive to small changes.
The following is a decent set of parameters to work from:

```lua
noise_params = {
    offset  = 0,
    scale   = 3,
    spread  = {x=200, y=200, z=200},
    seed    = 5390,
    octaves = 4,
    persistence = 0.5,
    lacunarity = 2.0,
    flags = "eased",
},
noise_threshold = 1.6
```

**WARNING**: Use this ore type *very* sparingly since it is ~200x more
computationally expensive than any other ore.

### `stratum`

Creates a single undulating ore stratum that is continuous across mapchunk
borders and horizontally spans the world.

The 2D perlin noise described by `noise_params` defines the Y coordinate of
the stratum midpoint. The 2D perlin noise described by `np_stratum_thickness`
defines the stratum's vertical thickness (in units of nodes). Due to being
continuous across mapchunk borders the stratum's vertical thickness is
unlimited.

If the noise parameter `noise_params` is omitted the ore will occur from y_min
to y_max in a simple horizontal stratum.

A parameter `stratum_thickness` can be provided instead of the noise parameter
`np_stratum_thickness`, to create a constant thickness.

Leaving out one or both noise parameters makes the ore generation less
intensive, useful when adding multiple strata.

`y_min` and `y_max` define the limits of the ore generation and for performance
reasons should be set as close together as possible but without clipping the
stratum's Y variation.

Each node in the stratum has a 1-in-`clust_scarcity` chance of being ore, so a
solid-ore stratum would require a `clust_scarcity` of 1.

The parameters `clust_num_ores`, `clust_size`, `noise_threshold` and
`random_factor` are ignored by this ore type.

Ore attributes
--------------

See section [Flag Specifier Format].

Currently supported flags:
`puff_cliffs`, `puff_additive_composition`.

### `puff_cliffs`

If set, puff ore generation will not taper down large differences in
displacement when approaching the edge of a puff. This flag has no effect for
ore types other than `puff`.

### `puff_additive_composition`

By default, when noise described by `np_puff_top` or `np_puff_bottom` results
in a negative displacement, the sub-column at that point is not generated. With
this attribute set, puff ore generation will instead generate the absolute
difference in noise displacement values. This flag has no effect for ore types
other than `puff`.
