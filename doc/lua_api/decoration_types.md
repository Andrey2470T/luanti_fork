Decoration types
================

The varying types of decorations that can be placed.

`simple`
--------

Creates a 1 times `H` times 1 column of a specified node (or a random node from
a list, if a decoration list is specified). Can specify a certain node it must
spawn next to, such as water or lava, for example. Can also generate a
decoration of random height between a specified lower and upper bound.
This type of decoration is intended for placement of grass, flowers, cacti,
papyri, waterlilies and so on.

`schematic`
-----------

Copies a box of `MapNodes` from a specified schematic file (or raw description).
Can specify a probability of a node randomly appearing when placed.
This decoration type is intended to be used for multi-node sized discrete
structures, such as trees, cave spikes, rocks, and so on.

`lsystem`
-----------

Generates a L-system tree at the position where the decoration is placed.
Uses the same L-system as `core.spawn_tree`, but is faster than using it manually.
The `treedef` field in the decoration definition is used for the tree definition.
