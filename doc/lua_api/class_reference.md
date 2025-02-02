Class reference
===============

Sorted alphabetically.

`AreaStore`
-----------

AreaStore is a data structure to calculate intersections of 3D cuboid volumes
and points. The `data` field (string) may be used to store and retrieve any
mod-relevant information to the specified area.

Despite its name, mods must take care of persisting AreaStore data. They may
use the provided load and write functions for this.


### Methods

* `AreaStore(type_name)`
    * Returns a new AreaStore instance
    * `type_name`: optional, forces the internally used API.
        * Possible values: `"LibSpatial"` (default).
        * When other values are specified, or SpatialIndex is not available,
          the custom Luanti functions are used.
* `get_area(id, include_corners, include_data)`
    * Returns the area information about the specified ID.
    * Returned values are either of these:
      ```lua
      nil  -- Area not found
      true -- Without `include_corners` and `include_data`
      {
          min = pos, max = pos -- `include_corners == true`
          data = string        -- `include_data == true`
      }
      ```

* `get_areas_for_pos(pos, include_corners, include_data)`
    * Returns all areas as table, indexed by the area ID.
    * Table values: see `get_area`.
* `get_areas_in_area(corner1, corner2, accept_overlap, include_corners, include_data)`
    * Returns all areas that contain all nodes inside the area specified by`
      `corner1 and `corner2` (inclusive).
    * `accept_overlap`: if `true`, areas are returned that have nodes in
      common (intersect) with the specified area.
    * Returns the same values as `get_areas_for_pos`.
* `insert_area(corner1, corner2, data, [id])`: inserts an area into the store.
    * Returns the new area's ID, or nil if the insertion failed.
    * The (inclusive) positions `corner1` and `corner2` describe the area.
    * `data` is a string stored with the area.
    * `id` (optional): will be used as the internal area ID if it is a unique
      number between 0 and 2^32-2.
* `reserve(count)`
    * Requires SpatialIndex, no-op function otherwise.
    * Reserves resources for `count` many contained areas to improve
      efficiency when working with many area entries. Additional areas can still
      be inserted afterwards at the usual complexity.
* `remove_area(id)`: removes the area with the given id from the store, returns
  success.
* `set_cache_params(params)`: sets params for the included prefiltering cache.
  Calling invalidates the cache, so that its elements have to be newly
  generated.
    * `params` is a table with the following fields:
      ```lua
      {
          enabled = boolean,   -- Whether to enable, default true
          block_radius = int,  -- The radius (in nodes) of the areas the cache
                               -- generates prefiltered lists for, minimum 16,
                               -- default 64
          limit = int,         -- The cache size, minimum 20, default 1000
      }
      ```
* `to_string()`: Experimental. Returns area store serialized as a (binary)
  string.
* `to_file(filename)`: Experimental. Like `to_string()`, but writes the data to
  a file.
* `from_string(str)`: Experimental. Deserializes string and loads it into the
  AreaStore.
  Returns success and, optionally, an error message.
* `from_file(filename)`: Experimental. Like `from_string()`, but reads the data
  from a file.

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

`ItemStack`
-----------

An `ItemStack` is a stack of items.

It can be created via `ItemStack(x)`, where x is an `ItemStack`,
an itemstring, a table or `nil`.

### Methods

* `is_empty()`: returns `true` if stack is empty.
* `get_name()`: returns item name (e.g. `"default:stone"`).
* `set_name(item_name)`: returns a boolean indicating whether the item was
  cleared.
* `get_count()`: Returns number of items on the stack.
* `set_count(count)`: returns a boolean indicating whether the item was cleared
    * `count`: number, unsigned 16 bit integer
* `get_wear()`: returns tool wear (`0`-`65535`), `0` for non-tools.
* `set_wear(wear)`: returns boolean indicating whether item was cleared
    * `wear`: number, unsigned 16 bit integer
* `get_meta()`: returns ItemStackMetaRef. See section for more details
* `get_metadata()`: **Deprecated.** Returns metadata (a string attached to an item stack).
    * If you need to access this to maintain backwards compatibility,
      use `stack:get_meta():get_string("")` instead.
* `set_metadata(metadata)`: **Deprecated.** Returns true.
    * If you need to set this to maintain backwards compatibility,
      use `stack:get_meta():set_string("", metadata)` instead.
