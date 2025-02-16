`ObjectRef`
-----------

Moving things in the game are generally these.
This is basically a reference to a C++ `ServerActiveObject`.

### Advice on handling `ObjectRefs`

When you receive an `ObjectRef` as a callback argument or from another API
function, it is possible to store the reference somewhere and keep it around.
It will keep functioning until the object is unloaded or removed.

However, doing this is **NOT** recommended - `ObjectRefs` should be "let go"
of as soon as control is returned from Lua back to the engine.

Doing so is much less error-prone and you will never need to wonder if the
object you are working with still exists.

If this is not feasible, you can test whether an `ObjectRef` is still valid
via `object:is_valid()`.

Getters may be called for invalid objects and will return nothing then.
All other methods should not be called on invalid objects.

### Attachments

It is possible to attach objects to other objects (`set_attach` method).

When an object is attached, it is positioned relative to the parent's position
and rotation. `get_pos` and `get_rotation` will always return the parent's
values and changes via their setter counterparts are ignored.

To change position or rotation call `set_attach` again with the new values.

**Note**: Just like model dimensions, the relative position in `set_attach`
must be multiplied by 10 compared to world positions.

It is also possible to attach to a bone of the parent object. In that case the
child will follow movement and rotation of that bone.

### Methods

* `is_valid()`: returns whether the object is valid.
   * See "Advice on handling `ObjectRefs`" above.
* `get_pos()`: returns position as vector `{x=num, y=num, z=num}`
* `set_pos(pos)`:
    * Sets the position of the object.
    * No-op if object is attached.
    * `pos` is a vector `{x=num, y=num, z=num}`
* `add_pos(pos)`:
    * Changes position by adding to the current position.
    * No-op if object is attached.
    * `pos` is a vector `{x=num, y=num, z=num}`.
    * In comparison to using `set_pos`, `add_pos` will avoid synchronization problems.
* `get_velocity()`: returns the velocity, a vector.
* `add_velocity(vel)`
    * Changes velocity by adding to the current velocity.
    * `vel` is a vector, e.g. `{x=0.0, y=2.3, z=1.0}`
    * In comparison to using `get_velocity`, adding the velocity and then using
      `set_velocity`, `add_velocity` is supposed to avoid synchronization problems.
      Additionally, players also do not support `set_velocity`.
    * If object is a player:
        * Does not apply during `free_move`.
        * Note that since the player speed is normalized at each move step,
          increasing e.g. Y velocity beyond what would usually be achieved
          (see: physics overrides) will cause existing X/Z velocity to be reduced.
        * Example: `add_velocity({x=0, y=6.5, z=0})` is equivalent to
          pressing the jump key (assuming default settings)
* `move_to(pos, continuous=false)`
    * Does an interpolated move for Lua entities for visually smooth transitions.
    * If `continuous` is true, the Lua entity will not be moved to the current
      position before starting the interpolated move.
    * For players this does the same as `set_pos`,`continuous` is ignored.
    * no-op if object is attached
* `punch(puncher, time_from_last_punch, tool_capabilities, dir)`
    * punches the object, triggering all consequences a normal punch would have
    * `puncher`: another `ObjectRef` which punched the object or `nil`
    * `dir`: direction vector of punch
    * Other arguments: See `on_punch` for entities
    * Arguments `time_from_last_punch`, `tool_capabilities`, and `dir`
      will be replaced with a default value when the caller sets them to `nil`.
* `right_click(clicker)`:
    * simulates using the 'place/use' key on the object
    * triggers all consequences as if a real player had done this
    * `clicker` is another `ObjectRef` which has clicked
    * note: this is called `right_click` for historical reasons only
* `get_hp()`: returns number of health points
* `set_hp(hp, reason)`: set number of health points
    * See reason in register_on_player_hpchange
    * Is limited to the range of 0 ... 65535 (2^16 - 1)
    * For players: HP are also limited by `hp_max` specified in object properties
* `get_inventory()`: returns an `InvRef` for players, otherwise returns `nil`
* `get_wield_list()`: returns the name of the inventory list the wielded item
   is in.
* `get_wield_index()`: returns the wield list index of the wielded item (starting with 1)
* `get_wielded_item()`: returns a copy of the wielded item as an `ItemStack`
* `set_wielded_item(item)`: replaces the wielded item, returns `true` if
  successful.
* `get_armor_groups()`:
    * returns a table with all of the object's armor group ratings
    * syntax: the table keys are the armor group names,
      the table values are the corresponding group ratings
    * see section '`ObjectRef` armor groups' for details
* `set_armor_groups({group1=rating, group2=rating, ...})`
    * sets the object's full list of armor groups
    * same table syntax as for `get_armor_groups`
    * note: all armor groups not in the table will be removed
