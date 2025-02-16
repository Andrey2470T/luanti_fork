Detached inventory callbacks
----------------------------

Used by `core.create_detached_inventory`.

```lua
{
    allow_move = function(inv, from_list, from_index, to_list, to_index, count, player),
    -- Called when a player wants to move items inside the inventory.
    -- Return value: number of items allowed to move.

    allow_put = function(inv, listname, index, stack, player),
    -- Called when a player wants to put something into the inventory.
    -- Return value: number of items allowed to put.
    -- Return value -1: Allow and don't modify item count in inventory.

    allow_take = function(inv, listname, index, stack, player),
    -- Called when a player wants to take something out of the inventory.
    -- Return value: number of items allowed to take.
    -- Return value -1: Allow and don't modify item count in inventory.

    on_move = function(inv, from_list, from_index, to_list, to_index, count, player),
    on_put = function(inv, listname, index, stack, player),
    on_take = function(inv, listname, index, stack, player),
    -- Called after the actual action has happened, according to what was
    -- allowed.
    -- No return value.
}
```