* `get_description()`: returns the description shown in inventory list tooltips.
    * The engine uses this when showing item descriptions in tooltips.
    * Fields for finding the description, in order:
        * `description` in item metadata (See [Item Metadata].)
        * `description` in item definition
        * item name
* `get_short_description()`: returns the short description or nil.
    * Unlike the description, this does not include new lines.
    * Fields for finding the short description, in order:
        * `short_description` in item metadata (See [Item Metadata].)
        * `short_description` in item definition
        * first line of the description (From item meta or def, see `get_description()`.)
        * Returns nil if none of the above are set
* `clear()`: removes all items from the stack, making it empty.
* `replace(item)`: replace the contents of this stack.
    * `item` can also be an itemstring or table.
* `to_string()`: returns the stack in itemstring form.
* `to_table()`: returns the stack in Lua table form.
* `get_stack_max()`: returns the maximum size of the stack (depends on the
  item).
* `get_free_space()`: returns `get_stack_max() - get_count()`.
* `is_known()`: returns `true` if the item name refers to a defined item type.
* `get_definition()`: returns the item definition table.
* `get_tool_capabilities()`: returns the digging properties of the item,
  or those of the hand if none are defined for this item type
* `add_wear(amount)`
    * Increases wear by `amount` if the item is a tool, otherwise does nothing
    * Valid `amount` range is [0,65536]
    * `amount`: number, integer
* `add_wear_by_uses(max_uses)`
    * Increases wear in such a way that, if only this function is called,
      the item breaks after `max_uses` times
    * Valid `max_uses` range is [0,65536]
    * Does nothing if item is not a tool or if `max_uses` is 0
* `get_wear_bar_params()`: returns the wear bar parameters of the item,
  or nil if none are defined for this item type or in the stack's meta
* `add_item(item)`: returns leftover `ItemStack`
    * Put some item or stack onto this stack
* `item_fits(item)`: returns `true` if item or stack can be fully added to
  this one.
* `take_item(n)`: returns taken `ItemStack`
    * Take (and remove) up to `n` items from this stack
    * `n`: number, default: `1`
