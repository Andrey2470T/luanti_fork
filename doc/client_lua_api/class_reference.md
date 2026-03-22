Class reference
===============

### ModChannel

An interface to use mod channels on client and server

#### Methods
* `leave()`: leave the mod channel.
    * Client leaves channel `channel_name`.
    * No more incoming or outgoing messages can be sent to this channel from client mods.
    * This invalidate all future object usage
    * Ensure your set mod_channel to nil after that to free Lua resources
* `is_writeable()`: returns true if channel is writable and mod can send over it.
* `send_all(message)`: Send `message` though the mod channel.
    * If mod channel is not writable or invalid, message will be dropped.
    * Message size is limited to 65535 characters by protocol.

### Minimap
An interface to manipulate minimap on client UI

#### Methods
* `show()`: shows the minimap (if not disabled by server)
* `hide()`: hides the minimap
* `set_pos(pos)`: sets the minimap position on screen
* `get_pos()`: returns the minimap current position
* `set_angle(deg)`: sets the minimap angle in degrees
* `get_angle()`: returns the current minimap angle in degrees
* `set_mode(mode)`: sets the minimap mode (0 to 6)
* `get_mode()`: returns the current minimap mode
* `set_shape(shape)`: Sets the minimap shape. (0 = square, 1 = round)
* `get_shape()`: Gets the minimap shape. (0 = square, 1 = round)

### Camera
An interface to get or set information about the camera and camera-node.
Please do not try to access the reference until the camera is initialized, otherwise the reference will be nil.

#### Methods
* `set_camera_mode(mode)`
    * Pass `0` for first-person, `1` for third person, and `2` for third person front
* `get_camera_mode()`
    * Returns 0, 1, or 2 as described above
* `get_fov()`
    * Returns a table with X, Y, maximum and actual FOV in degrees:

        ```lua
        {
            x = number,
            y = number,
            max = number,
            actual = number
        }
        ```

* `get_pos()`
    * Returns position of camera with view bobbing
* `get_offset()`
    * Returns eye offset vector
* `get_look_dir()`
    * Returns eye direction unit vector
* `get_look_vertical()`
    * Returns pitch in radians
* `get_look_horizontal()`
    * Returns yaw in radians
* `get_aspect_ratio()`
    * Returns aspect ratio of screen

### LocalPlayer
An interface to retrieve information about the player.
This object will only be available after the client is initialized. Earlier accesses will yield a `nil` value.

Methods:

* `get_pos()`
    * returns current player current position
* `get_velocity()`
    * returns player speed vector
* `get_hp()`
    * returns player HP
* `get_name()`
    * returns player name
* `get_wield_index()`
    * returns the index of the wielded item
* `get_wielded_item()`
    * returns the itemstack the player is holding
* `is_attached()`
    * returns true if player is attached
* `is_touching_ground()`
    * returns true if player touching ground
* `is_in_liquid()`
    * returns true if player is in a liquid (This oscillates so that the player jumps a bit above the surface)
* `is_in_liquid_stable()`
    * returns true if player is in a stable liquid (This is more stable and defines the maximum speed of the player)
* `get_move_resistance()`
    * returns move resistance of current node, the higher the slower the player moves
* `is_climbing()`
    * returns true if player is climbing
* `swimming_vertical()`
    * returns true if player is swimming in vertical
* `get_physics_override()`
    * returns:

        ```lua
        {
            speed = float,
            speed_climb = float,
            speed_crouch = float,
            speed_fast = float,
            speed_walk = float,
            acceleration_default = float,
            acceleration_air = float,
            acceleration_fast = float,
            jump = float,
            gravity = float,
            liquid_fluidity = float,
            liquid_fluidity_smooth = float,
            liquid_sink = float,
            sneak = boolean,
            sneak_glitch = boolean,
            new_move = boolean,
        }
        ```

* `get_override_pos()`
    * returns override position
