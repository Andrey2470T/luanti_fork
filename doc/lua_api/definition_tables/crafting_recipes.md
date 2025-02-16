Crafting recipes
----------------

Crafting converts one or more inputs to one output itemstack of arbitrary
count (except for fuels, which don't have an output). The conversion reduces
each input ItemStack by 1.

Craft recipes are registered by `core.register_craft` and use a
table format. The accepted parameters are listed below.

Recipe input items can either be specified by item name (item count = 1)
or by group (see "Groups in crafting recipes" for details).

The following sections describe the types and syntaxes of recipes.

### Shaped

This is the default recipe type (when no `type` is specified).

A shaped recipe takes one or multiple items as input and has
a single item stack as output. The input items must be specified
in a 2-dimensional matrix (see parameters below) to specify the
exact arrangement (the "shape") in which the player must place them
in the crafting grid.

For example, for a 3x3 recipe, the `recipes` table must have
3 rows and 3 columns.

In order to craft the recipe, the players' crafting grid must
have equal or larger dimensions (both width and height).

Parameters:

* `type = "shaped"`: (optional) specifies recipe type as shaped
* `output`: Itemstring of output itemstack (item counts >= 1 are allowed)
* `recipe`: A 2-dimensional matrix of items, with a width *w* and height *h*.
    * *w* and *h* are chosen by you, they don't have to be equal but must be at least 1
    * The matrix is specified as a table containing tables containing itemnames
    * The inner tables are the rows. There must be *h* tables, specified from the top to the bottom row
    * Values inside of the inner table are the columns.
      Each inner table must contain a list of *w* items, specified from left to right
    * Empty slots *must* be filled with the empty string
* `replacements`: (optional) Allows you to replace input items with some other items
      when something is crafted
    * Provided as a list of item pairs of the form `{ old_item, new_item }` where
      `old_item` is the input item to replace (same syntax as for a regular input
      slot; groups are allowed) and `new_item` is an itemstring for the item stack
      it will become
    * When the output is crafted, Luanti iterates through the list
      of input items if the crafting grid. For each input item stack, it checks if
      it matches with an `old_item` in the item pair list.
        * If it matches, the item will be replaced. Also, this item pair
          will *not* be applied again for the remaining items
        * If it does not match, the item is consumed (reduced by 1) normally
    * The `new_item` will appear in one of 3 places:
        * Crafting grid, if the input stack size was exactly 1
        * Player inventory, if input stack size was larger
        * Drops as item entity, if it fits neither in craft grid or inventory

#### Examples

A typical shaped recipe:

```lua
-- Stone pickaxe
{
    output = "example:stone_pickaxe",
    -- A 3x3 recipe which needs 3 stone in the 1st row,
    -- and 1 stick in the horizontal middle in each of the 2nd and 3nd row.
    -- The 4 remaining slots have to be empty.
    recipe = {
        {"example:stone", "example:stone", "example:stone"}, -- row 1
        {"",              "example:stick", ""             }, -- row 2
        {"",              "example:stick", ""             }, -- row 3
    --   ^ column 1       ^ column 2       ^ column 3
    },
    -- There is no replacements table, so every input item
    -- will be consumed.
}
```

Simple replacement example:

```lua
-- Wet sponge
{
    output = "example:wet_sponge",
    -- 1x2 recipe with a water bucket above a dry sponge
    recipe = {
        {"example:water_bucket"},
        {"example:dry_sponge"},
    },
    -- When the wet sponge is crafted, the water bucket
    -- in the input slot is replaced with an empty
    -- bucket
    replacements = {
        {"example:water_bucket", "example:empty_bucket"},
    },
}
```

Complex replacement example 1:

```lua
-- Very wet sponge
{
    output = "example:very_wet_sponge",
    -- 3x3 recipe with a wet sponge in the center
    -- and 4 water buckets around it
    recipe = {
        {"","example:water_bucket",""},
        {"example:water_bucket","example:wet_sponge","example:water_bucket"},
        {"","example:water_bucket",""},
    },
    -- When the wet sponge is crafted, all water buckets
    -- in the input slot become empty
    replacements = {
        -- Without these repetitions, only the first
        -- water bucket would be replaced.
        {"example:water_bucket", "example:empty_bucket"},
        {"example:water_bucket", "example:empty_bucket"},
        {"example:water_bucket", "example:empty_bucket"},
        {"example:water_bucket", "example:empty_bucket"},
    },
}
```

Complex replacement example 2:

```lua
-- Magic book:
-- 3 magic orbs + 1 book crafts a magic book,
-- and the orbs will be replaced with 3 different runes.
{
    output = "example:magic_book",
    -- 3x2 recipe
    recipe = {
        -- 3 items in the group `magic_orb` on top of a book in the middle
        {"group:magic_orb", "group:magic_orb", "group:magic_orb"},
        {"", "example:book", ""},
    },
    -- When the book is crafted, the 3 magic orbs will be turned into
    -- 3 runes: ice rune, earth rune and fire rune (from left to right)
    replacements = {
        {"group:magic_orb", "example:ice_rune"},
        {"group:magic_orb", "example:earth_rune"},
        {"group:magic_orb", "example:fire_rune"},
    },
}
```

### Shapeless

Takes a list of input items (at least 1). The order or arrangement
of input items does not matter.

In order to craft the recipe, the players' crafting grid must have matching or
larger *count* of slots. The grid dimensions do not matter.

Parameters:

* `type = "shapeless"`: Mandatory
* `output`: Same as for shaped recipe
* `recipe`: List of item names
* `replacements`: Same as for shaped recipe

#### Example

```lua
{
    -- Craft a mushroom stew from a bowl, a brown mushroom and a red mushroom
    -- (no matter where in the input grid the items are placed)
    type = "shapeless",
    output = "example:mushroom_stew",
    recipe = {
        "example:bowl",
        "example:mushroom_brown",
        "example:mushroom_red",
    },
}
```

### Tool repair

Syntax:

    {
        type = "toolrepair",
        additional_wear = -0.02, -- multiplier of 65536
    }

Adds a shapeless recipe for *every* tool that doesn't have the `disable_repair=1`
group. If this recipe is used, repairing is possible with any crafting grid
with at least 2 slots.
The player can put 2 equal tools in the craft grid to get one "repaired" tool
back.
The wear of the output is determined by the wear of both tools, plus a
'repair bonus' given by `additional_wear`. To reduce the wear (i.e. 'repair'),
you want `additional_wear` to be negative.

The formula used to calculate the resulting wear is:

    65536 * (1 - ( (1 - tool_1_wear) + (1 - tool_2_wear) + additional_wear))

The result is rounded and can't be lower than 0. If the result is 65536 or higher,
no crafting is possible.

### Cooking

A cooking recipe has a single input item, a single output item stack
and a cooking time. It represents cooking/baking/smelting/etc. items in
an oven, furnace, or something similar; the exact meaning is up for games
to decide, if they choose to use cooking at all.

The engine does not implement anything specific to cooking recipes, but
the recipes can be retrieved later using `core.get_craft_result` to
have a consistent interface across different games/mods.

Parameters:

* `type = "cooking"`: Mandatory
* `output`: Same as for shaped recipe
* `recipe`: An itemname of the single input item
* `cooktime`: (optional) Time it takes to cook this item, in seconds.
              A floating-point number. (default: 3.0)
* `replacements`: Same meaning as for shaped recipes, but the mods
                  that utilize cooking recipes (e.g. for adding a furnace
                  node) need to implement replacements on their own

Note: Games and mods are free to re-interpret the cooktime in special
cases, e.g. for a super furnace that cooks items twice as fast.

#### Example

Cooking sand to glass in 3 seconds:

```lua
{
    type = "cooking",
    output = "example:glass",
    recipe = "example:sand",
    cooktime = 3.0,
}
```

### Fuel

A fuel recipe is an item associated with a "burning time" and an optional
item replacement. There is no output. This is usually used as fuel for
furnaces, ovens, stoves, etc.

Like with cooking recipes, the engine does not do anything specific with
fuel recipes and it's up to games and mods to use them by retrieving
them via `core.get_craft_result`.

Parameters:

* `type = "fuel"`: Mandatory
* `recipe`: Itemname of the item to be used as fuel
* `burntime`: (optional) Burning time this item provides, in seconds.
              A floating-point number. (default: 1.0)
* `replacements`: Same meaning as for shaped recipes, but the mods
                  that utilize fuels need to implement replacements
                  on their own

Note: Games and mods are free to re-interpret the burntime in special
cases, e.g. for an efficient furnace in which fuels burn twice as
long.

#### Examples

Coal lump with a burntime of 20 seconds. Will be consumed when used.

```lua
{
    type = "fuel",
    recipe = "example:coal_lump",
    burntime = 20.0,
}
```

Lava bucket with a burn time of 60 seconds. Will become an empty bucket
if used:

```lua
{
    type = "fuel",
    recipe = "example:lava_bucket",
    burntime = 60.0,
    replacements = {{"example:lava_bucket", "example:empty_bucket"}},
}
```