* `peek_item(n)`: returns taken `ItemStack`
    * Copy (don't remove) up to `n` items from this stack
    * `n`: number, default: `1`
* `equals(other)`:
    * returns `true` if this stack is identical to `other`.
    * Note: `stack1:to_string() == stack2:to_string()` is not reliable,
      as stack metadata can be serialized in arbitrary order.
    * Note: if `other` is an itemstring or table representation of an
      ItemStack, this will always return false, even if it is
      "equivalent".

### Operators

* `stack1 == stack2`:
    * Returns whether `stack1` and `stack2` are identical.
    * Note: `stack1:to_string() == stack2:to_string()` is not reliable,
      as stack metadata can be serialized in arbitrary order.
    * Note: if `stack2` is an itemstring or table representation of an
      ItemStack, this will always return false, even if it is
      "equivalent".

`ItemStackMetaRef`
------------------

ItemStack metadata: reference extra data and functionality stored in a stack.
Can be obtained via `item:get_meta()`.

### Methods

* All methods in MetaDataRef
* `set_tool_capabilities([tool_capabilities])`
    * Overrides the item's tool capabilities
    * A nil value will clear the override data and restore the original
      behavior.
* `set_wear_bar_params([wear_bar_params])`
    * Overrides the item's wear bar parameters (see "Wear Bar Color" section)
    * A nil value will clear the override data and restore the original
      behavior.

`MetaDataRef`
-------------

Base class used by [`StorageRef`], [`NodeMetaRef`], [`ItemStackMetaRef`],
and [`PlayerMetaRef`].

Note: If a metadata value is in the format `${k}`, an attempt to get the value
will return the value associated with key `k`. There is a low recursion limit.
This behavior is **deprecated** and will be removed in a future version. Usage
of the `${k}` syntax in formspecs is not deprecated.

### Methods

* `contains(key)`: Returns true if key present, otherwise false.
    * Returns `nil` when the MetaData is inexistent.
* `get(key)`: Returns `nil` if key not present, else the stored string.
* `set_string(key, value)`: Value of `""` will delete the key.
* `get_string(key)`: Returns `""` if key not present.
* `set_int(key, value)`
    * The range for the value is system-dependent (usually 32 bits).
      The value will be converted into a string when stored.
* `get_int(key)`: Returns `0` if key not present.
* `set_float(key, value)`
    * The range for the value is system-dependent (usually 32 bits).
      The value will be converted into a string when stored.
* `get_float(key)`: Returns `0` if key not present.
* `get_keys()`: returns a list of all keys in the metadata.
* `to_table()`:
    * Returns a metadata table (see below) or `nil` on failure.
* `from_table(data)`
    * Imports metadata from a metadata table
    * If `data` is a metadata table (see below), the metadata it represents
      will replace all metadata of this MetaDataRef object
    * Any non-table value for `data` will clear all metadata
    * Item table values the `inventory` field may also be itemstrings
    * Returns `true` on success
* `equals(other)`
    * returns `true` if this metadata has the same key-value pairs as `other`

### Metadata tables

Metadata tables represent MetaDataRef in a Lua table form (see `from_table`/`to_table`).

A metadata table is a table that has the following keys:

* `fields`: key-value storage of metadata fields
    * all values are stored as strings
    * numbers must be converted to strings first
* `inventory` (for NodeMetaRef only): A node inventory in table form
    * inventory table keys are inventory list names
    * inventory table values are item tables
    * item table keys are slot IDs (starting with 1)
    * item table values are ItemStacks

Example:

```lua
metadata_table = {
    -- metadata fields (key/value store)
    fields = {
        infotext = "Container",
        another_key = "Another Value",
    },

    -- inventory data (for nodes)
    inventory = {
        -- inventory list "main" with 4 slots
        main = {
            -- list of all item slots
            [1] = "example:dirt",
            [2] = "example:stone 25",
            [3] = "", -- empty slot
            [4] = "example:pickaxe",
        },
        -- inventory list "hidden" with 1 slot
        hidden = {
            [1] = "example:diamond",
        },
    },
}
```

`ModChannel`
------------

An interface to use mod channels on client and server

### Methods

* `leave()`: leave the mod channel.
    * Server leaves channel `channel_name`.
    * No more incoming or outgoing messages can be sent to this channel from
      server mods.
    * This invalidate all future object usage.
    * Ensure you set mod_channel to nil after that to free Lua resources.
* `is_writeable()`: returns true if channel is writeable and mod can send over
  it.
* `send_all(message)`: Send `message` though the mod channel.
    * If mod channel is not writeable or invalid, message will be dropped.
    * Message size is limited to 65535 characters by protocol.

`NodeMetaRef`
-------------

Node metadata: reference extra data and functionality stored in a node.
Can be obtained via `core.get_meta(pos)`.

### Methods

* All methods in MetaDataRef
* `get_inventory()`: returns `InvRef`
* `mark_as_private(name or {name1, name2, ...})`: Mark specific vars as private
  This will prevent them from being sent to the client. Note that the "private"
  status will only be remembered if an associated key-value pair exists,
  meaning it's best to call this when initializing all other meta (e.g.
  `on_construct`).

`NodeTimerRef`
--------------

Node Timers: a high resolution persistent per-node timer.
Can be gotten via `core.get_node_timer(pos)`.

### Methods

* `set(timeout,elapsed)`
    * set a timer's state
    * `timeout` is in seconds, and supports fractional values (0.1 etc)
    * `elapsed` is in seconds, and supports fractional values (0.1 etc)
    * will trigger the node's `on_timer` function after `(timeout - elapsed)`
      seconds.
* `start(timeout)`
    * start a timer
    * equivalent to `set(timeout,0)`
* `stop()`
    * stops the timer
* `get_timeout()`: returns current timeout in seconds
    * if `timeout` equals `0`, timer is inactive
* `get_elapsed()`: returns current elapsed time in seconds
    * the node's `on_timer` function will be called after `(timeout - elapsed)`
      seconds.
* `is_started()`: returns boolean state of timer
    * returns `true` if timer is started, otherwise `false`

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


`PcgRandom`
-----------

A 32-bit pseudorandom number generator.
Uses PCG32, an algorithm of the permuted congruential generator family,
offering very strong randomness.

* constructor `PcgRandom(seed, [seq])`
  * `seed`: 64-bit unsigned seed
  * `seq`: 64-bit unsigned sequence, optional

