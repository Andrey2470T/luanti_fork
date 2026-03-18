Metadata
========

Node Metadata
-------------

The instance of a node in the world normally only contains the three values
mentioned in [Nodes]. However, it is possible to insert extra data into a node.
It is called "node metadata"; See `NodeMetaRef`.

Node metadata contains two things:

* A key-value store
* An inventory

Some of the values in the key-value store are handled specially:

* `formspec`: Defines an inventory menu that is opened with the
              'place/use' key. Only works if no `on_rightclick` was
              defined for the node. See also [Formspec].
* `infotext`: Text shown on the screen when the node is pointed at.
              Line-breaks will be applied automatically.
              If the infotext is very long, it will be truncated.

Example:

```lua
local meta = core.get_meta(pos)

-- Set node formspec and infotext
meta:set_string("formspec",
        "size[8,9]"..
        "list[context;main;0,0;8,4;]"..
        "list[current_player;main;0,5;8,4;]")
meta:set_string("infotext", "Chest");

-- Set inventory list size of `"main"` list to 32
local inv = meta:get_inventory()
inv:set_size("main", 32)

-- Dump node metadata
print(dump(meta:to_table()))

-- Set node metadata from a metadata table
meta:from_table({
    inventory = {
        -- Set items of inventory in all 32 slots of the `"main"` list
        main = {[1] = "default:dirt", [2] = "", [3] = "", [4] = "",
                [5] = "", [6] = "", [7] = "", [8] = "", [9] = "",
                [10] = "", [11] = "", [12] = "", [13] = "",
                [14] = "default:cobble", [15] = "", [16] = "", [17] = "",
                [18] = "", [19] = "", [20] = "default:cobble", [21] = "",
                [22] = "", [23] = "", [24] = "", [25] = "", [26] = "",
                [27] = "", [28] = "", [29] = "", [30] = "", [31] = "",
                [32] = ""}
    },
    -- metadata fields
    fields = {
        formspec = "size[8,9]list[context;main;0,0;8,4;]list[current_player;main;0,5;8,4;]",
        infotext = "Chest"
    }
})
```

Item Metadata
-------------

Item stacks can store metadata too. See [`ItemStackMetaRef`].

Item metadata only contains a key-value store.

Some of the values in the key-value store are handled specially:

* `description`: Set the item stack's description.
  See also: `get_description` in [`ItemStack`]
* `short_description`: Set the item stack's short description.
  See also: `get_short_description` in [`ItemStack`]
* `inventory_image`: Override inventory_image
* `inventory_overlay`: Override inventory_overlay
* `wield_image`: Override wield_image
* `wield_overlay`: Override wield_overlay
* `wield_scale`: Override wield_scale, use vector.to_string
* `color`: A `ColorString`, which sets the stack's color.
* `palette_index`: If the item has a palette, this is used to get the
  current color from the palette.
* `count_meta`: Replace the displayed count with any string.
* `count_alignment`: Set the alignment of the displayed count value. This is an
  int value. The lowest 2 bits specify the alignment in x-direction, the 3rd and
  4th bit specify the alignment in y-direction:
  0 = default, 1 = left / up, 2 = middle, 3 = right / down
  The default currently is the same as right/down.
  Example: 6 = 2 + 1*4 = middle,up
* `range`: Overrides the pointing range
  Example: `meta:set_float("range", 4.2)`

Example:

```lua
local meta = stack:get_meta()
meta:set_string("key", "value")
print(dump(meta:to_table()))
```

Example manipulations of "description" and expected output behaviors:

```lua
print(ItemStack("default:pick_steel"):get_description()) --> Steel Pickaxe
print(ItemStack("foobar"):get_description()) --> Unknown Item

local stack = ItemStack("default:stone")
stack:get_meta():set_string("description", "Custom description\nAnother line")
print(stack:get_description()) --> Custom description\nAnother line
print(stack:get_short_description()) --> Custom description

stack:get_meta():set_string("short_description", "Short")
print(stack:get_description()) --> Custom description\nAnother line
print(stack:get_short_description()) --> Short

print(ItemStack("mod:item_with_no_desc"):get_description()) --> mod:item_with_no_desc
```
