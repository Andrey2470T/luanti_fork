Inventory
=========

Inventory locations
-------------------

* `"context"`: Selected node metadata (deprecated: `"current_name"`)
* `"current_player"`: Player to whom the menu is shown
* `"player:<name>"`: Any player
* `"nodemeta:<X>,<Y>,<Z>"`: Any node metadata
* `"detached:<name>"`: A detached inventory

Player Inventory lists
----------------------

* `main`: list containing the default inventory
* `craft`: list containing the craft input
* `craftpreview`: list containing the craft prediction
* `craftresult`: list containing the crafted output
* `hand`: list containing an override for the empty hand
    * Is not created automatically, use `InvRef:set_size`
    * Is only used to enhance the empty hand's tool capabilities

Custom lists can be added and deleted with `InvRef:set_size(name, size)` like
any other inventory.

ItemStack transaction order
---------------------------

This list describes the situation for non-empty ItemStacks in both slots
that cannot be stacked at all, hence triggering an ItemStack swap operation.
Put/take callbacks on empty ItemStack are not executed.

1. The "allow take" and "allow put" callbacks are each run once for the source
   and destination inventory.
2. The allowed ItemStacks are exchanged.
3. The "on take" callbacks are run for the source and destination inventories
4. The "on put" callbacks are run for the source and destination inventories