### Methods

* `next()`: return next integer random number [`-2147483648`...`2147483647`]
* `next(min, max)`: return next integer random number [`min`...`max`]
* `rand_normal_dist(min, max, num_trials=6)`: return normally distributed
  random number [`min`...`max`].
    * This is only a rough approximation of a normal distribution with:
    * `mean = (max - min) / 2`, and
    * `variance = (((max - min + 1) ^ 2) - 1) / (12 * num_trials)`
    * Increasing `num_trials` improves accuracy of the approximation
* `get_state()`: return generator state encoded in string
* `set_state(state_string)`: restore generator state from encoded string

`PerlinNoise`
-------------

A perlin noise generator.
It can be created via `PerlinNoise()` or `core.get_perlin()`.
For `core.get_perlin()`, the actual seed used is the noiseparams seed
plus the world seed, to create world-specific noise.

`PerlinNoise(noiseparams)`
`PerlinNoise(seed, octaves, persistence, spread)` (Deprecated).

`core.get_perlin(noiseparams)`
`core.get_perlin(seeddiff, octaves, persistence, spread)` (Deprecated).

### Methods

* `get_2d(pos)`: returns 2D noise value at `pos={x=,y=}`
* `get_3d(pos)`: returns 3D noise value at `pos={x=,y=,z=}`

`PerlinNoiseMap`
----------------

A fast, bulk perlin noise generator.

It can be created via `PerlinNoiseMap(noiseparams, size)` or
`core.get_perlin_map(noiseparams, size)`.
For `core.get_perlin_map()`, the actual seed used is the noiseparams seed
plus the world seed, to create world-specific noise.

Format of `size` is `{x=dimx, y=dimy, z=dimz}`. The `z` component is omitted
for 2D noise, and it must be larger than 1 for 3D noise (otherwise
`nil` is returned).

For each of the functions with an optional `buffer` parameter: If `buffer` is
not nil, this table will be used to store the result instead of creating a new
table.

### Methods

* `get_2d_map(pos)`: returns a `<size.x>` times `<size.y>` 2D array of 2D noise
  with values starting at `pos={x=,y=}`
* `get_3d_map(pos)`: returns a `<size.x>` times `<size.y>` times `<size.z>`
  3D array of 3D noise with values starting at `pos={x=,y=,z=}`.
* `get_2d_map_flat(pos, buffer)`: returns a flat `<size.x * size.y>` element
  array of 2D noise with values starting at `pos={x=,y=}`
* `get_3d_map_flat(pos, buffer)`: Same as `get2dMap_flat`, but 3D noise
* `calc_2d_map(pos)`: Calculates the 2d noise map starting at `pos`. The result
  is stored internally.
* `calc_3d_map(pos)`: Calculates the 3d noise map starting at `pos`. The result
  is stored internally.
* `get_map_slice(slice_offset, slice_size, buffer)`: In the form of an array,
  returns a slice of the most recently computed noise results. The result slice
  begins at coordinates `slice_offset` and takes a chunk of `slice_size`.
  E.g. to grab a 2-slice high horizontal 2d plane of noise starting at buffer
  offset y = 20:
  `noisevals = noise:get_map_slice({y=20}, {y=2})`
  It is important to note that `slice_offset` offset coordinates begin at 1,
  and are relative to the starting position of the most recently calculated
  noise.
  To grab a single vertical column of noise starting at map coordinates
  x = 1023, y=1000, z = 1000:
  `noise:calc_3d_map({x=1000, y=1000, z=1000})`
  `noisevals = noise:get_map_slice({x=24, z=1}, {x=1, z=1})`

`PlayerMetaRef`
---------------

Player metadata.
Uses the same method of storage as the deprecated player attribute API, so
data there will also be in player meta.
Can be obtained using `player:get_meta()`.

### Methods

* All methods in MetaDataRef

`PseudoRandom`
--------------

A 16-bit pseudorandom number generator.
Uses a well-known LCG algorithm introduced by K&R.

**Note**:
`PseudoRandom` is slower and has worse random distribution than `PcgRandom`.
Use `PseudoRandom` only if you need output to match the well-known LCG algorithm introduced by K&R.
Otherwise, use `PcgRandom`.

* constructor `PseudoRandom(seed)`
  * `seed`: 32-bit signed number

