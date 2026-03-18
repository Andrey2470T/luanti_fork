Registered entities
===================

Functions receive a "luaentity" table as `self`:

* It has the member `name`, which is the registered name `("mod:thing")`
* It has the member `object`, which is an `ObjectRef` pointing to the object
* The original prototype is visible directly via a metatable

Callbacks:

* `on_activate(self, staticdata, dtime_s)`
    * Called when the object is instantiated.
    * `dtime_s` is the time passed since the object was unloaded, which can be
      used for updating the entity state.
* `on_deactivate(self, removal)`
    * Called when the object is about to get removed or unloaded.
    * `removal`: boolean indicating whether the object is about to get removed.
      Calling `object:remove()` on an active object will call this with `removal=true`.
      The mapblock the entity resides in being unloaded will call this with `removal=false`.
    * Note that this won't be called if the object hasn't been activated in the first place.
      In particular, `core.clear_objects({mode = "full"})` won't call this,
      whereas `core.clear_objects({mode = "quick"})` might call this.
* `on_step(self, dtime, moveresult)`
    * Called on every server tick, after movement and collision processing.
    * `dtime`: elapsed time since last call
    * `moveresult`: table with collision info (only available if physical=true)
* `on_punch(self, puncher, time_from_last_punch, tool_capabilities, dir, damage)`
    * Called when somebody punches the object.
    * Note that you probably want to handle most punches using the automatic
      armor group system.
    * `puncher`: an `ObjectRef` (can be `nil`)
    * `time_from_last_punch`: Meant for disallowing spamming of clicks
      (can be `nil`).
    * `tool_capabilities`: capability table of used item (can be `nil`)
    * `dir`: unit vector of direction of punch. Always defined. Points from the
      puncher to the punched.
    * `damage`: damage that will be done to entity.
    * Can return `true` to prevent the default damage mechanism.
* `on_death(self, killer)`
    * Called when the object dies.
    * `killer`: an `ObjectRef` (can be `nil`)
* `on_rightclick(self, clicker)`
    * Called when `clicker` pressed the 'place/use' key while pointing
      to the object (not necessarily an actual rightclick)
    * `clicker`: an `ObjectRef` (may or may not be a player)
* `on_attach_child(self, child)`
    * Called after another object is attached to this object.
    * `child`: an `ObjectRef` of the child
* `on_detach_child(self, child)`
    * Called after another object has detached from this object.
    * `child`: an `ObjectRef` of the child
* `on_detach(self, parent)`
    * Called after detaching from another object.
    * `parent`: an `ObjectRef` from where it got detached
    * Note: this is also called before removal from the world.
* `get_staticdata(self)`
    * Should return a string that will be passed to `on_activate` when the
      object is instantiated the next time.

Collision info passed to `on_step` (`moveresult` argument):

```lua
{
    touching_ground = boolean,
    -- Note that touching_ground is only true if the entity was moving and
    -- collided with ground.

    collides = boolean,
    standing_on_object = boolean,

    collisions = {
        {
            type = string, -- "node" or "object",
            axis = string, -- "x", "y" or "z"
            node_pos = vector, -- if type is "node"
            object = ObjectRef, -- if type is "object"
            -- The position of the entity when the collision occurred.
            -- Available since feature "moveresult_new_pos".
            new_pos = vector,
            old_velocity = vector,
            new_velocity = vector,
        },
        ...
    }
    -- `collisions` does not contain data of unloaded mapblock collisions
    -- or when the velocity changes are negligibly small
}
```
