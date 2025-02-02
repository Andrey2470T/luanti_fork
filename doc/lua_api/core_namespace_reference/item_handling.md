Item handling
=============

* `core.inventorycube(img1, img2, img3)`
    * Returns a string for making an image of a cube (useful as an item image)
* `core.get_pointed_thing_position(pointed_thing, above)`
    * Returns the position of a `pointed_thing` or `nil` if the `pointed_thing`
      does not refer to a node or entity.
    * If the optional `above` parameter is true and the `pointed_thing` refers
      to a node, then it will return the `above` position of the `pointed_thing`.
* `core.dir_to_facedir(dir, is6d)`
    * Convert a vector to a facedir value, used in `param2` for
      `paramtype2="facedir"`.
    * passing something non-`nil`/`false` for the optional second parameter
      causes it to take the y component into account.
* `core.facedir_to_dir(facedir)`
    * Convert a facedir back into a vector aimed directly out the "back" of a
      node.
* `core.dir_to_fourdir(dir)`
    * Convert a vector to a 4dir value, used in `param2` for
      `paramtype2="4dir"`.
* `core.fourdir_to_dir(fourdir)`
    * Convert a 4dir back into a vector aimed directly out the "back" of a
      node.
* `core.dir_to_wallmounted(dir)`
    * Convert a vector to a wallmounted value, used for
      `paramtype2="wallmounted"`.
* `core.wallmounted_to_dir(wallmounted)`
    * Convert a wallmounted value back into a vector aimed directly out the
      "back" of a node.
* `core.dir_to_yaw(dir)`
    * Convert a vector into a yaw (angle)
* `core.yaw_to_dir(yaw)`
    * Convert yaw (angle) to a vector
* `core.is_colored_paramtype(ptype)`
    * Returns a boolean. Returns `true` if the given `paramtype2` contains
      color information (`color`, `colorwallmounted`, `colorfacedir`, etc.).
* `core.strip_param2_color(param2, paramtype2)`
    * Removes everything but the color information from the
      given `param2` value.
    * Returns `nil` if the given `paramtype2` does not contain color
      information.
* `core.get_node_drops(node, toolname)`
    * Returns list of itemstrings that are dropped by `node` when dug
      with the item `toolname` (not limited to tools).
    * `node`: node as table or node name
    * `toolname`: name of the item used to dig (can be `nil`)
* `core.get_craft_result(input)`: returns `output, decremented_input`
    * `input.method` = `"normal"` or `"cooking"` or `"fuel"`
    * `input.width` = for example `3`
    * `input.items` = for example
      `{stack1, stack2, stack3, stack4, stack 5, stack 6, stack 7, stack 8, stack 9}`
    * `output.item` = `ItemStack`, if unsuccessful: empty `ItemStack`
    * `output.time` = a number, if unsuccessful: `0`
    * `output.replacements` = List of replacement `ItemStack`s that couldn't be
      placed in `decremented_input.items`. Replacements can be placed in
      `decremented_input` if the stack of the replaced item has a count of 1.
    * `decremented_input` = like `input`
* `core.get_craft_recipe(output)`: returns input
    * returns last registered recipe for output item (node)
    * `output` is a node or item type such as `"default:torch"`
    * `input.method` = `"normal"` or `"cooking"` or `"fuel"`
    * `input.width` = for example `3`
    * `input.items` = for example
      `{stack1, stack2, stack3, stack4, stack 5, stack 6, stack 7, stack 8, stack 9}`
        * `input.items` = `nil` if no recipe found
* `core.get_all_craft_recipes(query item)`: returns a table or `nil`
    * returns indexed table with all registered recipes for query item (node)
      or `nil` if no recipe was found.
    * recipe entry table:
        * `method`: 'normal' or 'cooking' or 'fuel'
        * `width`: 0-3, 0 means shapeless recipe
        * `items`: indexed [1-9] table with recipe items
        * `output`: string with item name and quantity
    * Example result for `"default:gold_ingot"` with two recipes:
      ```lua
      {
          {
              method = "cooking", width = 3,
              output = "default:gold_ingot", items = {"default:gold_lump"}
          },
          {
              method = "normal", width = 1,
              output = "default:gold_ingot 9", items = {"default:goldblock"}
          }
      }
      ```

* `core.handle_node_drops(pos, drops, digger)`
    * `drops`: list of itemstrings
    * Handles drops from nodes after digging: Default action is to put them
      into digger's inventory.
    * Can be overridden to get different functionality (e.g. dropping items on
      ground)
* `core.itemstring_with_palette(item, palette_index)`: returns an item
  string.
    * Creates an item string which contains palette index information
      for hardware colorization. You can use the returned string
      as an output in a craft recipe.
    * `item`: the item stack which becomes colored. Can be in string,
      table and native form.
    * `palette_index`: this index is added to the item stack
* `core.itemstring_with_color(item, colorstring)`: returns an item string
    * Creates an item string which contains static color information
      for hardware colorization. Use this method if you wish to colorize
      an item that does not own a palette. You can use the returned string
      as an output in a craft recipe.
    * `item`: the item stack which becomes colored. Can be in string,
      table and native form.
    * `colorstring`: the new color of the item stack

Defaults for the `on_place` and `on_drop` item definition functions
-------------------------------------------------------------------

* `core.item_place_node(itemstack, placer, pointed_thing[, param2, prevent_after_place])`
    * Place item as a node
    * `param2` overrides `facedir` and wallmounted `param2`
    * `prevent_after_place`: if set to `true`, `after_place_node` is not called
      for the newly placed node to prevent a callback and placement loop
    * returns `itemstack, position`
      * `position`: the location the node was placed to. `nil` if nothing was placed.
* `core.item_place_object(itemstack, placer, pointed_thing)`
    * Place item as-is
    * returns the leftover itemstack
    * **Note**: This function is deprecated and will never be called.
* `core.item_place(itemstack, placer, pointed_thing[, param2])`
    * Wrapper that calls `core.item_place_node` if appropriate
    * Calls `on_rightclick` of `pointed_thing.under` if defined instead
    * **Note**: is not called when wielded item overrides `on_place`
    * `param2` overrides facedir and wallmounted `param2`
    * returns `itemstack, position`
      * `position`: the location the node was placed to. `nil` if nothing was placed.
* `core.item_pickup(itemstack, picker, pointed_thing, time_from_last_punch, ...)`
    * Runs callbacks registered by `core.register_on_item_pickup` and adds
      the item to the picker's `"main"` inventory list.
    * Parameters are the same as in `on_pickup`.
    * Returns the leftover itemstack.
* `core.item_drop(itemstack, dropper, pos)`
    * Drop the item
    * returns the leftover itemstack
* `core.item_eat(hp_change[, replace_with_item])`
    * Returns `function(itemstack, user, pointed_thing)` as a
      function wrapper for `core.do_item_eat`.
    * `replace_with_item` is the itemstring which is added to the inventory.
      If the player is eating a stack and `replace_with_item` doesn't fit onto
      the eaten stack, then the remainings go to a different spot, or are dropped.

Defaults for the `on_punch` and `on_dig` node definition callbacks
------------------------------------------------------------------

* `core.node_punch(pos, node, puncher, pointed_thing)`
    * Calls functions registered by `core.register_on_punchnode()`
* `core.node_dig(pos, node, digger)`
    * Checks if node can be dug, puts item into inventory, removes node
    * Calls functions registered by `core.registered_on_dignodes()`
