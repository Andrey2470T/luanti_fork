Entity definition
-----------------

Used by `core.register_entity`.
The entity definition table becomes a metatable of a newly created per-entity
luaentity table, meaning its fields (e.g. `initial_properties`) will be shared
between all instances of an entity.

```lua
{
    initial_properties = {
        visual = "mesh",
        mesh = "boats_boat.obj",
        ...,
    },
    -- A table of object properties, see the `Object properties` section.
    -- The properties in this table are applied to the object
    -- once when it is spawned.

    -- Refer to the "Registered entities" section for explanations
    on_activate = function(self, staticdata, dtime_s) end,
    on_deactivate = function(self, removal) end,
    on_step = function(self, dtime, moveresult) end,
    on_punch = function(self, puncher, time_from_last_punch, tool_capabilities, dir, damage) end,
    on_death = function(self, killer) end,
    on_rightclick = function(self, clicker) end,
    on_attach_child = function(self, child) end,
    on_detach_child = function(self, child) end,
    on_detach = function(self, parent) end,
    get_staticdata = function(self) end,

    _custom_field = whatever,
    -- You can define arbitrary member variables here (see Item definition
    -- for more info) by using a '_' prefix
}
```