* `get_last_pos()`
    * returns last player position before the current client step
* `get_last_velocity()`
    * returns last player speed
* `get_breath()`
    * returns the player's breath
* `get_movement_acceleration()`
    * returns acceleration of the player in different environments
      (note: does not take physics overrides into account):

        ```lua
        {
            fast = float,
            air = float,
            default = float,
        }
        ```

* `get_movement_speed()`
    * returns player's speed in different environments
      (note: does not take physics overrides into account):

        ```lua
        {
            walk = float,
            jump = float,
            crouch = float,
            fast = float,
            climb = float,
        }
        ```

* `get_movement()`
    * returns player's movement in different environments
      (note: does not take physics overrides into account):

        ```lua
        {
            liquid_fluidity = float,
            liquid_sink = float,
            liquid_fluidity_smooth = float,
            gravity = float,
        }
        ```

* `get_last_look_horizontal()`:
    * returns last look horizontal angle
* `get_last_look_vertical()`:
    * returns last look vertical angle
* `get_control()`:
    * returns pressed player controls

        ```lua
        {
            up = boolean,
            down = boolean,
            left = boolean,
            right = boolean,
            jump = boolean,
            aux1 = boolean,
            sneak = boolean,
            zoom = boolean,
            dig = boolean,
            place = boolean,
        }
        ```

* `get_armor_groups()`
    * returns a table with the armor group ratings
* `hud_add(definition)`
    * add a HUD element described by HUD def, returns ID number on success and `nil` on failure.
    * See [`HUD definition`](#hud-definition-hud_add-hud_get)
* `hud_get(id)`
    * returns the [`definition`](#hud-definition-hud_add-hud_get) of the HUD with that ID number or `nil`, if non-existent.
* `hud_get_all()`:
    * Returns a table in the form `{ [id] = HUD definition, [id] = ... }`.
    * A mod should keep track of its introduced IDs and only use this to access foreign elements.
    * It is discouraged to change foreign HUD elements.
* `hud_remove(id)`
    * remove the HUD element of the specified id, returns `true` on success
* `hud_change(id, stat, value)`
    * change a value of a previously added HUD element
    * element `stat` values: `position`, `name`, `scale`, `text`, `number`, `item`, `dir`
    * Returns `true` on success, otherwise returns `nil`

### Settings
An interface to read config files in the format of `minetest.conf`.

It can be created via `Settings(filename)`.

#### Methods
* `get(key)`: returns a value
* `get_bool(key)`: returns a boolean
* `set(key, value)`
* `remove(key)`: returns a boolean (`true` for success)
* `get_names()`: returns `{key1,...}`
* `write()`: returns a boolean (`true` for success)
    * write changes to file
* `to_table()`: returns `{[key1]=value1,...}`

### NodeMetaRef
Node metadata: reference extra data and functionality stored in a node.
Can be obtained via `core.get_meta(pos)`.

#### Methods
* `get_string(name)`
* `get_int(name)`
* `get_float(name)`
* `to_table()`: returns `nil` or a table with keys:
    * `fields`: key-value storage
    * `inventory`: `{list1 = {}, ...}}`

### `Raycast`

A raycast on the map. It works with selection boxes.
Can be used as an iterator in a for loop as:

```lua
local ray = Raycast(...)
for pointed_thing in ray do
    ...
end
```

The map is loaded as the ray advances. If the map is modified after the
`Raycast` is created, the changes may or may not have an effect on the object.

It can be created via `Raycast(pos1, pos2, objects, liquids)` or
`core.raycast(pos1, pos2, objects, liquids)` where:

* `pos1`: start of the ray
* `pos2`: end of the ray
* `objects`: if false, only nodes will be returned. Default is true.
* `liquids`: if false, liquid nodes won't be returned. Default is false.

#### Methods

* `next()`: returns a `pointed_thing` with exact pointing location
    * Returns the next thing pointed by the ray or nil.
