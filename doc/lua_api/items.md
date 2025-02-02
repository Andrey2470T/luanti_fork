Items
=====

Items are things that can be held by players, dropped in the map and
stored in inventories.
Items come in the form of item stacks, which are collections of equal
items that occupy a single inventory slot.

Item types
----------

There are three kinds of items: nodes, tools and craftitems.

* Node: Placeable item form of a node in the world's voxel grid
* Tool: Has a changeable wear property but cannot be stacked
* Craftitem: Has no special properties

Every registered node (the voxel in the world) has a corresponding
item form (the thing in your inventory) that comes along with it.
This item form can be placed which will create a node in the
world (by default).
Both the 'actual' node and its item form share the same identifier.
For all practical purposes, you can treat the node and its item form
interchangeably. We usually just say 'node' to the item form of
the node as well.

Note the definition of tools is purely technical. The only really
unique thing about tools is their wear, and that's basically it.
Beyond that, you can't make any gameplay-relevant assumptions
about tools or non-tools. It is perfectly valid to register something
that acts as tool in a gameplay sense as a craftitem, and vice-versa.

Craftitems can be used for items that neither need to be a node
nor a tool.

Amount and wear
---------------

All item stacks have an amount between 0 and 65535. It is 1 by
default. Tool item stacks cannot have an amount greater than 1.

Tools use a wear (damage) value ranging from 0 to 65535. The
value 0 is the default and is used for unworn tools. The values
1 to 65535 are used for worn tools, where a higher value stands for
a higher wear. Non-tools technically also have a wear property,
but it is always 0. There is also a special 'toolrepair' crafting
recipe that is only available to tools.

Item formats
------------

Items and item stacks can exist in three formats: Serializes, table format
and `ItemStack`.

When an item must be passed to a function, it can usually be in any of
these formats.

### Serialized

This is called "stackstring" or "itemstring". It is a simple string with
1-4 components:

1. Full item identifier ("item name")
2. Optional amount
3. Optional wear value
4. Optional item metadata

Syntax:

    <identifier> [<amount>[ <wear>[ <metadata>]]]

Examples:

* `"default:apple"`: 1 apple
* `"default:dirt 5"`: 5 dirt
* `"default:pick_stone"`: a new stone pickaxe
* `"default:pick_wood 1 21323"`: a wooden pickaxe, ca. 1/3 worn out
* `[[default:pick_wood 1 21323 "\u0001description\u0002My worn out pick\u0003"]]`:
  * a wooden pickaxe from the `default` mod,
  * amount must be 1 (pickaxe is a tool), ca. 1/3 worn out (it's a tool),
  * with the `description` field set to `"My worn out pick"` in its metadata
* `[[default:dirt 5 0 "\u0001description\u0002Special dirt\u0003"]]`:
  * analogous to the above example
  * note how the wear is set to `0` as dirt is not a tool

You should ideally use the `ItemStack` format to build complex item strings
(especially if they use item metadata)
without relying on the serialization format. Example:

    local stack = ItemStack("default:pick_wood")
    stack:set_wear(21323)
    stack:get_meta():set_string("description", "My worn out pick")
    local itemstring = stack:to_string()

Additionally the methods `core.itemstring_with_palette(item, palette_index)`
and `core.itemstring_with_color(item, colorstring)` may be used to create
item strings encoding color information in their metadata.

### Table format

Examples:

5 dirt nodes:

```lua
{name="default:dirt", count=5, wear=0, metadata=""}
```

A wooden pick about 1/3 worn out:

```lua
{name="default:pick_wood", count=1, wear=21323, metadata=""}
```

An apple:

```lua
{name="default:apple", count=1, wear=0, metadata=""}
```

### `ItemStack`

A native C++ format with many helper methods. Useful for converting
between formats. See the [Class reference] section for details.