* `set_animation(frame_range, frame_speed, frame_blend, frame_loop)`
    * Sets the object animation parameters and (re)starts the animation
    * Animations only work with a `"mesh"` visual
    * `frame_range`: Beginning and end frame (as specified in the mesh file).
       * Syntax: `{x=start_frame, y=end_frame}`
       * Animation interpolates towards the end frame but stops when it is reached
       * If looped, there is no interpolation back to the start frame
       * If looped, the model should look identical at start and end
       * default: `{x=1.0, y=1.0}`
    * `frame_speed`: How fast the animation plays, in frames per second (number)
       * default: `15.0`
    * `frame_blend`: number, default: `0.0`
    * `frame_loop`: If `true`, animation will loop. If false, it will play once
       * default: `true`
* `get_animation()`: returns current animation parameters set by `set_animation`:
    * `frame_range`, `frame_speed`, `frame_blend`, `frame_loop`.
* `set_animation_frame_speed(frame_speed)`
    * Sets the frame speed of the object's animation
    * Unlike `set_animation`, this will not restart the animation
    * `frame_speed`: See `set_animation`
* `set_attach(parent[, bone, position, rotation, forced_visible])`
    * Attaches object to `parent`
    * See 'Attachments' section for details
    * `parent`: `ObjectRef` to attach to
    * `bone`: Bone to attach to. Default is `""` (the root bone)
    * `position`: relative position, default `{x=0, y=0, z=0}`
    * `rotation`: relative rotation in degrees, default `{x=0, y=0, z=0}`
    * `forced_visible`: Boolean to control whether the attached entity
       should appear in first person, default `false`.
    * This command may fail silently (do nothing) when it would result
      in circular attachments.
* `get_attach()`:
    * returns current attachment parameters or nil if it isn't attached
    * If attached, returns `parent`, `bone`, `position`, `rotation`, `forced_visible`
* `get_children()`: returns a list of ObjectRefs that are attached to the
    object.
* `set_detach()`: Detaches object. No-op if object was not attached.
* `set_bone_position([bone, position, rotation])`
    * Sets absolute bone overrides, e.g. it is equivalent to
      ```lua
      obj:set_bone_override(bone, {
          position = {vec = position, absolute = true},
          rotation = {vec = rotation:apply(math.rad), absolute = true}
      })
      ```
    * **Note:** Rotation is in degrees, not radians.
    * **Deprecated:** Use `set_bone_override` instead.
* `get_bone_position(bone)`: returns the previously set position and rotation of the bone
    * Shorthand for `get_bone_override(bone).position.vec, get_bone_override(bone).rotation.vec:apply(math.deg)`.
    * **Note:** Returned rotation is in degrees, not radians.
    * **Deprecated:** Use `get_bone_override` instead.
* `set_bone_override(bone, override)`
    * `bone`: string
    * `override`: `{ position = property, rotation = property, scale = property }` or `nil`
    * `override = nil` (including omission) is shorthand for `override = {}` which clears the override
    * Each `property` is a table of the form
      `{ vec = vector, interpolation = 0, absolute = false }` or `nil`
        * `vec` is in the same coordinate system as the model, and in radians for rotation.
          It defaults to `vector.zero()` for translation and rotation and `vector.new(1, 1, 1)` for scale.
        * `interpolation`: The old and new overrides are interpolated over this timeframe (in seconds).
        * `absolute`: If set to `false` (which is the default),
          the override will be relative to the animated property:
            * Translation in the case of `position`;
            * Composition in the case of `rotation`;
            * Per-axis multiplication in the case of `scale`
    * `property = nil` is equivalent to no override on that property
    * **Note:** Unlike `set_bone_position`, the rotation is in radians, not degrees.
    * Compatibility note: Clients prior to 5.9.0 only support absolute position and rotation.
      All values are treated as absolute and are set immediately (no interpolation).
* `get_bone_override(bone)`: returns `override` in the above format
    * **Note:** Unlike `get_bone_position`, the returned rotation is in radians, not degrees.
* `get_bone_overrides()`: returns all bone overrides as table `{[bonename] = override, ...}`
* `set_properties(object property table)`
* `get_properties()`: returns a table of all object properties
* `set_observers(observers)`: sets observers (players this object is sent to)
    * If `observers` is `nil`, the object's observers are "unmanaged":
      The object is sent to all players as governed by server settings. This is the default.
    * `observers` is a "set" of player names: `{name1 = true, name2 = true, ...}`
        * A set is a table where the keys are the elements of the set
          (in this case, *valid* player names) and the values are all `true`.
    * Attachments: The *effective observers* of an object are made up of
      all players who can observe the object *and* are also effective observers
      of its parent object (if there is one).
    * Players are automatically added to their own observer sets.
      Players **must** effectively observe themselves.
    * Object activation and deactivation are unaffected by observability.
    * Attached sounds do not work correctly and thus should not be used
      on objects with managed observers yet.
