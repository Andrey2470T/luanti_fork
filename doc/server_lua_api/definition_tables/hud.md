HUD Definition
--------------

Since most values have multiple different functions, please see the
documentation in [HUD] section.

Used by `ObjectRef:hud_add`. Returned by `ObjectRef:hud_get`.

```lua
{
    type = "image",
    -- Type of element, can be "compass", "hotbar" (46 ¹), "image", "image_waypoint",
    -- "inventory", "minimap" (44 ¹), "statbar", "text" or "waypoint"
    -- ¹: minimal protocol version for client-side support
    -- If undefined "text" will be used.

    hud_elem_type = "image",
    -- Deprecated, same as `type`.
    -- In case both are specified `type` will be used.

    position = {x=0.5, y=0.5},
    -- Top left corner position of element

    name = "<name>",

    scale = {x = 1, y = 1},

    text = "<text>",

    text2 = "<text>",

    number = 0,

    item = 0,

    direction = 0,
    -- Direction: 0: left-right, 1: right-left, 2: top-bottom, 3: bottom-top

    alignment = {x=0, y=0},

    offset = {x=0, y=0},

    world_pos = {x=0, y=0, z=0},

    size = {x=0, y=0},

    z_index = 0,
    -- Z index: lower z-index HUDs are displayed behind higher z-index HUDs

    style = 0,
}
```
