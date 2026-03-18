Schematics
==========

Schematic specifier
--------------------

A schematic specifier identifies a schematic by either a filename to a
Luanti Schematic file (`.mts`) or through raw data supplied through Lua,
in the form of a table.  This table specifies the following fields:

* The `size` field is a 3D vector containing the dimensions of the provided
  schematic. (required field)
* The `yslice_prob` field is a table of {ypos, prob} slice tables. A slice table
  sets the probability of a particular horizontal slice of the schematic being
  placed. (optional field)
  `ypos` = 0 for the lowest horizontal slice of a schematic.
  The default of `prob` is 255.
* The `data` field is a flat table of MapNode tables making up the schematic,
  in the order of `[z [y [x]]]`. (required field)
  Each MapNode table contains:
    * `name`: the name of the map node to place (required)
    * `prob` (alias `param1`): the probability of this node being placed
      (default: 255)
    * `param2`: the raw param2 value of the node being placed onto the map
      (default: 0)
    * `force_place`: boolean representing if the node should forcibly overwrite
      any previous contents (default: false)

About probability values:

* A probability value of `0` or `1` means that node will never appear
  (0% chance).
* A probability value of `254` or `255` means the node will always appear
  (100% chance).
* If the probability value `p` is greater than `1`, then there is a
  `(p / 256 * 100)` percent chance that node will appear when the schematic is
  placed on the map.

Schematic attributes
--------------------

See section [Flag Specifier Format].

Currently supported flags: `place_center_x`, `place_center_y`, `place_center_z`,
                           `force_placement`.

* `place_center_x`: Placement of this decoration is centered along the X axis.
* `place_center_y`: Placement of this decoration is centered along the Y axis.
* `place_center_z`: Placement of this decoration is centered along the Z axis.
* `force_placement`: Schematic nodes other than "ignore" will replace existing
  nodes.