* `get_observers()`:
    * throws an error if the object is invalid
    * returns `nil` if the observers are unmanaged
    * returns a table with all observer names as keys and `true` values (a "set") otherwise
* `get_effective_observers()`:
    * Like `get_observers()`, but returns the "effective" observers, taking into account attachments
    * Time complexity: O(nm)
        * n: number of observers of the involved entities
        * m: number of ancestors along the attachment chain
* `is_player()`: returns true for players, false otherwise
* `get_nametag_attributes()`
    * returns a table with the attributes of the nametag of an object
    * a nametag is a HUD text rendered above the object
    * ```lua
      {
          text = "",
          color = {a=0..255, r=0..255, g=0..255, b=0..255},
          bgcolor = {a=0..255, r=0..255, g=0..255, b=0..255},
      }
      ```
* `set_nametag_attributes(attributes)`
    * sets the attributes of the nametag of an object
    * `attributes`:
      ```lua
      {
          text = "My Nametag",
          color = ColorSpec,
          -- ^ Text color
          bgcolor = ColorSpec or false,
          -- ^ Sets background color of nametag
          -- `false` will cause the background to be set automatically based on user settings
          -- Default: false
      }
      ```

#### Lua entity only (no-op for other objects)

* `remove()`: remove object
    * The object is removed after returning from Lua. However the `ObjectRef`
      itself instantly becomes unusable with all further method calls having
      no effect and returning `nil`.
* `set_velocity(vel)`
    * Sets the velocity
    * `vel` is a vector, e.g. `{x=0.0, y=2.3, z=1.0}`
* `set_acceleration(acc)`
    * Sets the acceleration
    * `acc` is a vector
* `get_acceleration()`: returns the acceleration, a vector
* `set_rotation(rot)`
    * Sets the rotation
    * `rot` is a vector (radians). X is pitch (elevation), Y is yaw (heading)
      and Z is roll (bank).
    * Does not reset rotation incurred through `automatic_rotate`.
      Remove & re-add your objects to force a certain rotation.
* `get_rotation()`: returns the rotation, a vector (radians)
* `set_yaw(yaw)`: sets the yaw in radians (heading).
* `get_yaw()`: returns number in radians
* `set_texture_mod(mod)`
    * Set a texture modifier to the base texture, for sprites and meshes.
    * When calling `set_texture_mod` again, the previous one is discarded.
    * `mod` the texture modifier. See [Texture modifiers].
* `get_texture_mod()` returns current texture modifier
* `set_sprite(start_frame, num_frames, framelength, select_x_by_camera)`
    * Specifies and starts a sprite animation
    * Only used by `sprite` and `upright_sprite` visuals
    * Animations iterate along the frame `y` position.
    * `start_frame`: {x=column number, y=row number}, the coordinate of the
      first frame, default: `{x=0, y=0}`
    * `num_frames`: Total frames in the texture, default: `1`
    * `framelength`: Time per animated frame in seconds, default: `0.2`
    * `select_x_by_camera`: Only for visual = `sprite`. Changes the frame `x`
      position according to the view direction. default: `false`.
        * First column:  subject facing the camera
        * Second column: subject looking to the left
        * Third column:  subject backing the camera
        * Fourth column: subject looking to the right
        * Fifth column:  subject viewed from above
        * Sixth column:  subject viewed from below
* `get_luaentity()`:
    * Returns the object's associated luaentity table, if there is one
    * Otherwise returns `nil` (e.g. for players)
* `get_entity_name()`:
    * **Deprecated**: Will be removed in a future version,
      use `:get_luaentity().name` instead.

#### Player only (no-op for other objects)

* `get_player_name()`: Returns player name or `""` if is not a player
* `get_player_velocity()`: **DEPRECATED**, use get_velocity() instead.
  table {x, y, z} representing the player's instantaneous velocity in nodes/s
* `add_player_velocity(vel)`: **DEPRECATED**, use add_velocity(vel) instead.
* `get_look_dir()`: get camera direction as a unit vector
* `get_look_vertical()`: pitch in radians
    * Angle ranges between -pi/2 and pi/2, which are straight up and down
      respectively.
* `get_look_horizontal()`: yaw in radians
    * Angle is counter-clockwise from the +z direction.
* `set_look_vertical(radians)`: sets look pitch
    * radians: Angle from looking forward, where positive is downwards.
* `set_look_horizontal(radians)`: sets look yaw
    * radians: Angle from the +z direction, where positive is counter-clockwise.
* `get_look_pitch()`: pitch in radians - Deprecated as broken. Use
  `get_look_vertical`.
    * Angle ranges between -pi/2 and pi/2, which are straight down and up
      respectively.
* `get_look_yaw()`: yaw in radians - Deprecated as broken. Use
  `get_look_horizontal`.
    * Angle is counter-clockwise from the +x direction.
* `set_look_pitch(radians)`: sets look pitch - Deprecated. Use
  `set_look_vertical`.
