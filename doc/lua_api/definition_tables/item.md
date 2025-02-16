Item definition
---------------

Used by `core.register_node`, `core.register_craftitem`, and
`core.register_tool`.

```lua
{
    description = "",
    -- Can contain new lines. "\n" has to be used as new line character.
    -- See also: `get_description` in [`ItemStack`]

    short_description = "",
    -- Must not contain new lines.
    -- Defaults to nil.
    -- Use an [`ItemStack`] to get the short description, e.g.:
    --   ItemStack(itemname):get_short_description()

    groups = {},
    -- key = name, value = rating; rating = <number>.
    -- If rating not applicable, use 1.
    -- e.g. {wool = 1, fluffy = 3}
    --      {soil = 2, outerspace = 1, crumbly = 1}
    --      {bendy = 2, snappy = 1},
    --      {hard = 1, metal = 1, spikes = 1}

    inventory_image = "",
    -- Texture shown in the inventory GUI
    -- Defaults to a 3D rendering of the node if left empty.

    inventory_overlay = "",
    -- An overlay texture which is not affected by colorization

    wield_image = "",
    -- Texture shown when item is held in hand
    -- Defaults to a 3D rendering of the node if left empty.

    wield_overlay = "",
    -- Like inventory_overlay but only used in the same situation as wield_image

    wield_scale = {x = 1, y = 1, z = 1},
    -- Scale for the item when held in hand

    palette = "",
    -- An image file containing the palette of a node.
    -- You can set the currently used color as the "palette_index" field of
    -- the item stack metadata.
    -- The palette is always stretched to fit indices between 0 and 255, to
    -- ensure compatibility with "colorfacedir" (and similar) nodes.

    color = "#ffffffff",
    -- Color the item is colorized with. The palette overrides this.

    stack_max = 99,
    -- Maximum amount of items that can be in a single stack.
    -- The default can be changed by the setting `default_stack_max`

    range = 4.0,
    -- Range of node and object pointing that is possible with this item held
    -- Can be overridden with itemstack meta.

    liquids_pointable = false,
    -- If true, item can point to all liquid nodes (`liquidtype ~= "none"`),
    -- even those for which `pointable = false`

    pointabilities = {
        nodes = {
            ["default:stone"] = "blocking",
            ["group:leaves"] = false,
        },
        objects = {
            ["modname:entityname"] = true,
            ["group:ghosty"] = true, -- (an armor group)
        },
    },
    -- Contains lists to override the `pointable` property of nodes and objects.
    -- The index can be a node/entity name or a group with the prefix `"group:"`.
    -- (For objects `armor_groups` are used and for players the entity name is irrelevant.)
    -- If multiple fields fit, the following priority order is applied:
    -- 1. value of matching node/entity name
    -- 2. `true` for any group
    -- 3. `false` for any group
    -- 4. `"blocking"` for any group
    -- 5. `liquids_pointable` if it is a liquid node
    -- 6. `pointable` property of the node or object

    light_source = 0,
    -- When used for nodes: Defines amount of light emitted by node.
    -- Otherwise: Defines texture glow when viewed as a dropped item
    -- To set the maximum (14), use the value 'core.LIGHT_MAX'.
    -- A value outside the range 0 to core.LIGHT_MAX causes undefined
    -- behavior.

    -- See "Tool Capabilities" section for an example including explanation
    tool_capabilities = {
        full_punch_interval = 1.0,
        max_drop_level = 0,
        groupcaps = {
            -- For example:
            choppy = {times = {2.50, 1.40, 1.00}, uses = 20, maxlevel = 2},
        },
        damage_groups = {groupname = damage},
        -- Damage values must be between -32768 and 32767 (2^15)

        punch_attack_uses = nil,
        -- Amount of uses this tool has for attacking players and entities
        -- by punching them (0 = infinite uses).
        -- For compatibility, this is automatically set from the first
        -- suitable groupcap using the formula "uses * 3^(maxlevel - 1)".
        -- It is recommend to set this explicitly instead of relying on the
        -- fallback behavior.
    },

    -- Set wear bar color of the tool by setting color stops and blend mode
    -- See "Wear Bar Color" section for further explanation including an example
    wear_color = {
        -- interpolation mode: 'constant' or 'linear'
        -- (nil defaults to 'constant')
        blend = "linear",
        color_stops = {
            [0.0] = "#ff0000",
            [0.5] = "#ffff00",
            [1.0] = "#00ff00",
        }
    },

    node_placement_prediction = nil,
    -- If nil and item is node, prediction is made automatically.
    -- If nil and item is not a node, no prediction is made.
    -- If "" and item is anything, no prediction is made.
    -- Otherwise should be name of node which the client immediately places
    -- on ground when the player places the item. Server will always update
    -- with actual result shortly.

    node_dig_prediction = "air",
    -- if "", no prediction is made.
    -- if "air", node is removed.
    -- Otherwise should be name of node which the client immediately places
    -- upon digging. Server will always update with actual result shortly.

    touch_interaction = <TouchInteractionMode> OR {
        pointed_nothing = <TouchInteractionMode>,
        pointed_node    = <TouchInteractionMode>,
        pointed_object  = <TouchInteractionMode>,
    },
      -- Only affects touchscreen clients.
      -- Defines the meaning of short and long taps with the item in hand.
      -- If specified as a table, the field to be used is selected according to
      -- the current `pointed_thing`.
      -- There are three possible TouchInteractionMode values:
      -- * "long_dig_short_place" (long tap  = dig, short tap = place)
      -- * "short_dig_long_place" (short tap = dig, long tap  = place)
      -- * "user":
      --   * For `pointed_object`: Equivalent to "short_dig_long_place" if the
      --     client-side setting "touch_punch_gesture" is "short_tap" (the
      --     default value) and the item is able to punch (i.e. has no on_use
      --     callback defined).
      --     Equivalent to "long_dig_short_place" otherwise.
      --   * For `pointed_node` and `pointed_nothing`:
      --     Equivalent to "long_dig_short_place".
      --   * The behavior of "user" may change in the future.
      -- The default value is "user".

    sound = {
        -- Definition of item sounds to be played at various events.
        -- All fields in this table are optional.

        breaks = <SimpleSoundSpec>,
        -- When tool breaks due to wear. Ignored for non-tools

        eat = <SimpleSoundSpec>,
        -- When item is eaten with `core.do_item_eat`

        punch_use = <SimpleSoundSpec>,
        -- When item is used with the 'punch/mine' key pointing at a node or entity

        punch_use_air = <SimpleSoundSpec>,
        -- When item is used with the 'punch/mine' key pointing at nothing (air)
    },

    on_place = function(itemstack, placer, pointed_thing),
    -- When the 'place' key was pressed with the item in hand
    -- and a node was pointed at.
    -- Shall place item and return the leftover itemstack
    -- or nil to not modify the inventory.
    -- The placer may be any ObjectRef or nil.
    -- default: core.item_place

    on_secondary_use = function(itemstack, user, pointed_thing),
    -- Same as on_place but called when not pointing at a node.
    -- Function must return either nil if inventory shall not be modified,
    -- or an itemstack to replace the original itemstack.
    -- The user may be any ObjectRef or nil.
    -- default: nil

    on_drop = function(itemstack, dropper, pos),
    -- Shall drop item and return the leftover itemstack.
    -- The dropper may be any ObjectRef or nil.
    -- default: core.item_drop

    on_pickup = function(itemstack, picker, pointed_thing, time_from_last_punch, ...),
    -- Called when a dropped item is punched by a player.
    -- Shall pick-up the item and return the leftover itemstack or nil to not
    -- modify the dropped item.
    -- Parameters:
    -- * `itemstack`: The `ItemStack` to be picked up.
    -- * `picker`: Any `ObjectRef` or `nil`.
    -- * `pointed_thing` (optional): The dropped item (a `"__builtin:item"`
    --   luaentity) as `type="object"` `pointed_thing`.
    -- * `time_from_last_punch, ...` (optional): Other parameters from
    --   `luaentity:on_punch`.
    -- default: `core.item_pickup`

    on_use = function(itemstack, user, pointed_thing),
    -- default: nil
    -- When user pressed the 'punch/mine' key with the item in hand.
    -- Function must return either nil if inventory shall not be modified,
    -- or an itemstack to replace the original itemstack.
    -- e.g. itemstack:take_item(); return itemstack
    -- Otherwise, the function is free to do what it wants.
    -- The user may be any ObjectRef or nil.
    -- The default functions handle regular use cases.

    after_use = function(itemstack, user, node, digparams),
    -- default: nil
    -- If defined, should return an itemstack and will be called instead of
    -- wearing out the item (if tool). If returns nil, does nothing.
    -- If after_use doesn't exist, it is the same as:
    --   function(itemstack, user, node, digparams)
    --     itemstack:add_wear(digparams.wear)
    --     return itemstack
    --   end
    -- The user may be any ObjectRef or nil.

    _custom_field = whatever,
    -- Add your own custom fields. By convention, all custom field names
    -- should start with `_` to avoid naming collisions with future engine
    -- usage.
}
```
