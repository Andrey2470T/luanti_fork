Soft texture overlay
====================

Sometimes hardware coloring is not enough, because it affects the
whole tile. Soft texture overlays were added to Luanti to allow
the dynamic coloring of only specific parts of the node's texture.
For example a grass block may have colored grass, while keeping the
dirt brown.

These overlays are 'soft', because unlike texture modifiers, the layers
are not merged in the memory, but they are simply drawn on top of each
other. This allows different hardware coloring, but also means that
tiles with overlays are drawn slower. Using too much overlays might
cause FPS loss.

For inventory and wield images you can specify overlays which
hardware coloring does not modify. You have to set `inventory_overlay`
and `wield_overlay` fields to an image name.

To define a node overlay, simply set the `overlay_tiles` field of the node
definition. These tiles are defined in the same way as plain tiles:
they can have a texture name, color etc.
To skip one face, set that overlay tile to an empty string.

Example (colored grass block):

```lua
core.register_node("default:dirt_with_grass", {
    description = "Dirt with Grass",
    -- Regular tiles, as usual
    -- The dirt tile disables palette coloring
    tiles = {{name = "default_grass.png"},
        {name = "default_dirt.png", color = "white"}},
    -- Overlay tiles: define them in the same style
    -- The top and bottom tile does not have overlay
    overlay_tiles = {"", "",
        {name = "default_grass_side.png"}},
    -- Global color, used in inventory
    color = "green",
    -- Palette in the world
    paramtype2 = "color",
    palette = "default_foilage.png",
})
```