* `set_look_yaw(radians)`: sets look yaw - Deprecated. Use
  `set_look_horizontal`.
* `get_breath()`: returns player's breath
* `set_breath(value)`: sets player's breath
    * values:
        * `0`: player is drowning
        * max: bubbles bar is not shown
        * See [Object properties] for more information
    * Is limited to range 0 ... 65535 (2^16 - 1)
* `set_fov(fov, is_multiplier, transition_time)`: Sets player's FOV
    * `fov`: Field of View (FOV) value.
    * `is_multiplier`: Set to `true` if the FOV value is a multiplier.
      Defaults to `false`.
    * `transition_time`: If defined, enables smooth FOV transition.
      Interpreted as the time (in seconds) to reach target FOV.
      If set to 0, FOV change is instantaneous. Defaults to 0.
    * Set `fov` to 0 to clear FOV override.
* `get_fov()`: Returns the following:
    * Server-sent FOV value. Returns 0 if an FOV override doesn't exist.
    * Boolean indicating whether the FOV value is a multiplier.
    * Time (in seconds) taken for the FOV transition. Set by `set_fov`.
* `set_attribute(attribute, value)`:  DEPRECATED, use get_meta() instead
    * Sets an extra attribute with value on player.
    * `value` must be a string, or a number which will be converted to a
      string.
    * If `value` is `nil`, remove attribute from player.
* `get_attribute(attribute)`:  DEPRECATED, use get_meta() instead
    * Returns value (a string) for extra attribute.
    * Returns `nil` if no attribute found.
* `get_meta()`: Returns metadata associated with the player (a PlayerMetaRef).
* `set_inventory_formspec(formspec)`
    * Redefine player's inventory form
    * Should usually be called in `on_joinplayer`
    * If `formspec` is `""`, the player's inventory is disabled.
* `get_inventory_formspec()`: returns a formspec string
* `set_formspec_prepend(formspec)`:
    * the formspec string will be added to every formspec shown to the user,
      except for those with a no_prepend[] tag.
    * This should be used to set style elements such as background[] and
      bgcolor[], any non-style elements (eg: label) may result in weird behavior.
    * Only affects formspecs shown after this is called.
* `get_formspec_prepend()`: returns a formspec string.
* `get_player_control()`: returns table with player input
    * The table contains the following boolean fields representing the pressed
      keys: `up`, `down`, `left`, `right`, `jump`, `aux1`, `sneak`, `dig`,
      `place`, `LMB`, `RMB` and `zoom`.
    * The fields `LMB` and `RMB` are equal to `dig` and `place` respectively,
      and exist only to preserve backwards compatibility.
    * The table also contains the fields `movement_x` and `movement_y`.
        * They represent the movement of the player. Values are numbers in the
          range [-1.0,+1.0].
        * They take both keyboard and joystick input into account.
        * You should prefer them over `up`, `down`, `left` and `right` to
          support different input methods correctly.
    * Returns an empty table `{}` if the object is not a player.
* `get_player_control_bits()`: returns integer with bit packed player pressed
  keys.
    * Bits:
        * 0 - up
        * 1 - down
        * 2 - left
        * 3 - right
        * 4 - jump
        * 5 - aux1
        * 6 - sneak
        * 7 - dig
        * 8 - place
        * 9 - zoom
    * Returns `0` (no bits set) if the object is not a player.