### Methods

* `next()`: return next integer random number [`0`...`32767`]
* `next(min, max)`: return next integer random number [`min`...`max`]
    * Either `max - min == 32767` or `max - min <= 6553` must be true
      due to the simple implementation making a bad distribution otherwise.
* `get_state()`: return state of pseudorandom generator as number
    * use returned number as seed in PseudoRandom constructor to restore

`Raycast`
---------

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
* `liquids`: if false, liquid nodes (`liquidtype ~= "none"`) won't be
             returned. Default is false.

### Limitations

Raycasts don't always work properly for attached objects as the server has no knowledge of models & bones.

**Rotated selectionboxes paired with `automatic_rotate` are not reliable** either since the server
can't reliably know the total rotation of the objects on different clients (which may differ on a per-client basis).
The server calculates the total rotation incurred through `automatic_rotate` as a "best guess"
assuming the object was active & rotating on the client all the time since its creation.
This may be significantly out of sync with what clients see.
Additionally, network latency and delayed property sending may create a mismatch of client- & server rotations.

In singleplayer mode, raycasts on objects with rotated selectionboxes & automatic rotate will usually only be slightly off;
toggling automatic rotation may however cause errors to add up.

In multiplayer mode, the error may be arbitrarily large.

### Methods

* `next()`: returns a `pointed_thing` with exact pointing location
    * Returns the next thing pointed by the ray or nil.

`SecureRandom`
--------------

Interface for the operating system's crypto-secure PRNG.

It can be created via `SecureRandom()`.  The constructor throws an error if a
secure random device cannot be found on the system.

### Methods

* `next_bytes([count])`: return next `count` (default 1, capped at 2048) many
  random bytes, as a string.

`Settings`
----------

An interface to read config files in the format of `minetest.conf`.

`core.settings` is a `Settings` instance that can be used to access the
main config file (`minetest.conf`). Instances for other config files can be
created via `Settings(filename)`.

Engine settings on the `core.settings` object have internal defaults that
will be returned if a setting is unset.
The engine does *not* (yet) read `settingtypes.txt` for this purpose. This
means that no defaults will be returned for mod settings.

### Methods

* `get(key)`: returns a value
    * Returns `nil` if `key` is not found.
* `get_bool(key, [default])`: returns a boolean
    * `default` is the value returned if `key` is not found.
    * Returns `nil` if `key` is not found and `default` not specified.
* `get_np_group(key)`: returns a NoiseParams table
    * Returns `nil` if `key` is not found.
* `get_flags(key)`:
    * Returns `{flag = true/false, ...}` according to the set flags.
    * Is currently limited to mapgen flags `mg_flags` and mapgen-specific
      flags like `mgv5_spflags`.
    * Returns `nil` if `key` is not found.
* `get_pos(key)`:
    * Returns a `vector`
    * Returns `nil` if no value is found or parsing failed.
* `set(key, value)`
    * Setting names can't contain whitespace or any of `="{}#`.
    * Setting values can't contain the sequence `\n"""`.
    * Setting names starting with "secure." can't be set on the main settings
      object (`core.settings`).
* `set_bool(key, value)`
    * See documentation for `set()` above.
* `set_np_group(key, value)`
    * `value` is a NoiseParams table.
    * Also, see documentation for `set()` above.
* `set_pos(key, value)`
    * `value` is a `vector`.
    * Also, see documentation for `set()` above.
* `remove(key)`: returns a boolean (`true` for success)
* `get_names()`: returns `{key1,...}`
* `has(key)`:
    * Returns a boolean indicating whether `key` exists.
    * In contrast to the various getter functions, `has()` doesn't consider
      any default values.
    * This means that on the main settings object (`core.settings`),
      `get(key)` might return a value even if `has(key)` returns `false`.
* `write()`: returns a boolean (`true` for success)
    * Writes changes to file.
* `to_table()`: returns `{[key1]=value1,...}`

### Format

The settings have the format `key = value`. Example:

    foo = example text
    bar = """
    Multiline
    value
    """


`StorageRef`
------------

Mod metadata: per mod metadata, saved automatically.
Can be obtained via `core.get_mod_storage()` during load time.

WARNING: This storage backend is incapable of saving raw binary data due
to restrictions of JSON.

### Methods

* All methods in MetaDataRef
