Entity damage mechanism
=======================

Damage calculation:

    damage = 0
    foreach group in cap.damage_groups:
        damage += cap.damage_groups[group]
            * limit(actual_interval / cap.full_punch_interval, 0.0, 1.0)
            * (object.armor_groups[group] / 100.0)
            -- Where object.armor_groups[group] is 0 for inexistent values
    return damage

Client predicts damage based on damage groups. Because of this, it is able to
give an immediate response when an entity is damaged or dies; the response is
pre-defined somehow (e.g. by defining a sprite animation) (not implemented;
TODO).
Currently a smoke puff will appear when an entity dies.

The group `immortal` completely disables normal damage.

Entities can define a special armor group, which is `punch_operable`. This
group disables the regular damage mechanism for players punching it by hand or
a non-tool item, so that it can do something else than take damage.

On the Lua side, every punch calls:

```lua
entity:on_punch(puncher, time_from_last_punch, tool_capabilities, direction,
                damage)
```

This should never be called directly, because damage is usually not handled by
the entity itself.

* `puncher` is the object performing the punch. Can be `nil`. Should never be
  accessed unless absolutely required, to encourage interoperability.
* `time_from_last_punch` is time from last punch (by `puncher`) or `nil`.
* `tool_capabilities` can be `nil`.
* `direction` is a unit vector, pointing from the source of the punch to
   the punched object.
* `damage` damage that will be done to entity
Return value of this function will determine if damage is done by this function
(retval true) or shall be done by engine (retval false)

To punch an entity/object in Lua, call:

```lua
object:punch(puncher, time_from_last_punch, tool_capabilities, direction)
```

* Return value is tool wear.
* Parameters are equal to the above callback.
* If `direction` equals `nil` and `puncher` does not equal `nil`, `direction`
  will be automatically filled in based on the location of `puncher`.