* `set_physics_override(override_table)`
    * Overrides the physics attributes of the player
    * `override_table` is a table with the following fields:
        * `speed`: multiplier to *all* movement speed (`speed_*`) and
                   acceleration (`acceleration_*`) values (default: `1`)
        * `speed_walk`: multiplier to default walk speed value (default: `1`)
            * Note: The actual walk speed is the product of `speed` and `speed_walk`
        * `speed_climb`: multiplier to default climb speed value (default: `1`)
            * Note: The actual climb speed is the product of `speed` and `speed_climb`
        * `speed_crouch`: multiplier to default sneak speed value (default: `1`)
            * Note: The actual sneak speed is the product of `speed` and `speed_crouch`
        * `speed_fast`: multiplier to default speed value in Fast Mode (default: `1`)
            * Note: The actual fast speed is the product of `speed` and `speed_fast`
        * `jump`: multiplier to default jump value (default: `1`)
        * `gravity`: multiplier to default gravity value (default: `1`)
        * `liquid_fluidity`: multiplier to liquid movement resistance value
          (for nodes with `liquid_move_physics`); the higher this value, the lower the
          resistance to movement. At `math.huge`, the resistance is zero and you can
          move through any liquid like air. (default: `1`)
            * Warning: Values below 1 are currently unsupported.
        * `liquid_fluidity_smooth`: multiplier to default maximum liquid resistance value
          (for nodes with `liquid_move_physics`); controls deceleration when entering
          node at high speed. At higher values you come to a halt more quickly
          (default: `1`)
        * `liquid_sink`: multiplier to default liquid sinking speed value;
          (for nodes with `liquid_move_physics`) (default: `1`)
        * `acceleration_default`: multiplier to horizontal and vertical acceleration
          on ground or when climbing (default: `1`)
            * Note: The actual acceleration is the product of `speed` and `acceleration_default`
        * `acceleration_air`: multiplier to acceleration
          when jumping or falling (default: `1`)
            * Note: The actual acceleration is the product of `speed` and `acceleration_air`
        * `acceleration_fast`: multiplier to acceleration in Fast Mode (default: `1`)
            * Note: The actual acceleration is the product of `speed` and `acceleration_fast`
        * `sneak`: whether player can sneak (default: `true`)
        * `sneak_glitch`: whether player can use the new move code replications
          of the old sneak side-effects: sneak ladders and 2 node sneak jump
          (default: `false`)
        * `new_move`: use new move/sneak code. When `false` the exact old code
          is used for the specific old sneak behavior (default: `true`)
    * Note: All numeric fields above modify a corresponding `movement_*` setting.
    * For games, we recommend for simpler code to first modify the `movement_*`
      settings (e.g. via the game's `minetest.conf`) to set a global base value
      for all players and only use `set_physics_override` when you need to change
      from the base value on a per-player basis
    * Note: Some of the fields don't exist in old API versions, see feature
      `physics_overrides_v2`.

* `get_physics_override()`: returns the table given to `set_physics_override`
* `hud_add(hud definition)`: add a HUD element described by HUD def, returns ID
   number on success
* `hud_remove(id)`: remove the HUD element of the specified id
* `hud_change(id, stat, value)`: change a value of a previously added HUD
  element.
    * `stat` supports the same keys as in the hud definition table except for
      `"type"` (or the deprecated `"hud_elem_type"`).
* `hud_get(id)`: gets the HUD element definition structure of the specified ID
* `hud_get_all()`:
    * Returns a table in the form `{ [id] = HUD definition, [id] = ... }`.
    * A mod should keep track of its introduced IDs and only use this to access foreign elements.
    * It is discouraged to change foreign HUD elements.
* `hud_set_flags(flags)`: sets specified HUD flags of player.
    * `flags`: A table with the following fields set to boolean values
        * `hotbar`
        * `healthbar`
        * `crosshair`
        * `wielditem`
        * `breathbar`
        * `minimap`: Modifies the client's permission to view the minimap.
          The client may locally elect to not view the minimap.
        * `minimap_radar`: is only usable when `minimap` is true
        * `basic_debug`: Allow showing basic debug info that might give a gameplay advantage.
          This includes map seed, player position, look direction, the pointed node and block bounds.
          Does not affect players with the `debug` privilege.
        * `chat`: Modifies the client's permission to view chat on the HUD.
          The client may locally elect to not view chat. Does not affect the console.
    * If a flag equals `nil`, the flag is not modified
* `hud_get_flags()`: returns a table of player HUD flags with boolean values.
    * See `hud_set_flags` for a list of flags that can be toggled.
* `hud_set_hotbar_itemcount(count)`: sets number of items in builtin hotbar
    * `count`: number of items, must be between `1` and `32`
    * If `count` exceeds the `"main"` list size, the list size will be used instead.
* `hud_get_hotbar_itemcount()`: returns number of visible items
    * This value is also clamped by the `"main"` list size.
* `hud_set_hotbar_image(texturename)`
    * sets background image for hotbar
* `hud_get_hotbar_image()`: returns texturename
* `hud_set_hotbar_selected_image(texturename)`
    * sets image for selected item of hotbar
* `hud_get_hotbar_selected_image()`: returns texturename
* `set_minimap_modes({mode, mode, ...}, selected_mode)`
    * Overrides the available minimap modes (and toggle order), and changes the
    selected mode.
    * `mode` is a table consisting of up to four fields:
        * `type`: Available type:
            * `off`: Minimap off
            * `surface`: Minimap in surface mode
            * `radar`: Minimap in radar mode
            * `texture`: Texture to be displayed instead of terrain map
              (texture is centered around 0,0 and can be scaled).
              Texture size is limited to 512 x 512 pixel.
        * `label`: Optional label to display on minimap mode toggle
          The translation must be handled within the mod.
        * `size`: Sidelength or diameter, in number of nodes, of the terrain
          displayed in minimap
        * `texture`: Only for texture type, name of the texture to display
        * `scale`: Only for texture type, scale of the texture map in nodes per
          pixel (for example a `scale` of 2 means each pixel represents a 2x2
          nodes square)
    * `selected_mode` is the mode index to be selected after modes have been changed
    (0 is the first mode).
* `set_sky(sky_parameters)`
    * The presence of the function `set_sun`, `set_moon` or `set_stars` indicates
      whether `set_sky` accepts this format. Check the legacy format otherwise.
    * Passing no arguments resets the sky to its default values.
    * `sky_parameters` is a table with the following optional fields:
        * `base_color`: ColorSpec, meaning depends on `type` (default: `#ffffff`)
        * `body_orbit_tilt`: Float, rotation angle of sun/moon orbit in degrees.
           By default, orbit is controlled by a client-side setting, and this field is not set.
           After a value is assigned, it can only be changed to another float value.
           Valid range [-60.0,60.0] (default: not set)
        * `type`: Available types:
            * `"regular"`: Uses 0 textures, `base_color` ignored
            * `"skybox"`: Uses 6 textures, `base_color` used as fog.
            * `"plain"`: Uses 0 textures, `base_color` used as both fog and sky.
            (default: `"regular"`)
        * `textures`: A table containing up to six textures in the following
            order: Y+ (top), Y- (bottom), X+ (east), X- (west), Z- (south), Z+ (north).
            The top and bottom textures are oriented in-line with the east (X+) face (the top edge of the
            bottom texture and the bottom edge of the top texture touch the east face).
            Some top and bottom textures expect to be aligned with the north face and will need to be rotated
            by -90 and 90 degrees, respectively, to fit the eastward orientation.
        * `clouds`: Boolean for whether clouds appear. (default: `true`)
        * `sky_color`: A table used in `"regular"` type only, containing the
          following values (alpha is ignored):
            * `day_sky`: ColorSpec, for the top half of the sky during the day.
              (default: `#61b5f5`)
            * `day_horizon`: ColorSpec, for the bottom half of the sky during the day.
              (default: `#90d3f6`)
            * `dawn_sky`: ColorSpec, for the top half of the sky during dawn/sunset.
              (default: `#b4bafa`)
              The resulting sky color will be a darkened version of the ColorSpec.
              Warning: The darkening of the ColorSpec is subject to change.
            * `dawn_horizon`: ColorSpec, for the bottom half of the sky during dawn/sunset.
              (default: `#bac1f0`)
              The resulting sky color will be a darkened version of the ColorSpec.
              Warning: The darkening of the ColorSpec is subject to change.
            * `night_sky`: ColorSpec, for the top half of the sky during the night.
              (default: `#006bff`)
              The resulting sky color will be a dark version of the ColorSpec.
              Warning: The darkening of the ColorSpec is subject to change.
            * `night_horizon`: ColorSpec, for the bottom half of the sky during the night.
              (default: `#4090ff`)
              The resulting sky color will be a dark version of the ColorSpec.
              Warning: The darkening of the ColorSpec is subject to change.
            * `indoors`: ColorSpec, for when you're either indoors or underground.
              (default: `#646464`)
            * `fog_sun_tint`: ColorSpec, changes the fog tinting for the sun
              at sunrise and sunset. (default: `#f47d1d`)
            * `fog_moon_tint`: ColorSpec, changes the fog tinting for the moon
              at sunrise and sunset. (default: `#7f99cc`)
            * `fog_tint_type`: string, changes which mode the directional fog
                abides by, `"custom"` uses `sun_tint` and `moon_tint`, while
                `"default"` uses the classic Luanti sun and moon tinting.
                Will use tonemaps, if set to `"default"`. (default: `"default"`)
        * `fog`: A table with following optional fields:
            * `fog_distance`: integer, set an upper bound for the client's viewing_range.
               Any value >= 0 sets the desired upper bound for viewing_range,
               disables range_all and prevents disabling fog (F3 key by default).
               Any value < 0 resets the behavior to being client-controlled.
               (default: -1)
            * `fog_start`: float, override the client's fog_start.
               Fraction of the visible distance at which fog starts to be rendered.
               Any value between [0.0, 0.99] set the fog_start as a fraction of the viewing_range.
               Any value < 0, resets the behavior to being client-controlled.
               (default: -1)
            * `fog_color`: ColorSpec, override the color of the fog.
               Unlike `base_color` above this will apply regardless of the skybox type.
               (default: `"#00000000"`, which means no override)
* `set_sky(base_color, type, {texture names}, clouds)`
    * Deprecated. Use `set_sky(sky_parameters)`
    * `base_color`: ColorSpec, defaults to white
    * `type`: Available types:
        * `"regular"`: Uses 0 textures, `bgcolor` ignored
        * `"skybox"`: Uses 6 textures, `bgcolor` used
        * `"plain"`: Uses 0 textures, `bgcolor` used
    * `clouds`: Boolean for whether clouds appear in front of `"skybox"` or
      `"plain"` custom skyboxes (default: `true`)
* `get_sky(as_table)`:
    * `as_table`: boolean that determines whether the deprecated version of this
    function is being used.
        * `true` returns a table containing sky parameters as defined in `set_sky(sky_parameters)`.
        * Deprecated: `false` or `nil` returns base_color, type, table of textures,
        clouds.
* `get_sky_color()`:
    * Deprecated: Use `get_sky(as_table)` instead.
    * returns a table with the `sky_color` parameters as in `set_sky`.
* `set_sun(sun_parameters)`:
    * Passing no arguments resets the sun to its default values.
    * `sun_parameters` is a table with the following optional fields:
        * `visible`: Boolean for whether the sun is visible.
            (default: `true`)
        * `texture`: A regular texture for the sun. Setting to `""`
            will re-enable the mesh sun. (default: "sun.png", if it exists)
            The texture appears non-rotated at sunrise and rotated 180 degrees
            (upside down) at sunset.
        * `tonemap`: A 512x1 texture containing the tonemap for the sun
            (default: `"sun_tonemap.png"`)
        * `sunrise`: A regular texture for the sunrise texture.
            (default: `"sunrisebg.png"`)
        * `sunrise_visible`: Boolean for whether the sunrise texture is visible.
            (default: `true`)
        * `scale`: Float controlling the overall size of the sun. (default: `1`)
            Note: For legacy reasons, the sun is bigger than the moon by a factor
            of about `1.57` for equal `scale` values.
* `get_sun()`: returns a table with the current sun parameters as in
    `set_sun`.
* `set_moon(moon_parameters)`:
    * Passing no arguments resets the moon to its default values.
    * `moon_parameters` is a table with the following optional fields:
        * `visible`: Boolean for whether the moon is visible.
            (default: `true`)
        * `texture`: A regular texture for the moon. Setting to `""`
            will re-enable the mesh moon. (default: `"moon.png"`, if it exists)
            The texture appears non-rotated at sunrise / moonset and rotated 180
            degrees (upside down) at sunset / moonrise.
            Note: Relative to the sun, the moon texture is hence rotated by 180°.
            You can use the `^[transformR180` texture modifier to achieve the same orientation.
        * `tonemap`: A 512x1 texture containing the tonemap for the moon
            (default: `"moon_tonemap.png"`)
        * `scale`: Float controlling the overall size of the moon (default: `1`)
            Note: For legacy reasons, the sun is bigger than the moon by a factor
            of about `1.57` for equal `scale` values.
* `get_moon()`: returns a table with the current moon parameters as in
    `set_moon`.
* `set_stars(star_parameters)`:
    * Passing no arguments resets stars to their default values.
    * `star_parameters` is a table with the following optional fields:
        * `visible`: Boolean for whether the stars are visible.
            (default: `true`)
        * `day_opacity`: Float for maximum opacity of stars at day.
            No effect if `visible` is false.
            (default: 0.0; maximum: 1.0; minimum: 0.0)
        * `count`: Integer number to set the number of stars in
            the skybox. Only applies to `"skybox"` and `"regular"` sky types.
            (default: `1000`)
        * `star_color`: ColorSpec, sets the colors of the stars,
            alpha channel is used to set overall star brightness.
            (default: `#ebebff69`)
        * `scale`: Float controlling the overall size of the stars (default: `1`)
* `get_stars()`: returns a table with the current stars parameters as in
    `set_stars`.
* `set_clouds(cloud_parameters)`: set cloud parameters
    * Passing no arguments resets clouds to their default values.
    * `cloud_parameters` is a table with the following optional fields:
        * `density`: from `0` (no clouds) to `1` (full clouds) (default `0.4`)
        * `color`: basic cloud color with alpha channel, ColorSpec
          (default `#fff0f0e5`).
        * `ambient`: cloud color lower bound, use for a "glow at night" effect.
          ColorSpec (alpha ignored, default `#000000`)
        * `height`: cloud height, i.e. y of cloud base (default per conf,
          usually `120`)
        * `thickness`: cloud thickness in nodes (default `16`).
          if set to zero the clouds are rendered flat.
        * `speed`: 2D cloud speed + direction in nodes per second
          (default `{x=0, z=-2}`).
        * `shadow`: shadow color, applied to the base of the cloud
          (default `#cccccc`).
* `get_clouds()`: returns a table with the current cloud parameters as in
  `set_clouds`.
* `override_day_night_ratio(ratio or nil)`
    * `0`...`1`: Overrides day-night ratio, controlling sunlight to a specific
      amount.
    * Passing no arguments disables override, defaulting to sunlight based on day-night cycle
    * See also `core.time_to_day_night_ratio`,
* `get_day_night_ratio()`: returns the ratio or nil if it isn't overridden
* `set_local_animation(idle, walk, dig, walk_while_dig, frame_speed)`:
  set animation for player model in third person view.
    * Every animation equals to a `{x=starting frame, y=ending frame}` table.
    * `frame_speed` sets the animations frame speed. Default is 30.
* `get_local_animation()`: returns idle, walk, dig, walk_while_dig tables and
  `frame_speed`.
* `set_eye_offset([firstperson, thirdperson_back, thirdperson_front])`: Sets camera offset vectors.
    * `firstperson`: Offset in first person view.
      Defaults to `vector.zero()` if unspecified.
    * `thirdperson_back`: Offset in third person back view.
      Clamped between `vector.new(-10, -10, -5)` and `vector.new(10, 15, 5)`.
      Defaults to `vector.zero()` if unspecified.
    * `thirdperson_front`: Offset in third person front view.
      Same limits as for `thirdperson_back` apply.
      Defaults to `thirdperson_back` if unspecified.
* `get_eye_offset()`: Returns camera offset vectors as set via `set_eye_offset`.
* `send_mapblock(blockpos)`:
    * Sends an already loaded mapblock to the player.
    * Returns `false` if nothing was sent (note that this can also mean that
      the client already has the block)
    * Resource intensive - use sparsely
* `set_lighting(light_definition)`: sets lighting for the player
    * Passing no arguments resets lighting to its default values.
    * `light_definition` is a table with the following optional fields:
      * `saturation` sets the saturation (vividness; default: `1.0`).
        * It is applied according to the function `result = b*(1-s) + c*s`, where:
          * `c` is the original color
          * `b` is the greyscale version of the color with the same luma
          * `s` is the saturation set here
        * The resulting color always has the same luma (perceived brightness) as the original.
        * This means that:
          * values > 1 oversaturate
          * values < 1 down to 0 desaturate, 0 being entirely greyscale
          * values < 0 cause an effect similar to inversion,
            but keeping original luma and being symmetrical in terms of saturation
            (eg. -1 and 1 is the same saturation and luma, but different hues)
        * This value has no effect on clients who have shaders or post-processing disabled.
      * `shadows` is a table that controls ambient shadows
        * This has no effect on clients who have the "Dynamic Shadows" effect disabled.
        * `intensity` sets the intensity of the shadows from 0 (no shadows, default) to 1 (blackness)
        * `tint` tints the shadows with the provided color, with RGB values ranging from 0 to 255.
          (default `{r=0, g=0, b=0}`)
      * `exposure` is a table that controls automatic exposure.
        The basic exposure factor equation is `e = 2^exposure_correction / clamp(luminance, 2^luminance_min, 2^luminance_max)`
        * This has no effect on clients who have the "Automatic Exposure" effect disabled.
        * `luminance_min` set the lower luminance boundary to use in the calculation (default: `-3.0`)
        * `luminance_max` set the upper luminance boundary to use in the calculation (default: `-3.0`)
        * `exposure_correction` correct observed exposure by the given EV value (default: `0.0`)
        * `speed_dark_bright` set the speed of adapting to bright light (default: `1000.0`)
        * `speed_bright_dark` set the speed of adapting to dark scene (default: `1000.0`)
        * `center_weight_power` set the power factor for center-weighted luminance measurement (default: `1.0`)
      * `bloom` is a table that controls bloom.
        * This has no effect on clients with protocol version < 46 or clients who
          have the "Bloom" effect disabled.
        * `intensity` defines much bloom is applied to the rendered image.
          * Recommended range: from 0.0 to 1.0, default: 0.05
          * If set to 0, bloom is disabled.
          * The default value is to be changed from 0.05 to 0 in the future.
            If you wish to keep the current default value, you should set it
            explicitly.
        * `strength_factor` defines the magnitude of bloom overexposure.
          * Recommended range: from 0.1 to 10.0, default: 1.0
        * `radius` is a logical value that controls how far the bloom effect
          spreads from the bright objects.
          * Recommended range: from 0.1 to 8.0, default: 1.0
        * The behavior of values outside the recommended range is unspecified.
      * `volumetric_light`: is a table that controls volumetric light (a.k.a. "godrays")
        * This has no effect on clients who have the "Volumetric Lighting" or "Bloom" effects disabled.
        * `strength`: sets the strength of the volumetric light effect from 0 (off, default) to 1 (strongest).
            * `0.2` is a reasonable standard value.
            * Currently, bloom `intensity` and `strength_factor` affect volumetric
              lighting `strength` and vice versa. This behavior is to be changed
              in the future, do not rely on it.

* `get_lighting()`: returns the current state of lighting for the player.
    * Result is a table with the same fields as `light_definition` in `set_lighting`.
* `respawn()`: Respawns the player using the same mechanism as the death screen,
  including calling `on_respawnplayer` callbacks.
* `get_flags()`: returns a table of player flags (the following boolean fields):
  * `breathing`: Whether breathing (regaining air) is enabled, default `true`.
  * `drowning`: Whether drowning (losing air) is enabled, default `true`.
  * `node_damage`: Whether the player takes damage from nodes, default `true`.
* `set_flags(flags)`: sets flags
  * takes a table in the same format as returned by `get_flags`
  * absent fields are left unchanged
