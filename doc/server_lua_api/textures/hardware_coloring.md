Hardware coloring
=================

The goal of hardware coloring is to simplify the creation of
colorful nodes. If your textures use the same pattern, and they only
differ in their color (like colored wool blocks), you can use hardware
coloring instead of creating and managing many texture files.
All of these methods use color multiplication (so a white-black texture
with red coloring will result in red-black color).

### Static coloring

This method is useful if you wish to create nodes/items with
the same texture, in different colors, each in a new node/item definition.

#### Global color

When you register an item or node, set its `color` field (which accepts a
`ColorSpec`) to the desired color.

An `ItemStack`'s static color can be overwritten by the `color` metadata
field. If you set that field to a `ColorString`, that color will be used.

#### Tile color

Each tile may have an individual static color, which overwrites every
other coloring method. To disable the coloring of a face,
set its color to white (because multiplying with white does nothing).
You can set the `color` property of the tiles in the node's definition
if the tile is in table format.

### Palettes

For nodes and items which can have many colors, a palette is more
suitable. A palette is a texture, which can contain up to 256 pixels.
Each pixel is one possible color for the node/item.
You can register one node/item, which can have up to 256 colors.

#### Palette indexing

When using palettes, you always provide a pixel index for the given
node or `ItemStack`. The palette is read from left to right and from
top to bottom. If the palette has less than 256 pixels, then it is
stretched to contain exactly 256 pixels (after arranging the pixels
to one line). The indexing starts from 0.

Examples:

* 16x16 palette, index = 0: the top left corner
* 16x16 palette, index = 4: the fifth pixel in the first row
* 16x16 palette, index = 16: the pixel below the top left corner
* 16x16 palette, index = 255: the bottom right corner
* 2 (width) x 4 (height) palette, index = 31: the top left corner.
  The palette has 8 pixels, so each pixel is stretched to 32 pixels,
  to ensure the total 256 pixels.
* 2x4 palette, index = 32: the top right corner
* 2x4 palette, index = 63: the top right corner
* 2x4 palette, index = 64: the pixel below the top left corner

#### Using palettes with items

When registering an item, set the item definition's `palette` field to
a texture. You can also use texture modifiers.

The `ItemStack`'s color depends on the `palette_index` field of the
stack's metadata. `palette_index` is an integer, which specifies the
index of the pixel to use.

#### Linking palettes with nodes

When registering a node, set the item definition's `palette` field to
a texture. You can also use texture modifiers.
The node's color depends on its `param2`, so you also must set an
appropriate `paramtype2`:

* `paramtype2 = "color"` for nodes which use their full `param2` for
  palette indexing. These nodes can have 256 different colors.
  The palette should contain 256 pixels.
* `paramtype2 = "colorwallmounted"` for nodes which use the first
  five bits (most significant) of `param2` for palette indexing.
  The remaining three bits are describing rotation, as in `wallmounted`
  paramtype2. Division by 8 yields the palette index (without stretching the
  palette). These nodes can have 32 different colors, and the palette
  should contain 32 pixels.
  Examples:
    * `param2 = 17` is 2 * 8 + 1, so the rotation is 1 and the third (= 2 + 1)
      pixel will be picked from the palette.
    * `param2 = 35` is 4 * 8 + 3, so the rotation is 3 and the fifth (= 4 + 1)
      pixel will be picked from the palette.
* `paramtype2 = "colorfacedir"` for nodes which use the first
  three bits of `param2` for palette indexing. The remaining
  five bits are describing rotation, as in `facedir` paramtype2.
  Division by 32 yields the palette index (without stretching the
  palette). These nodes can have 8 different colors, and the
  palette should contain 8 pixels.
  Examples:
    * `param2 = 17` is 0 * 32 + 17, so the rotation is 17 and the
      first (= 0 + 1) pixel will be picked from the palette.
    * `param2 = 35` is 1 * 32 + 3, so the rotation is 3 and the
      second (= 1 + 1) pixel will be picked from the palette.
* `paramtype2 = "color4dir"` for nodes which use the first
  six bits of `param2` for palette indexing. The remaining
  two bits are describing rotation, as in `4dir` paramtype2.
  Division by 4 yields the palette index (without stretching the
  palette). These nodes can have 64 different colors, and the
  palette should contain 64 pixels.
  Examples:
    * `param2 = 17` is 4 * 4 + 1, so the rotation is 1 and the
      fifth (= 4 + 1) pixel will be picked from the palette.
    * `param2 = 35` is 8 * 4 + 3, so the rotation is 3 and the
      ninth (= 8 + 1) pixel will be picked from the palette.

To colorize a node on the map, set its `param2` value (according
to the node's paramtype2).

### Conversion between nodes in the inventory and on the map

Static coloring is the same for both cases, there is no need
for conversion.

If the `ItemStack`'s metadata contains the `color` field, it will be
lost on placement, because nodes on the map can only use palettes.

If the `ItemStack`'s metadata contains the `palette_index` field, it is
automatically transferred between node and item forms by the engine,
when a player digs or places a colored node.
You can disable this feature by setting the `drop` field of the node
to itself (without metadata).
To transfer the color to a special drop, you need a drop table.

Example:

```lua
core.register_node("mod:stone", {
    description = "Stone",
    tiles = {"default_stone.png"},
    paramtype2 = "color",
    palette = "palette.png",
    drop = {
        items = {
            -- assume that mod:cobblestone also has the same palette
            {items = {"mod:cobblestone"}, inherit_color = true },
        }
    }
})
```

### Colored items in craft recipes

Craft recipes only support item strings, but fortunately item strings
can also contain metadata. Example craft recipe registration:

```lua
core.register_craft({
    output = core.itemstring_with_palette("wool:block", 3),
    type = "shapeless",
    recipe = {
        "wool:block",
        "dye:red",
    },
})
```

To set the `color` field, you can use `core.itemstring_with_color`.

Metadata field filtering in the `recipe` field are not supported yet,
so the craft output is independent of the color of the ingredients.
