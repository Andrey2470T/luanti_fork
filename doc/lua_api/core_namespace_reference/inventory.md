Inventory
=========

`core.get_inventory(location)`: returns an `InvRef`

* `location` = e.g.
    * `{type="player", name="celeron55"}`
    * `{type="node", pos={x=, y=, z=}}`
    * `{type="detached", name="creative"}`
* `core.create_detached_inventory(name, callbacks, [player_name])`: returns
  an `InvRef`.
    * `callbacks`: See [Detached inventory callbacks]
    * `player_name`: Make detached inventory available to one player
      exclusively, by default they will be sent to every player (even if not
      used).
      Note that this parameter is mostly just a workaround and will be removed
      in future releases.
    * Creates a detached inventory. If it already exists, it is cleared.
* `core.remove_detached_inventory(name)`
    * Returns a `boolean` indicating whether the removal succeeded.
* `core.do_item_eat(hp_change, replace_with_item, itemstack, user, pointed_thing)`:
  returns leftover ItemStack or nil to indicate no inventory change
    * See `core.item_eat` and `core.register_on_item_eat`
