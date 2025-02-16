`InvRef`
--------

An `InvRef` is a reference to an inventory.

### Methods

* `is_empty(listname)`: return `true` if list is empty
* `get_size(listname)`: get size of a list
* `set_size(listname, size)`: set size of a list
    * If `listname` is not known, a new list will be created
    * Setting `size` to 0 deletes a list
    * returns `false` on error (e.g. invalid `listname` or `size`)
* `get_width(listname)`: get width of a list
* `set_width(listname, width)`: set width of list; currently used for crafting
    * returns `false` on error (e.g. invalid `listname` or `width`)
* `get_stack(listname, i)`: get a copy of stack index `i` in list
* `set_stack(listname, i, stack)`: copy `stack` to index `i` in list
* `get_list(listname)`: returns full list (list of `ItemStack`s)
                        or `nil` if list doesn't exist (size 0)
* `set_list(listname, list)`: set full list (size will not change)
* `get_lists()`: returns table that maps listnames to inventory lists
* `set_lists(lists)`: sets inventory lists (size will not change)
* `add_item(listname, stack)`: add item somewhere in list, returns leftover
  `ItemStack`.
* `room_for_item(listname, stack):` returns `true` if the stack of items
  can be fully added to the list
* `contains_item(listname, stack, [match_meta])`: returns `true` if
  the stack of items can be fully taken from the list.
  If `match_meta` is false, only the items' names are compared
  (default: `false`).
* `remove_item(listname, stack)`: take as many items as specified from the
  list, returns the items that were actually removed (as an `ItemStack`)
  -- note that any item metadata is ignored, so attempting to remove a specific
  unique item this way will likely remove the wrong one -- to do that use
  `set_stack` with an empty `ItemStack`.
* `get_location()`: returns a location compatible to
  `core.get_inventory(location)`.
    * returns `{type="undefined"}` in case location is not known

### Callbacks

Detached & nodemeta inventories provide the following callbacks for move actions:

#### Before

The `allow_*` callbacks return how many items can be moved.

* `allow_move`/`allow_metadata_inventory_move`: Moving items in the inventory
* `allow_take`/`allow_metadata_inventory_take`: Taking items from the inventory
* `allow_put`/`allow_metadata_inventory_put`: Putting items to the inventory

#### After

The `on_*` callbacks are called after the items have been placed in the inventories.

* `on_move`/`on_metadata_inventory_move`: Moving items in the inventory
* `on_take`/`on_metadata_inventory_take`: Taking items from the inventory
* `on_put`/`on_metadata_inventory_put`: Putting items to the inventory

#### Swapping

When a player tries to put an item to a place where another item is, the items are *swapped*.
This means that all callbacks will be called twice (once for each action).
