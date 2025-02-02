Definition tables
=================

Object properties
-----------------

Used by `ObjectRef` methods. Part of an Entity definition.
These properties are not persistent, but are applied automatically to the
corresponding Lua entity using the given registration fields.
Player properties need to be saved manually.

```lua
{
    hp_max = 10,
    -- Defines the maximum and default HP of the object.
    -- For Lua entities, the maximum is not enforced.
    -- For players, this defaults to `core.PLAYER_MAX_HP_DEFAULT` (20).
    -- For Lua entities, the default is 10.

    breath_max = 0,
    -- For players only. Defines the maximum amount of "breath" for the player.
    -- Defaults to `core.PLAYER_MAX_BREATH_DEFAULT` (10).

    zoom_fov = 0.0,
    -- For players only. Zoom FOV in degrees.
    -- Note that zoom loads and/or generates world beyond the server's
    -- maximum send and generate distances, so acts like a telescope.
    -- Smaller zoom_fov values increase the distance loaded/generated.
    -- Defaults to 15 in creative mode, 0 in survival mode.
    -- zoom_fov = 0 disables zooming for the player.

    eye_height = 1.625,
    -- For players only. Camera height above feet position in nodes.

    physical = false,
    -- Collide with `walkable` nodes.

    collide_with_objects = true,
    -- Collide with other objects if physical = true

    collisionbox = { -0.5, -0.5, -0.5, 0.5, 0.5, 0.5 },  -- default
    selectionbox = { -0.5, -0.5, -0.5, 0.5, 0.5, 0.5, rotate = false },
    -- { xmin, ymin, zmin, xmax, ymax, zmax } in nodes from object position.
    -- Collision boxes cannot rotate, setting `rotate = true` on it has no effect.
    -- If not set, the selection box copies the collision box, and will also not rotate.
    -- If `rotate = false`, the selection box will not rotate with the object itself, remaining fixed to the axes.
    -- If `rotate = true`, it will match the object's rotation and any attachment rotations.
    -- Raycasts use the selection box and object's rotation, but do *not* obey attachment rotations.
    -- For server-side raycasts to work correctly,
    -- the selection box should extend at most 5 units in each direction.


    pointable = true,
    -- Can be `true` if it is pointable, `false` if it can be pointed through,
    -- or `"blocking"` if it is pointable but not selectable.
    -- Clients older than 5.9.0 interpret `pointable = "blocking"` as `pointable = true`.
    -- Can be overridden by the `pointabilities` of the held item.

    visual = "cube" / "sprite" / "upright_sprite" / "mesh" / "wielditem" / "item",
    -- "cube" is a node-sized cube.
    -- "sprite" is a flat texture always facing the player.
    -- "upright_sprite" is a vertical flat texture.
    -- "mesh" uses the defined mesh model.
    -- "wielditem" is used for dropped items.
    --   (see builtin/game/item_entity.lua).
    --   For this use 'wield_item = itemname'.
    --   Setting 'textures = {itemname}' has the same effect, but is deprecated.
    --   If the item has a 'wield_image' the object will be an extrusion of
    --   that, otherwise:
    --   If 'itemname' is a cubic node or nodebox the object will appear
    --   identical to 'itemname'.
    --   If 'itemname' is a plantlike node the object will be an extrusion
    --   of its texture.
    --   Otherwise for non-node items, the object will be an extrusion of
    --   'inventory_image'.
    --   If 'itemname' contains a ColorString or palette index (e.g. from
    --   `core.itemstring_with_palette()`), the entity will inherit the color.
    --   Wielditems are scaled a bit. If you want a wielditem to appear
    --   to be as large as a node, use `0.667` in `visual_size`
    -- "item" is similar to "wielditem" but ignores the 'wield_image' parameter.

    visual_size = {x = 1, y = 1, z = 1},
    -- Multipliers for the visual size. If `z` is not specified, `x` will be used
    -- to scale the entity along both horizontal axes.

    mesh = "model.obj",
    -- File name of mesh when using "mesh" visual

    textures = {},
    -- Number of required textures depends on visual.
    -- "cube" uses 6 textures just like a node, but all 6 must be defined.
    -- "sprite" uses 1 texture.
    -- "upright_sprite" uses 2 textures: {front, back}.
    -- "mesh" requires one texture for each mesh buffer/material (in order)
    -- Deprecated usage of "wielditem" expects 'textures = {itemname}' (see 'visual' above).

    colors = {},
    -- Number of required colors depends on visual

    use_texture_alpha = false,
    -- Use texture's alpha channel.
    -- Excludes "upright_sprite" and "wielditem".
    -- Note: currently causes visual issues when viewed through other
    -- semi-transparent materials such as water.

    spritediv = {x = 1, y = 1},
    -- Used with spritesheet textures for animation and/or frame selection
    -- according to position relative to player.
    -- Defines the number of columns and rows in the spritesheet:
    -- {columns, rows}.

    initial_sprite_basepos = {x = 0, y = 0},
    -- Used with spritesheet textures.
    -- Defines the {column, row} position of the initially used frame in the
    -- spritesheet.

    is_visible = true,
    -- If false, object is invisible and can't be pointed.

    makes_footstep_sound = false,
    -- If true, is able to make footstep sounds of nodes
    -- (see node sound definition for details).

    automatic_rotate = 0,
    -- Set constant rotation in radians per second, positive or negative.
    -- Object rotates along the local Y-axis, and works with set_rotation.
    -- Set to 0 to disable constant rotation.

    stepheight = 0,
    -- If positive number, object will climb upwards when it moves
    -- horizontally against a `walkable` node, if the height difference
    -- is within `stepheight`.

    automatic_face_movement_dir = 0.0,
    -- Automatically set yaw to movement direction, offset in degrees.
    -- 'false' to disable.

    automatic_face_movement_max_rotation_per_sec = -1,
    -- Limit automatic rotation to this value in degrees per second.
    -- No limit if value <= 0.

    backface_culling = true,
    -- Set to false to disable backface_culling for model

    glow = 0,
    -- Add this much extra lighting when calculating texture color.
    -- Value < 0 disables light's effect on texture color.
    -- For faking self-lighting, UI style entities, or programmatic coloring
    -- in mods.

    nametag = "",
    -- The name to display on the head of the object. By default empty.
    -- If the object is a player, a nil or empty nametag is replaced by the player's name.
    -- For all other objects, a nil or empty string removes the nametag.
    -- To hide a nametag, set its color alpha to zero. That will disable it entirely.

    nametag_color = <ColorSpec>,
    -- Sets text color of nametag

    nametag_bgcolor = <ColorSpec>,
    -- Sets background color of nametag
    -- `false` will cause the background to be set automatically based on user settings.
    -- Default: false

    infotext = "",
    -- Same as infotext for nodes. Empty by default

    static_save = true,
    -- If false, never save this object statically. It will simply be
    -- deleted when the block gets unloaded.
    -- The get_staticdata() callback is never called then.
    -- Defaults to 'true'.

    damage_texture_modifier = "^[brighten",
    -- Texture modifier to be applied for a short duration when object is hit

    shaded = true,
    -- Setting this to 'false' disables diffuse lighting of entity

    show_on_minimap = false,
    -- Defaults to true for players, false for other entities.
    -- If set to true the entity will show as a marker on the minimap.
}
```

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


ABM (ActiveBlockModifier) definition
------------------------------------

Used by `core.register_abm`.

```lua
{
    label = "Lava cooling",
    -- Descriptive label for profiling purposes (optional).
    -- Definitions with identical labels will be listed as one.

    nodenames = {"default:lava_source"},
    -- Apply `action` function to these nodes.
    -- `group:groupname` can also be used here.

    neighbors = {"default:water_source", "default:water_flowing"},
    -- Only apply `action` to nodes that have one of, or any
    -- combination of, these neighbors.
    -- If left out or empty, any neighbor will do.
    -- `group:groupname` can also be used here.

    without_neighbors = {"default:lava_source", "default:lava_flowing"},
    -- Only apply `action` to nodes that have no one of these neighbors.
    -- If left out or empty, it has no effect.
    -- `group:groupname` can also be used here.

    interval = 10.0,
    -- Operation interval in seconds

    chance = 50,
    -- Probability of triggering `action` per-node per-interval is 1.0 / chance (integers only)

    min_y = -32768,
    max_y = 32767,
    -- min and max height levels where ABM will be processed (inclusive)
    -- can be used to reduce CPU usage

    catch_up = true,
    -- If true, catch-up behavior is enabled: The `chance` value is
    -- temporarily reduced when returning to an area to simulate time lost
    -- by the area being unattended. Note that the `chance` value can often
    -- be reduced to 1.

    action = function(pos, node, active_object_count, active_object_count_wider),
    -- Function triggered for each qualifying node.
    -- `active_object_count` is number of active objects in the node's
    -- mapblock.
    -- `active_object_count_wider` is number of active objects in the node's
    -- mapblock plus all 26 neighboring mapblocks. If any neighboring
    -- mapblocks are unloaded an estimate is calculated for them based on
    -- loaded mapblocks.
}
```

LBM (LoadingBlockModifier) definition
-------------------------------------

Used by `core.register_lbm`.

A loading block modifier (LBM) is used to define a function that is called for
specific nodes (defined by `nodenames`) when a mapblock which contains such nodes
gets activated (not loaded!).

Note: LBMs operate on a "snapshot" of node positions taken once before they are triggered.
That means if an LBM callback adds a node, it won't be taken into account.
However the engine guarantees that when the callback is called that all given position(s)
contain a matching node.

```lua
{
    label = "Upgrade legacy doors",
    -- Descriptive label for profiling purposes (optional).
    -- Definitions with identical labels will be listed as one.

    name = "modname:replace_legacy_door",
    -- Identifier of the LBM, should follow the modname:<whatever> convention

    nodenames = {"default:lava_source"},
    -- List of node names to trigger the LBM on.
    -- Names of non-registered nodes and groups (as group:groupname)
    -- will work as well.

    run_at_every_load = false,
    -- Whether to run the LBM's action every time a block gets activated,
    -- and not only the first time the block gets activated after the LBM
    -- was introduced.

    action = function(pos, node, dtime_s) end,
    -- Function triggered for each qualifying node.
    -- `dtime_s` is the in-game time (in seconds) elapsed since the block
    -- was last active.

    bulk_action = function(pos_list, dtime_s) end,
    -- Function triggered with a list of all applicable node positions at once.
    -- This can be provided as an alternative to `action` (not both).
    -- Available since `core.features.bulk_lbms` (5.10.0)
    -- `dtime_s`: as above
}
```

Tile definition
---------------

* `"image.png"`
* `{name="image.png", animation={Tile Animation definition}}`
* `{name="image.png", backface_culling=bool, align_style="node"/"world"/"user", scale=int}`
    * backface culling enabled by default for most nodes
    * align style determines whether the texture will be rotated with the node
      or kept aligned with its surroundings. "user" means that client
      setting will be used, similar to `glasslike_framed_optional`.
      Note: supported by solid nodes and nodeboxes only.
    * scale is used to make texture span several (exactly `scale`) nodes,
      instead of just one, in each direction. Works for world-aligned
      textures only.
      Note that as the effect is applied on per-mapblock basis, `16` should
      be equally divisible by `scale` or you may get wrong results.
* `{name="image.png", color=ColorSpec}`
    * the texture's color will be multiplied with this color.
    * the tile's color overrides the owning node's color in all cases.
* deprecated, yet still supported field names:
    * `image` (name)

Tile animation definition
-------------------------

```lua
{
    type = "vertical_frames",

    aspect_w = 16,
    -- Width of a frame in pixels

    aspect_h = 16,
    -- Height of a frame in pixels

    length = 3.0,
    -- Full loop length
}

{
    type = "sheet_2d",

    frames_w = 5,
    -- Width in number of frames

    frames_h = 3,
    -- Height in number of frames

    frame_length = 0.5,
    -- Length of a single frame
}
```

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

Node definition
---------------

Used by `core.register_node`.

```lua
{
    -- <all fields allowed in item definitions>

    drawtype = "normal",  -- See "Node drawtypes"

    visual_scale = 1.0,
    -- Supported for drawtypes "plantlike", "signlike", "torchlike",
    -- "firelike", "mesh", "nodebox", "allfaces".
    -- For plantlike and firelike, the image will start at the bottom of the
    -- node. For torchlike, the image will start at the surface to which the
    -- node "attaches". For the other drawtypes the image will be centered
    -- on the node.

    tiles = {tile definition 1, def2, def3, def4, def5, def6},
    -- Textures of node; +Y, -Y, +X, -X, +Z, -Z
    -- List can be shortened to needed length.

    overlay_tiles = {tile definition 1, def2, def3, def4, def5, def6},
    -- Same as `tiles`, but these textures are drawn on top of the base
    -- tiles. You can use this to colorize only specific parts of your
    -- texture. If the texture name is an empty string, that overlay is not
    -- drawn. Since such tiles are drawn twice, it is not recommended to use
    -- overlays on very common nodes.

    special_tiles = {tile definition 1, Tile definition 2},
    -- Special textures of node; used rarely.
    -- List can be shortened to needed length.

    color = ColorSpec,
    -- The node's original color will be multiplied with this color.
    -- If the node has a palette, then this setting only has an effect in
    -- the inventory and on the wield item.

    use_texture_alpha = ...,
    -- Specifies how the texture's alpha channel will be used for rendering.
    -- Possible values:
    -- * "opaque":
    --   Node is rendered opaque regardless of alpha channel.
    -- * "clip":
    --   A given pixel is either fully see-through or opaque
    --   depending on the alpha channel being below/above 50% in value.
    --   Use this for nodes with fully transparent and fully opaque areas.
    -- * "blend":
    --   The alpha channel specifies how transparent a given pixel
    --   of the rendered node is. This comes at a performance cost.
    --   Only use this when correct rendering
    --   among semitransparent nodes is necessary.
    -- The default is "opaque" for drawtypes normal, liquid and flowingliquid,
    -- mesh and nodebox or "clip" otherwise.
    -- If set to a boolean value (deprecated): true either sets it to blend
    -- or clip, false sets it to clip or opaque mode depending on the drawtype.

    palette = "",
    -- The node's `param2` is used to select a pixel from the image.
    -- Pixels are arranged from left to right and from top to bottom.
    -- The node's color will be multiplied with the selected pixel's color.
    -- Tiles can override this behavior.
    -- Only when `paramtype2` supports palettes.

    post_effect_color = "#00000000",
    -- Screen tint if a player is inside this node, see `ColorSpec`.
    -- Color is alpha-blended over the screen.

    post_effect_color_shaded = false,
    -- Determines whether `post_effect_color` is affected by lighting.

    paramtype = "none",  -- See "Nodes"

    paramtype2 = "none",  -- See "Nodes"

    place_param2 = 0,
    -- Value for param2 that is set when player places node

    wallmounted_rotate_vertical = false,
    -- If true, place_param2 is nil, and this is a wallmounted node,
    -- this node might use the special 90° rotation when placed
    -- on the floor or ceiling, depending on the direction.
    -- See the explanation about wallmounted for details.
    -- Otherwise, the rotation is always the same on vertical placement.

    is_ground_content = true,
    -- If false, the cave generator and dungeon generator will not carve
    -- through this node.
    -- Specifically, this stops mod-added nodes being removed by caves and
    -- dungeons when those generate in a neighbor mapchunk and extend out
    -- beyond the edge of that mapchunk.

    sunlight_propagates = false,
    -- If true, sunlight will go infinitely through this node

    walkable = true,  -- If true, objects collide with node

    pointable = true,
    -- Can be `true` if it is pointable, `false` if it can be pointed through,
    -- or `"blocking"` if it is pointable but not selectable.
    -- Clients older than 5.9.0 interpret `pointable = "blocking"` as `pointable = true`.
    -- Can be overridden by the `pointabilities` of the held item.
    -- A client may be able to point non-pointable nodes, since it isn't checked server-side.

    diggable = true,  -- If false, can never be dug

    climbable = false,  -- If true, can be climbed on like a ladder

    move_resistance = 0,
    -- Slows down movement of players through this node (max. 7).
    -- If this is nil, it will be equal to liquid_viscosity.
    -- Note: If liquid movement physics apply to the node
    -- (see `liquid_move_physics`), the movement speed will also be
    -- affected by the `movement_liquid_*` settings.

    buildable_to = false,  -- If true, placed nodes can replace this node

    floodable = false,
    -- If true, liquids flow into and replace this node.
    -- Warning: making a liquid node 'floodable' will cause problems.

    liquidtype = "none",  -- specifies liquid flowing physics
    -- * "none":    no liquid flowing physics
    -- * "source":  spawns flowing liquid nodes at all 4 sides and below;
    --              recommended drawtype: "liquid".
    -- * "flowing": spawned from source, spawns more flowing liquid nodes
    --              around it until `liquid_range` is reached;
    --              will drain out without a source;
    --              recommended drawtype: "flowingliquid".
    -- If it's "source" or "flowing", then the
    -- `liquid_alternative_*` fields _must_ be specified

    liquid_alternative_flowing = "",
    liquid_alternative_source = "",
    -- These fields may contain node names that represent the
    -- flowing version (`liquid_alternative_flowing`) and
    -- source version (`liquid_alternative_source`) of a liquid.
    --
    -- Specifically, these fields are required if `liquidtype ~= "none"` or
    -- `drawtype == "flowingliquid"`.
    --
    -- Liquids consist of up to two nodes: source and flowing.
    --
    -- There are two ways to define a liquid:
    -- 1) Source node and flowing node. This requires both fields to be
    --    specified for both nodes.
    -- 2) Standalone source node (cannot flow). `liquid_alternative_source`
    --    must be specified and `liquid_range` must be set to 0.
    --
    -- Example:
    --     liquid_alternative_flowing = "example:water_flowing",
    --     liquid_alternative_source = "example:water_source",

    liquid_viscosity = 0,
    -- Controls speed at which the liquid spreads/flows (max. 7).
    -- 0 is fastest, 7 is slowest.
    -- By default, this also slows down movement of players inside the node
    -- (can be overridden using `move_resistance`)

    liquid_renewable = true,
    -- If true, a new liquid source can be created by placing two or more
    -- sources nearby

    liquid_move_physics = nil, -- specifies movement physics if inside node
    -- * false: No liquid movement physics apply.
    -- * true: Enables liquid movement physics. Enables things like
    --   ability to "swim" up/down, sinking slowly if not moving,
    --   smoother speed change when falling into, etc. The `movement_liquid_*`
    --   settings apply.
    -- * nil: Will be treated as true if `liquidtype ~= "none"`
    --   and as false otherwise.

    air_equivalent = nil,
    -- unclear meaning, the engine sets this to true for 'air' and 'ignore'
    -- deprecated.

    leveled = 0,
    -- Only valid for "nodebox" drawtype with 'type = "leveled"'.
    -- Allows defining the nodebox height without using param2.
    -- The nodebox height is 'leveled' / 64 nodes.
    -- The maximum value of 'leveled' is `leveled_max`.

    leveled_max = 127,
    -- Maximum value for `leveled` (0-127), enforced in
    -- `core.set_node_level` and `core.add_node_level`.
    -- Values above 124 might causes collision detection issues.

    liquid_range = 8,
    -- Maximum distance that flowing liquid nodes can spread around
    -- source on flat land;
    -- maximum = 8; set to 0 to disable liquid flow

    drowning = 0,
    -- Player will take this amount of damage if no bubbles are left

    damage_per_second = 0,
    -- If player is inside node, this damage is caused

    node_box = {type = "regular"},  -- See "Node boxes"

    connects_to = {},
    -- Used for nodebox nodes with the type == "connected".
    -- Specifies to what neighboring nodes connections will be drawn.
    -- e.g. `{"group:fence", "default:wood"}` or `"default:stone"`

    connect_sides = {},
    -- Tells connected nodebox nodes to connect only to these sides of this
    -- node. possible: "top", "bottom", "front", "left", "back", "right"

    mesh = "",
    -- File name of mesh when using "mesh" drawtype

    selection_box = {
        -- see [Node boxes] for possibilities
    },
    -- Custom selection box definition. Multiple boxes can be defined.
    -- If "nodebox" drawtype is used and selection_box is nil, then node_box
    -- definition is used for the selection box.

    collision_box = {
        -- see [Node boxes] for possibilities
    },
    -- Custom collision box definition. Multiple boxes can be defined.
    -- If "nodebox" drawtype is used and collision_box is nil, then node_box
    -- definition is used for the collision box.

    -- Support maps made in and before January 2012
    legacy_facedir_simple = false,
    legacy_wallmounted = false,

    waving = 0,
    -- Valid for drawtypes:
    -- mesh, nodebox, plantlike, allfaces_optional, liquid, flowingliquid.
    -- 1 - wave node like plants (node top moves side-to-side, bottom is fixed)
    -- 2 - wave node like leaves (whole node moves side-to-side)
    -- 3 - wave node like liquids (whole node moves up and down)
    -- Not all models will properly wave.
    -- plantlike drawtype can only wave like plants.
    -- allfaces_optional drawtype can only wave like leaves.
    -- liquid, flowingliquid drawtypes can only wave like liquids.

    sounds = {
        -- Definition of node sounds to be played at various events.
        -- All fields in this table are optional.

        footstep = <SimpleSoundSpec>,
        -- If walkable, played when object walks on it. If node is
        -- climbable or a liquid, played when object moves through it.
        -- Sound is played at the base of the object's collision-box.
        -- Gain is multiplied by `0.6`.
        -- For local player, it's played position-less, with normal gain.

        dig = <SimpleSoundSpec> or "__group",
        -- While digging node.
        -- If `"__group"`, then the sound will be
        -- `{name = "default_dig_<groupname>", gain = 0.5}` , where `<groupname>` is the
        -- name of the item's digging group with the fastest digging time.
        -- In case of a tie, one of the sounds will be played (but we
        -- cannot predict which one)
        -- Default value: `"__group"`

        dug = <SimpleSoundSpec>,
        -- Node was dug

        place = <SimpleSoundSpec>,
        -- Node was placed. Also played after falling

        place_failed = <SimpleSoundSpec>,
        -- When node placement failed.
        -- Note: This happens if the _built-in_ node placement failed.
        -- This sound will still be played if the node is placed in the
        -- `on_place` callback manually.

        fall = <SimpleSoundSpec>,
        -- When node starts to fall or is detached
    },

    drop = "",
    -- Name of dropped item when dug.
    -- Default dropped item is the node itself.

    -- Using a table allows multiple items, drop chances and item filtering:
    drop = {
        max_items = 1,
        -- Maximum number of item lists to drop.
        -- The entries in 'items' are processed in order. For each:
        -- Item filtering is applied, chance of drop is applied, if both are
        -- successful the entire item list is dropped.
        -- Entry processing continues until the number of dropped item lists
        -- equals 'max_items'.
        -- Therefore, entries should progress from low to high drop chance.
        items = {
            -- Examples:
            {
                -- 1 in 1000 chance of dropping a diamond.
                -- Default rarity is '1'.
                rarity = 1000,
                items = {"default:diamond"},
            },
            {
                -- Only drop if using an item whose name is identical to one
                -- of these.
                tools = {"default:shovel_mese", "default:shovel_diamond"},
                rarity = 5,
                items = {"default:dirt"},
                -- Whether all items in the dropped item list inherit the
                -- hardware coloring palette color from the dug node.
                -- Default is 'false'.
                inherit_color = true,
            },
            {
                -- Only drop if using an item whose name contains
                -- "default:shovel_" (this item filtering by string matching
                -- is deprecated, use tool_groups instead).
                tools = {"~default:shovel_"},
                rarity = 2,
                -- The item list dropped.
                items = {"default:sand", "default:desert_sand"},
            },
            {
                -- Only drop if using an item in the "magicwand" group, or
                -- an item that is in both the "pickaxe" and the "lucky"
                -- groups.
                tool_groups = {
                    "magicwand",
                    {"pickaxe", "lucky"}
                },
                items = {"default:coal_lump"},
            },
        },
    },

    on_construct = function(pos),
    -- Node constructor; called after adding node.
    -- Can set up metadata and stuff like that.
    -- Not called for bulk node placement (i.e. schematics and VoxelManip).
    -- Note: Within an on_construct callback, core.set_node can cause an
    -- infinite loop if it invokes the same callback.
    --  Consider using core.swap_node instead.
    -- default: nil

    on_destruct = function(pos),
    -- Node destructor; called before removing node.
    -- Not called for bulk node placement.
    -- default: nil

    after_destruct = function(pos, oldnode),
    -- Node destructor; called after removing node.
    -- Not called for bulk node placement.
    -- default: nil

    on_flood = function(pos, oldnode, newnode),
    -- Called when a liquid (newnode) is about to flood oldnode, if it has
    -- `floodable = true` in the nodedef. Not called for bulk node placement
    -- (i.e. schematics and VoxelManip) or air nodes. If return true the
    -- node is not flooded, but on_flood callback will most likely be called
    -- over and over again every liquid update interval.
    -- Default: nil
    -- Warning: making a liquid node 'floodable' will cause problems.

    preserve_metadata = function(pos, oldnode, oldmeta, drops),
    -- Called when `oldnode` is about be converted to an item, but before the
    -- node is deleted from the world or the drops are added. This is
    -- generally the result of either the node being dug or an attached node
    -- becoming detached.
    -- * `pos`: node position
    -- * `oldnode`: node table of node before it was deleted
    -- * `oldmeta`: metadata of node before it was deleted, as a metadata table
    -- * `drops`: a table of `ItemStack`s, so any metadata to be preserved can
    --   be added directly to one or more of the dropped items. See
    --   "ItemStackMetaRef".
    -- default: `nil`

    after_place_node = function(pos, placer, itemstack, pointed_thing),
    -- Called after constructing node when node was placed using
    -- core.item_place_node / core.place_node.
    -- If return true no item is taken from itemstack.
    -- `placer` may be any valid ObjectRef or nil.
    -- default: nil

    after_dig_node = function(pos, oldnode, oldmetadata, digger),
    -- Called after destructing the node when node was dug using
    -- `core.node_dig` / `core.dig_node`.
    -- * `pos`: node position
    -- * `oldnode`: node table of node before it was dug
    -- * `oldmetadata`: metadata of node before it was dug,
    --                  as a metadata table
    -- * `digger`: ObjectRef of digger
    -- default: nil

    can_dig = function(pos, [player]),
    -- Returns true if node can be dug, or false if not.
    -- default: nil

    on_punch = function(pos, node, puncher, pointed_thing),
    -- default: core.node_punch
    -- Called when puncher (an ObjectRef) punches the node at pos.
    -- By default calls core.register_on_punchnode callbacks.

    on_rightclick = function(pos, node, clicker, itemstack, pointed_thing),
    -- default: nil
    -- Called when clicker (an ObjectRef) used the 'place/build' key
    -- (not necessarily an actual rightclick)
    -- while pointing at the node at pos with 'node' being the node table.
    -- itemstack will hold clicker's wielded item.
    -- Shall return the leftover itemstack.
    -- Note: pointed_thing can be nil, if a mod calls this function.
    -- This function does not get triggered by clients <=0.4.16 if the
    -- "formspec" node metadata field is set.

    on_dig = function(pos, node, digger),
    -- default: core.node_dig
    -- By default checks privileges, wears out item (if tool) and removes node.
    -- return true if the node was dug successfully, false otherwise.
    -- Deprecated: returning nil is the same as returning true.

    on_timer = function(pos, elapsed),
    -- default: nil
    -- called by NodeTimers, see core.get_node_timer and NodeTimerRef.
    -- elapsed is the total time passed since the timer was started.
    -- return true to run the timer for another cycle with the same timeout
    -- value.

    on_receive_fields = function(pos, formname, fields, sender),
    -- fields = {name1 = value1, name2 = value2, ...}
    -- formname should be the empty string; you **must not** use formname.
    -- Called when an UI form (e.g. sign text input) returns data.
    -- See core.register_on_player_receive_fields for more info.
    -- default: nil

    allow_metadata_inventory_move = function(pos, from_list, from_index, to_list, to_index, count, player),
    -- Called when a player wants to move items inside the inventory.
    -- Return value: number of items allowed to move.

    allow_metadata_inventory_put = function(pos, listname, index, stack, player),
    -- Called when a player wants to put something into the inventory.
    -- Return value: number of items allowed to put.
    -- Return value -1: Allow and don't modify item count in inventory.

    allow_metadata_inventory_take = function(pos, listname, index, stack, player),
    -- Called when a player wants to take something out of the inventory.
    -- Return value: number of items allowed to take.
    -- Return value -1: Allow and don't modify item count in inventory.

    on_metadata_inventory_move = function(pos, from_list, from_index, to_list, to_index, count, player),
    on_metadata_inventory_put = function(pos, listname, index, stack, player),
    on_metadata_inventory_take = function(pos, listname, index, stack, player),
    -- Called after the actual action has happened, according to what was
    -- allowed.
    -- No return value.

    on_blast = function(pos, intensity),
    -- intensity: 1.0 = mid range of regular TNT.
    -- If defined, called when an explosion touches the node, instead of
    -- removing the node.

    mod_origin = "modname",
    -- stores which mod actually registered a node
    -- If the source could not be determined it contains "??"
    -- Useful for getting which mod truly registered something
    -- example: if a node is registered as ":othermodname:nodename",
    -- nodename will show "othermodname", but mod_origin will say "modname"
}
```

Wear Bar Color
--------------

'Wear Bar' is a property of items that defines the coloring
of the bar that appears under damaged tools.
If it is absent, the default behavior of green-yellow-red is
used.

### Wear bar colors definition

#### Syntax

```lua
{
    -- 'constant' or 'linear'
    -- (nil defaults to 'constant')
    blend = "linear",
    color_stops = {
        [0.0] = "#ff0000",
        [0.5] = "slateblue",
        [1.0] = {r=0, g=255, b=0, a=150},
    }
}
```

#### Blend mode `blend`

* `linear`: blends smoothly between each defined color point.
* `constant`: each color starts at its defined point, and continues up to the next point

#### Color stops `color_stops`

Specified as `ColorSpec` color values assigned to `float` durability keys.

"Durability" is defined as `1 - (wear / 65535)`.

#### Shortcut usage

Wear bar color can also be specified as a single `ColorSpec` instead of a table.

Crafting recipes
----------------

Crafting converts one or more inputs to one output itemstack of arbitrary
count (except for fuels, which don't have an output). The conversion reduces
each input ItemStack by 1.

Craft recipes are registered by `core.register_craft` and use a
table format. The accepted parameters are listed below.

Recipe input items can either be specified by item name (item count = 1)
or by group (see "Groups in crafting recipes" for details).

The following sections describe the types and syntaxes of recipes.

### Shaped

This is the default recipe type (when no `type` is specified).

A shaped recipe takes one or multiple items as input and has
a single item stack as output. The input items must be specified
in a 2-dimensional matrix (see parameters below) to specify the
exact arrangement (the "shape") in which the player must place them
in the crafting grid.

For example, for a 3x3 recipe, the `recipes` table must have
3 rows and 3 columns.

In order to craft the recipe, the players' crafting grid must
have equal or larger dimensions (both width and height).

Parameters:

* `type = "shaped"`: (optional) specifies recipe type as shaped
* `output`: Itemstring of output itemstack (item counts >= 1 are allowed)
* `recipe`: A 2-dimensional matrix of items, with a width *w* and height *h*.
    * *w* and *h* are chosen by you, they don't have to be equal but must be at least 1
    * The matrix is specified as a table containing tables containing itemnames
    * The inner tables are the rows. There must be *h* tables, specified from the top to the bottom row
    * Values inside of the inner table are the columns.
      Each inner table must contain a list of *w* items, specified from left to right
    * Empty slots *must* be filled with the empty string
* `replacements`: (optional) Allows you to replace input items with some other items
      when something is crafted
    * Provided as a list of item pairs of the form `{ old_item, new_item }` where
      `old_item` is the input item to replace (same syntax as for a regular input
      slot; groups are allowed) and `new_item` is an itemstring for the item stack
      it will become
    * When the output is crafted, Luanti iterates through the list
      of input items if the crafting grid. For each input item stack, it checks if
      it matches with an `old_item` in the item pair list.
        * If it matches, the item will be replaced. Also, this item pair
          will *not* be applied again for the remaining items
        * If it does not match, the item is consumed (reduced by 1) normally
    * The `new_item` will appear in one of 3 places:
        * Crafting grid, if the input stack size was exactly 1
        * Player inventory, if input stack size was larger
        * Drops as item entity, if it fits neither in craft grid or inventory

#### Examples

A typical shaped recipe:

```lua
-- Stone pickaxe
{
    output = "example:stone_pickaxe",
    -- A 3x3 recipe which needs 3 stone in the 1st row,
    -- and 1 stick in the horizontal middle in each of the 2nd and 3nd row.
    -- The 4 remaining slots have to be empty.
    recipe = {
        {"example:stone", "example:stone", "example:stone"}, -- row 1
        {"",              "example:stick", ""             }, -- row 2
        {"",              "example:stick", ""             }, -- row 3
    --   ^ column 1       ^ column 2       ^ column 3
    },
    -- There is no replacements table, so every input item
    -- will be consumed.
}
```

Simple replacement example:

```lua
-- Wet sponge
{
    output = "example:wet_sponge",
    -- 1x2 recipe with a water bucket above a dry sponge
    recipe = {
        {"example:water_bucket"},
        {"example:dry_sponge"},
    },
    -- When the wet sponge is crafted, the water bucket
    -- in the input slot is replaced with an empty
    -- bucket
    replacements = {
        {"example:water_bucket", "example:empty_bucket"},
    },
}
```

Complex replacement example 1:

```lua
-- Very wet sponge
{
    output = "example:very_wet_sponge",
    -- 3x3 recipe with a wet sponge in the center
    -- and 4 water buckets around it
    recipe = {
        {"","example:water_bucket",""},
        {"example:water_bucket","example:wet_sponge","example:water_bucket"},
        {"","example:water_bucket",""},
    },
    -- When the wet sponge is crafted, all water buckets
    -- in the input slot become empty
    replacements = {
        -- Without these repetitions, only the first
        -- water bucket would be replaced.
        {"example:water_bucket", "example:empty_bucket"},
        {"example:water_bucket", "example:empty_bucket"},
        {"example:water_bucket", "example:empty_bucket"},
        {"example:water_bucket", "example:empty_bucket"},
    },
}
```

Complex replacement example 2:

```lua
-- Magic book:
-- 3 magic orbs + 1 book crafts a magic book,
-- and the orbs will be replaced with 3 different runes.
{
    output = "example:magic_book",
    -- 3x2 recipe
    recipe = {
        -- 3 items in the group `magic_orb` on top of a book in the middle
        {"group:magic_orb", "group:magic_orb", "group:magic_orb"},
        {"", "example:book", ""},
    },
    -- When the book is crafted, the 3 magic orbs will be turned into
    -- 3 runes: ice rune, earth rune and fire rune (from left to right)
    replacements = {
        {"group:magic_orb", "example:ice_rune"},
        {"group:magic_orb", "example:earth_rune"},
        {"group:magic_orb", "example:fire_rune"},
    },
}
```

### Shapeless

Takes a list of input items (at least 1). The order or arrangement
of input items does not matter.

In order to craft the recipe, the players' crafting grid must have matching or
larger *count* of slots. The grid dimensions do not matter.

Parameters:

* `type = "shapeless"`: Mandatory
* `output`: Same as for shaped recipe
* `recipe`: List of item names
* `replacements`: Same as for shaped recipe

#### Example

```lua
{
    -- Craft a mushroom stew from a bowl, a brown mushroom and a red mushroom
    -- (no matter where in the input grid the items are placed)
    type = "shapeless",
    output = "example:mushroom_stew",
    recipe = {
        "example:bowl",
        "example:mushroom_brown",
        "example:mushroom_red",
    },
}
```

### Tool repair

Syntax:

    {
        type = "toolrepair",
        additional_wear = -0.02, -- multiplier of 65536
    }

Adds a shapeless recipe for *every* tool that doesn't have the `disable_repair=1`
group. If this recipe is used, repairing is possible with any crafting grid
with at least 2 slots.
The player can put 2 equal tools in the craft grid to get one "repaired" tool
back.
The wear of the output is determined by the wear of both tools, plus a
'repair bonus' given by `additional_wear`. To reduce the wear (i.e. 'repair'),
you want `additional_wear` to be negative.

The formula used to calculate the resulting wear is:

    65536 * (1 - ( (1 - tool_1_wear) + (1 - tool_2_wear) + additional_wear))

The result is rounded and can't be lower than 0. If the result is 65536 or higher,
no crafting is possible.

### Cooking

A cooking recipe has a single input item, a single output item stack
and a cooking time. It represents cooking/baking/smelting/etc. items in
an oven, furnace, or something similar; the exact meaning is up for games
to decide, if they choose to use cooking at all.

The engine does not implement anything specific to cooking recipes, but
the recipes can be retrieved later using `core.get_craft_result` to
have a consistent interface across different games/mods.

Parameters:

* `type = "cooking"`: Mandatory
* `output`: Same as for shaped recipe
* `recipe`: An itemname of the single input item
* `cooktime`: (optional) Time it takes to cook this item, in seconds.
              A floating-point number. (default: 3.0)
* `replacements`: Same meaning as for shaped recipes, but the mods
                  that utilize cooking recipes (e.g. for adding a furnace
                  node) need to implement replacements on their own

Note: Games and mods are free to re-interpret the cooktime in special
cases, e.g. for a super furnace that cooks items twice as fast.

#### Example

Cooking sand to glass in 3 seconds:

```lua
{
    type = "cooking",
    output = "example:glass",
    recipe = "example:sand",
    cooktime = 3.0,
}
```

### Fuel

A fuel recipe is an item associated with a "burning time" and an optional
item replacement. There is no output. This is usually used as fuel for
furnaces, ovens, stoves, etc.

Like with cooking recipes, the engine does not do anything specific with
fuel recipes and it's up to games and mods to use them by retrieving
them via `core.get_craft_result`.

Parameters:

* `type = "fuel"`: Mandatory
* `recipe`: Itemname of the item to be used as fuel
* `burntime`: (optional) Burning time this item provides, in seconds.
              A floating-point number. (default: 1.0)
* `replacements`: Same meaning as for shaped recipes, but the mods
                  that utilize fuels need to implement replacements
                  on their own

Note: Games and mods are free to re-interpret the burntime in special
cases, e.g. for an efficient furnace in which fuels burn twice as
long.

#### Examples

Coal lump with a burntime of 20 seconds. Will be consumed when used.

```lua
{
    type = "fuel",
    recipe = "example:coal_lump",
    burntime = 20.0,
}
```

Lava bucket with a burn time of 60 seconds. Will become an empty bucket
if used:

```lua
{
    type = "fuel",
    recipe = "example:lava_bucket",
    burntime = 60.0,
    replacements = {{"example:lava_bucket", "example:empty_bucket"}},
}
```

Ore definition
--------------

Used by `core.register_ore`.

See [Ores] section above for essential information.

```lua
{
    ore_type = "",
    -- Supported: "scatter", "sheet", "puff", "blob", "vein", "stratum"

    ore = "",
    -- Ore node to place

    ore_param2 = 0,
    -- Param2 to set for ore (e.g. facedir rotation)

    wherein = "",
    -- Node to place ore in. Multiple are possible by passing a list.

    clust_scarcity = 8 * 8 * 8,
    -- Ore has a 1 out of clust_scarcity chance of spawning in a node.
    -- If the desired average distance between ores is 'd', set this to
    -- d * d * d.

    clust_num_ores = 8,
    -- Number of ores in a cluster

    clust_size = 3,
    -- Size of the bounding box of the cluster.
    -- In this example, there is a 3 * 3 * 3 cluster where 8 out of the 27
    -- nodes are coal ore.

    y_min = -31000,
    y_max = 31000,
    -- Lower and upper limits for ore (inclusive)

    flags = "",
    -- Attributes for the ore generation, see 'Ore attributes' section above

    noise_threshold = 0,
    -- If noise is above this threshold, ore is placed. Not needed for a
    -- uniform distribution.

    noise_params = {
        offset = 0,
        scale = 1,
        spread = {x = 100, y = 100, z = 100},
        seed = 23,
        octaves = 3,
        persistence = 0.7
    },
    -- NoiseParams structure describing one of the perlin noises used for
    -- ore distribution.
    -- Needed by "sheet", "puff", "blob" and "vein" ores.
    -- Omit from "scatter" ore for a uniform ore distribution.
    -- Omit from "stratum" ore for a simple horizontal strata from y_min to
    -- y_max.

    biomes = {"desert", "rainforest"},
    -- List of biomes in which this ore occurs.
    -- Occurs in all biomes if this is omitted, and ignored if the Mapgen
    -- being used does not support biomes.
    -- Can be a list of (or a single) biome names, IDs, or definitions.

    -- Type-specific parameters

    -- "sheet"
    column_height_min = 1,
    column_height_max = 16,
    column_midpoint_factor = 0.5,

    -- "puff"
    np_puff_top = {
        offset = 4,
        scale = 2,
        spread = {x = 100, y = 100, z = 100},
        seed = 47,
        octaves = 3,
        persistence = 0.7
    },
    np_puff_bottom = {
        offset = 4,
        scale = 2,
        spread = {x = 100, y = 100, z = 100},
        seed = 11,
        octaves = 3,
        persistence = 0.7
    },

    -- "vein"
    random_factor = 1.0,

    -- "stratum"
    np_stratum_thickness = {
        offset = 8,
        scale = 4,
        spread = {x = 100, y = 100, z = 100},
        seed = 17,
        octaves = 3,
        persistence = 0.7
    },
    stratum_thickness = 8, -- only used if no noise defined
}
```

Biome definition
----------------

Used by `core.register_biome`.

The maximum number of biomes that can be used is 65535. However, using an
excessive number of biomes will slow down map generation. Depending on desired
performance and computing power the practical limit is much lower.

```lua
{
    name = "tundra",

    node_dust = "default:snow",
    -- Node dropped onto upper surface after all else is generated

    node_top = "default:dirt_with_snow",
    depth_top = 1,
    -- Node forming surface layer of biome and thickness of this layer

    node_filler = "default:permafrost",
    depth_filler = 3,
    -- Node forming lower layer of biome and thickness of this layer

    node_stone = "default:bluestone",
    -- Node that replaces all stone nodes between roughly y_min and y_max.

    node_water_top = "default:ice",
    depth_water_top = 10,
    -- Node forming a surface layer in seawater with the defined thickness

    node_water = "",
    -- Node that replaces all seawater nodes not in the surface layer

    node_river_water = "default:ice",
    -- Node that replaces river water in mapgens that use
    -- default:river_water

    node_riverbed = "default:gravel",
    depth_riverbed = 2,
    -- Node placed under river water and thickness of this layer

    node_cave_liquid = "default:lava_source",
    node_cave_liquid = {"default:water_source", "default:lava_source"},
    -- Nodes placed inside 50% of the medium size caves.
    -- Multiple nodes can be specified, each cave will use a randomly
    -- chosen node from the list.
    -- If this field is left out or 'nil', cave liquids fall back to
    -- classic behavior of lava and water distributed using 3D noise.
    -- For no cave liquid, specify "air".

    node_dungeon = "default:cobble",
    -- Node used for primary dungeon structure.
    -- If absent, dungeon nodes fall back to the 'mapgen_cobble' mapgen
    -- alias, if that is also absent, dungeon nodes fall back to the biome
    -- 'node_stone'.
    -- If present, the following two nodes are also used.

    node_dungeon_alt = "default:mossycobble",
    -- Node used for randomly-distributed alternative structure nodes.
    -- If alternative structure nodes are not wanted leave this absent.

    node_dungeon_stair = "stairs:stair_cobble",
    -- Node used for dungeon stairs.
    -- If absent, stairs fall back to 'node_dungeon'.

    y_max = 31000,
    y_min = 1,
    -- Upper and lower limits for biome.
    -- Alternatively you can use xyz limits as shown below.

    max_pos = {x = 31000, y = 128, z = 31000},
    min_pos = {x = -31000, y = 9, z = -31000},
    -- xyz limits for biome, an alternative to using 'y_min' and 'y_max'.
    -- Biome is limited to a cuboid defined by these positions.
    -- Any x, y or z field left undefined defaults to -31000 in 'min_pos' or
    -- 31000 in 'max_pos'.

    vertical_blend = 8,
    -- Vertical distance in nodes above 'y_max' over which the biome will
    -- blend with the biome above.
    -- Set to 0 for no vertical blend. Defaults to 0.

    heat_point = 0,
    humidity_point = 50,
    -- Characteristic temperature and humidity for the biome.
    -- These values create 'biome points' on a voronoi diagram with heat and
    -- humidity as axes. The resulting voronoi cells determine the
    -- distribution of the biomes.
    -- Heat and humidity have average values of 50, vary mostly between
    -- 0 and 100 but can exceed these values.

    weight = 1.0,
    -- Relative weight of the biome in the Voronoi diagram.
    -- A value of 0 (or less) is ignored and equivalent to 1.0.
}
```

Decoration definition
---------------------

See [Decoration types]. Used by `core.register_decoration`.

```lua
{
    deco_type = "simple",
    -- Type. "simple", "schematic" or "lsystem" supported

    place_on = "default:dirt_with_grass",
    -- Node (or list of nodes) that the decoration can be placed on

    sidelen = 8,
    -- Size of the square (X / Z) divisions of the mapchunk being generated.
    -- Determines the resolution of noise variation if used.
    -- If the chunk size is not evenly divisible by sidelen, sidelen is made
    -- equal to the chunk size.

    fill_ratio = 0.02,
    -- The value determines 'decorations per surface node'.
    -- Used only if noise_params is not specified.
    -- If >= 10.0 complete coverage is enabled and decoration placement uses
    -- a different and much faster method.

    noise_params = {
        offset = 0,
        scale = 0.45,
        spread = {x = 100, y = 100, z = 100},
        seed = 354,
        octaves = 3,
        persistence = 0.7,
        lacunarity = 2.0,
        flags = "absvalue"
    },
    -- NoiseParams structure describing the perlin noise used for decoration
    -- distribution.
    -- A noise value is calculated for each square division and determines
    -- 'decorations per surface node' within each division.
    -- If the noise value >= 10.0 complete coverage is enabled and
    -- decoration placement uses a different and much faster method.

    biomes = {"Oceanside", "Hills", "Plains"},
    -- List of biomes in which this decoration occurs. Occurs in all biomes
    -- if this is omitted, and ignored if the Mapgen being used does not
    -- support biomes.
    -- Can be a list of (or a single) biome names, IDs, or definitions.

    y_min = -31000,
    y_max = 31000,
    -- Lower and upper limits for decoration (inclusive).
    -- These parameters refer to the Y coordinate of the 'place_on' node.

    spawn_by = "default:water",
    -- Node (or list of nodes) that the decoration only spawns next to.
    -- Checks the 8 neighboring nodes on the same height,
    -- and also the ones at the height plus the check_offset, excluding both center nodes.

    check_offset = -1,
    -- Specifies the offset that spawn_by should also check
    -- The default value of -1 is useful to e.g check for water next to the base node.
    -- 0 disables additional checks, valid values: {-1, 0, 1}

    num_spawn_by = 1,
    -- Number of spawn_by nodes that must be surrounding the decoration
    -- position to occur.
    -- If absent or -1, decorations occur next to any nodes.

    flags = "liquid_surface, force_placement, all_floors, all_ceilings",
    -- Flags for all decoration types.
    -- "liquid_surface": Find the highest liquid (not solid) surface under
    --   open air. Search stops and fails on the first solid node.
    --   Cannot be used with "all_floors" or "all_ceilings" below.
    -- "force_placement": Nodes other than "air" and "ignore" are replaced
    --   by the decoration.
    -- "all_floors", "all_ceilings": Instead of placement on the highest
    --   surface in a mapchunk the decoration is placed on all floor and/or
    --   ceiling surfaces, for example in caves and dungeons.
    --   Ceiling decorations act as an inversion of floor decorations so the
    --   effect of 'place_offset_y' is inverted.
    --   Y-slice probabilities do not function correctly for ceiling
    --   schematic decorations as the behavior is unchanged.
    --   If a single decoration registration has both flags the floor and
    --   ceiling decorations will be aligned vertically.

    ----- Simple-type parameters

    decoration = "default:grass",
    -- The node name used as the decoration.
    -- If instead a list of strings, a randomly selected node from the list
    -- is placed as the decoration.

    height = 1,
    -- Decoration height in nodes.
    -- If height_max is not 0, this is the lower limit of a randomly
    -- selected height.

    height_max = 0,
    -- Upper limit of the randomly selected height.
    -- If absent, the parameter 'height' is used as a constant.

    param2 = 0,
    -- Param2 value of decoration nodes.
    -- If param2_max is not 0, this is the lower limit of a randomly
    -- selected param2.

    param2_max = 0,
    -- Upper limit of the randomly selected param2.
    -- If absent, the parameter 'param2' is used as a constant.

    place_offset_y = 0,
    -- Y offset of the decoration base node relative to the standard base
    -- node position.
    -- Can be positive or negative. Default is 0.
    -- Effect is inverted for "all_ceilings" decorations.
    -- Ignored by 'y_min', 'y_max' and 'spawn_by' checks, which always refer
    -- to the 'place_on' node.

    ----- Schematic-type parameters

    schematic = "foobar.mts",
    -- If schematic is a string, it is the filepath relative to the current
    -- working directory of the specified Luanti schematic file.
    -- Could also be the ID of a previously registered schematic.

    schematic = {
        size = {x = 4, y = 6, z = 4},
        data = {
            {name = "default:cobble", param1 = 255, param2 = 0},
            {name = "default:dirt_with_grass", param1 = 255, param2 = 0},
            {name = "air", param1 = 255, param2 = 0},
              ...
        },
        yslice_prob = {
            {ypos = 2, prob = 128},
            {ypos = 5, prob = 64},
              ...
        },
    },
    -- Alternative schematic specification by supplying a table. The fields
    -- size and data are mandatory whereas yslice_prob is optional.
    -- See 'Schematic specifier' for details.

    replacements = {["oldname"] = "convert_to", ...},
    -- Map of node names to replace in the schematic after reading it.

    flags = "place_center_x, place_center_y, place_center_z",
    -- Flags for schematic decorations. See 'Schematic attributes'.

    rotation = "90",
    -- Rotation can be "0", "90", "180", "270", or "random"

    place_offset_y = 0,
    -- If the flag 'place_center_y' is set this parameter is ignored.
    -- Y offset of the schematic base node layer relative to the 'place_on'
    -- node.
    -- Can be positive or negative. Default is 0.
    -- Effect is inverted for "all_ceilings" decorations.
    -- Ignored by 'y_min', 'y_max' and 'spawn_by' checks, which always refer
    -- to the 'place_on' node.

    ----- L-system-type parameters

    treedef = {},
    -- Same as for `core.spawn_tree`.
    -- See section [L-system trees] for more details.
}
```

Chat command definition
-----------------------

Used by `core.register_chatcommand`.

Specifies the function to be called and the privileges required when a player
issues the command.  A help message that is the concatenation of the params and
description fields is shown when the "/help" chatcommand is issued.

```lua
{
    params = "",
    -- Short parameter description.  See the below note.

    description = "",
    -- General description of the command's purpose.

    privs = {},
    -- Required privileges to run. See `core.check_player_privs()` for
    -- the format and see [Privileges] for an overview of privileges.

    func = function(name, param),
    -- Called when command is run.
    -- * `name` is the name of the player who issued the command.
    -- * `param` is a string with the full arguments to the command.
    -- Returns a boolean for success and a string value.
    -- The string is shown to the issuing player upon exit of `func` or,
    -- if `func` returns `false` and no string, the help message is shown.
}
```

Note that in params, the conventional use of symbols is as follows:

* `<>` signifies a placeholder to be replaced when the command is used. For
  example, when a player name is needed: `<name>`
* `[]` signifies param is optional and not required when the command is used.
  For example, if you require param1 but param2 is optional:
  `<param1> [<param2>]`
* `|` signifies exclusive or. The command requires one param from the options
  provided. For example: `<param1> | <param2>`
* `()` signifies grouping. For example, when param1 and param2 are both
  required, or only param3 is required: `(<param1> <param2>) | <param3>`

Example:

```lua
{
    params = "<name> <privilege>",

    description = "Remove privilege from player",

    privs = {privs=true},  -- Require the "privs" privilege to run

    func = function(name, param),
}
```

Privilege definition
--------------------

Used by `core.register_privilege`.

```lua
{
    description = "",
    -- Privilege description

    give_to_singleplayer = true,
    -- Whether to grant the privilege to singleplayer.

    give_to_admin = true,
    -- Whether to grant the privilege to the server admin.
    -- Uses value of 'give_to_singleplayer' by default.

    on_grant = function(name, granter_name),
    -- Called when given to player 'name' by 'granter_name'.
    -- 'granter_name' will be nil if the priv was granted by a mod.

    on_revoke = function(name, revoker_name),
    -- Called when taken from player 'name' by 'revoker_name'.
    -- 'revoker_name' will be nil if the priv was revoked by a mod.

    -- Note that the above two callbacks will be called twice if a player is
    -- responsible, once with the player name, and then with a nil player
    -- name.
    -- Return true in the above callbacks to stop register_on_priv_grant or
    -- revoke being called.
}
```

Detached inventory callbacks
----------------------------

Used by `core.create_detached_inventory`.

```lua
{
    allow_move = function(inv, from_list, from_index, to_list, to_index, count, player),
    -- Called when a player wants to move items inside the inventory.
    -- Return value: number of items allowed to move.

    allow_put = function(inv, listname, index, stack, player),
    -- Called when a player wants to put something into the inventory.
    -- Return value: number of items allowed to put.
    -- Return value -1: Allow and don't modify item count in inventory.

    allow_take = function(inv, listname, index, stack, player),
    -- Called when a player wants to take something out of the inventory.
    -- Return value: number of items allowed to take.
    -- Return value -1: Allow and don't modify item count in inventory.

    on_move = function(inv, from_list, from_index, to_list, to_index, count, player),
    on_put = function(inv, listname, index, stack, player),
    on_take = function(inv, listname, index, stack, player),
    -- Called after the actual action has happened, according to what was
    -- allowed.
    -- No return value.
}
```

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

Particle definition
-------------------

Used by `core.add_particle`.

```lua
{
    pos = {x=0, y=0, z=0},
    velocity = {x=0, y=0, z=0},
    acceleration = {x=0, y=0, z=0},
    -- Spawn particle at pos with velocity and acceleration

    expirationtime = 1,
    -- Disappears after expirationtime seconds

    size = 1,
    -- Scales the visual size of the particle texture.
    -- If `node` is set, size can be set to 0 to spawn a randomly-sized
    -- particle (just like actual node dig particles).

    collisiondetection = false,
    -- If true collides with `walkable` nodes and, depending on the
    -- `object_collision` field, objects too.

    collision_removal = false,
    -- If true particle is removed when it collides.
    -- Requires collisiondetection = true to have any effect.

    object_collision = false,
    -- If true particle collides with objects that are defined as
    -- `physical = true,` and `collide_with_objects = true,`.
    -- Requires collisiondetection = true to have any effect.

    vertical = false,
    -- If true faces player using y axis only

    texture = "image.png",
    -- The texture of the particle
    -- v5.6.0 and later: also supports the table format described in the
    -- following section, but due to a bug this did not take effect
    -- (beyond the texture name).
    -- v5.9.0 and later: fixes the bug.
    -- Note: "texture.animation" is ignored here. Use "animation" below instead.

    playername = "singleplayer",
    -- Optional, if specified spawns particle only on the player's client

    animation = {Tile Animation definition},
    -- Optional, specifies how to animate the particle texture

    glow = 0
    -- Optional, specify particle self-luminescence in darkness.
    -- Values 0-14.

    node = {name = "ignore", param2 = 0},
    -- Optional, if specified the particle will have the same appearance as
    -- node dig particles for the given node.
    -- `texture` and `animation` will be ignored if this is set.

    node_tile = 0,
    -- Optional, only valid in combination with `node`
    -- If set to a valid number 1-6, specifies the tile from which the
    -- particle texture is picked.
    -- Otherwise, the default behavior is used. (currently: any random tile)

    drag = {x=0, y=0, z=0},
    -- v5.6.0 and later: Optional drag value, consult the following section
    -- Note: Only a vector is supported here. Alternative forms like a single
    -- number are not supported.

    jitter = {min = ..., max = ..., bias = 0},
    -- v5.6.0 and later: Optional jitter range, consult the following section

    bounce = {min = ..., max = ..., bias = 0},
    -- v5.6.0 and later: Optional bounce range, consult the following section
}
```


`ParticleSpawner` definition
----------------------------

Used by `core.add_particlespawner`.

Before v5.6.0, particlespawners used a different syntax and had a more limited set
of features. Definition fields that are the same in both legacy and modern versions
are shown in the next listing, and the fields that are used by legacy versions are
shown separated by a comment; the modern fields are too complex to compactly
describe in this manner and are documented after the listing.

The older syntax can be used in combination with the newer syntax (e.g. having
`minpos`, `maxpos`, and `pos` all set) to support older servers. On newer servers,
the new syntax will override the older syntax; on older servers, the newer syntax
will be ignored.

```lua
{
    -------------------
    -- Common fields --
    -------------------
    -- (same name and meaning in both new and legacy syntax)

    amount = 1,
    -- Number of particles spawned over the time period `time`.

    time = 1,
    -- Lifespan of spawner in seconds.
    -- If time is 0 spawner has infinite lifespan and spawns the `amount` on
    -- a per-second basis.

    collisiondetection = false,
    -- If true collide with `walkable` nodes and, depending on the
    -- `object_collision` field, objects too.

    collision_removal = false,
    -- If true particles are removed when they collide.
    -- Requires collisiondetection = true to have any effect.

    object_collision = false,
    -- If true particles collide with objects that are defined as
    -- `physical = true,` and `collide_with_objects = true,`.
    -- Requires collisiondetection = true to have any effect.

    attached = ObjectRef,
    -- If defined, particle positions, velocities and accelerations are
    -- relative to this object's position and yaw

    vertical = false,
    -- If true face player using y axis only

    texture = "image.png",
    -- The texture of the particle
    -- v5.6.0 and later: also supports the table format described in the
    -- following section.

    playername = "singleplayer",
    -- Optional, if specified spawns particles only on the player's client

    animation = {Tile Animation definition},
    -- Optional, specifies how to animate the particles' texture
    -- v5.6.0 and later: set length to -1 to synchronize the length
    -- of the animation with the expiration time of individual particles.
    -- (-2 causes the animation to be played twice, and so on)

    glow = 0,
    -- Optional, specify particle self-luminescence in darkness.
    -- Values 0-14.

    node = {name = "ignore", param2 = 0},
    -- Optional, if specified the particles will have the same appearance as
    -- node dig particles for the given node.
    -- `texture` and `animation` will be ignored if this is set.

    node_tile = 0,
    -- Optional, only valid in combination with `node`
    -- If set to a valid number 1-6, specifies the tile from which the
    -- particle texture is picked.
    -- Otherwise, the default behavior is used. (currently: any random tile)

    -------------------
    -- Legacy fields --
    -------------------

    minpos = {x=0, y=0, z=0},
    maxpos = {x=0, y=0, z=0},
    minvel = {x=0, y=0, z=0},
    maxvel = {x=0, y=0, z=0},
    minacc = {x=0, y=0, z=0},
    maxacc = {x=0, y=0, z=0},
    minexptime = 1,
    maxexptime = 1,
    minsize = 1,
    maxsize = 1,
    -- The particles' properties are random values between the min and max
    -- values.
    -- applies to: pos, velocity, acceleration, expirationtime, size
    -- If `node` is set, min and maxsize can be set to 0 to spawn
    -- randomly-sized particles (just like actual node dig particles).
}
```

### Modern definition fields

After v5.6.0, spawner properties can be defined in several different ways depending
on the level of control you need. `pos` for instance can be set as a single vector,
in which case all particles will appear at that exact point throughout the lifetime
of the spawner. Alternately, it can be specified as a min-max pair, specifying a
cubic range the particles can appear randomly within. Finally, some properties can
be animated by suffixing their key with `_tween` (e.g. `pos_tween`) and supplying
a tween table.

The following definitions are all equivalent, listed in order of precedence from
lowest (the legacy syntax) to highest (tween tables). If multiple forms of a
property definition are present, the highest-precedence form will be selected
and all lower-precedence fields will be ignored, allowing for graceful
degradation in older clients).

```lua
{
  -- old syntax
  maxpos = {x = 0, y = 0, z = 0},
  minpos = {x = 0, y = 0, z = 0},

  -- absolute value
  pos = 0,
  -- all components of every particle's position vector will be set to this
  -- value

  -- vec3
  pos = vector.new(0,0,0),
  -- all particles will appear at this exact position throughout the lifetime
  -- of the particlespawner

  -- vec3 range
  pos = {
        -- the particle will appear at a position that is picked at random from
        -- within a cubic range

        min = vector.new(0,0,0),
        -- `min` is the minimum value this property will be set to in particles
        -- spawned by the generator

        max = vector.new(0,0,0),
        -- `max` is the minimum value this property will be set to in particles
        -- spawned by the generator

        bias = 0,
        -- when `bias` is 0, all random values are exactly as likely as any
        -- other. when it is positive, the higher it is, the more likely values
        -- will appear towards the minimum end of the allowed spectrum. when
        -- it is negative, the lower it is, the more likely values will appear
        -- towards the maximum end of the allowed spectrum. the curve is
        -- exponential and there is no particular maximum or minimum value
    },

    -- tween table
    pos_tween = {...},
    -- a tween table should consist of a list of frames in the same form as the
    -- untweened pos property above, which the engine will interpolate between,
    -- and optionally a number of properties that control how the interpolation
    -- takes place. currently **only two frames**, the first and the last, are
    -- used, but extra frames are accepted for the sake of forward compatibility.
    -- any of the above definition styles can be used here as well in any combination
    -- supported by the property type

    pos_tween = {
        style = "fwd",
        -- linear animation from first to last frame (default)
        style = "rev",
        -- linear animation from last to first frame
        style = "pulse",
        -- linear animation from first to last then back to first again
        style = "flicker",
        -- like "pulse", but slightly randomized to add a bit of stutter

        reps = 1,
        -- number of times the animation is played over the particle's lifespan

        start = 0.0,
        -- point in the spawner's lifespan at which the animation begins. 0 is
        -- the very beginning, 1 is the very end

        -- frames can be defined in a number of different ways, depending on the
        -- underlying type of the property. for now, all but the first and last
        -- frame are ignored

        -- frames

            -- floats
            0, 0,

            -- vec3s
            vector.new(0,0,0),
            vector.new(0,0,0),

            -- vec3 ranges
            { min = vector.new(0,0,0), max = vector.new(0,0,0), bias = 0 },
            { min = vector.new(0,0,0), max = vector.new(0,0,0), bias = 0 },

            -- mixed
            0, { min = vector.new(0,0,0), max = vector.new(0,0,0), bias = 0 },
    },
}
```

All of the properties that can be defined in this way are listed in the next
section, along with the datatypes they accept.

#### List of particlespawner properties

All properties in this list of type "vec3 range", "float range" or "vec3" can
be animated with `*_tween` tables. For example, `jitter` can be tweened by
setting a `jitter_tween` table instead of (or in addition to) a `jitter`
table/value.

In this section, a float range is a table defined as so: { min = A, max = B }
A and B are your supplemented values. For a vec3 range this means they are vectors.
Types used are defined in the previous section.

* vec3 range `pos`: the position at which particles can appear

* vec3 range `vel`: the initial velocity of the particle

* vec3 range `acc`: the direction and speed with which the particle
  accelerates

* float range `size`: scales the visual size of the particle texture.
  if `node` is set, this can be set to 0 to spawn randomly-sized particles
  (just like actual node dig particles).

* vec3 range `jitter`: offsets the velocity of each particle by a random
  amount within the specified range each frame. used to create Brownian motion.

* vec3 range `drag`: the amount by which absolute particle velocity along
  each axis is decreased per second.  a value of 1.0 means that the particle
  will be slowed to a stop over the space of a second; a value of -1.0 means
  that the particle speed will be doubled every second. to avoid interfering
  with gravity provided by `acc`, a drag vector like `vector.new(1,0,1)` can
  be used instead of a uniform value.

* float range `bounce`: how bouncy the particles are when `collisiondetection`
  is turned on. values less than or equal to `0` turn off particle bounce;
  `1` makes the particles bounce without losing any velocity, and `2` makes
  them double their velocity with every bounce.  `bounce` is not bounded but
  values much larger than `1.0` probably aren't very useful.

* float range `exptime`: the number of seconds after which the particle
  disappears.

* table `attract`: sets the birth orientation of particles relative to various
  shapes defined in world coordinate space. this is an alternative means of
  setting the velocity which allows particles to emerge from or enter into
  some entity or node on the map, rather than simply being assigned random
  velocity values within a range. the velocity calculated by this method will
  be **added** to that specified by `vel` if `vel` is also set, so in most
  cases **`vel` should be set to 0**. `attract` has the fields:

  * string `kind`: selects the kind of shape towards which the particles will
    be oriented. it must have one of the following values:

    * `"none"`: no attractor is set and the `attractor` table is ignored
    * `"point"`: the particles are attracted to a specific point in space.
      use this also if you want a sphere-like effect, in combination with
      the `radius` property.
    * `"line"`: the particles are attracted to an (infinite) line passing
      through the points `origin` and `angle`. use this for e.g. beacon
      effects, energy beam effects, etc.
    * `"plane"`: the particles are attracted to an (infinite) plane on whose
      surface `origin` designates a point in world coordinate space. use this
      for e.g. particles entering or emerging from a portal.

  * float range `strength`: the speed with which particles will move towards
    `attractor`. If negative, the particles will instead move away from that
    point.

  * vec3 `origin`: the origin point of the shape towards which particles will
    initially be oriented. functions as an offset if `origin_attached` is also
    set.

  * vec3 `direction`: sets the direction in which the attractor shape faces. for
    lines, this sets the angle of the line; e.g. a vector of (0,1,0) will
    create a vertical line that passes through `origin`. for planes, `direction`
    is the surface normal of an infinite plane on whose surface `origin` is
    a point. functions as an offset if `direction_attached` is also set.

  * ObjectRef `origin_attached`: allows the origin to be specified as an offset
    from the position of an entity rather than a coordinate in world space.

  * ObjectRef `direction_attached`: allows the direction to be specified as an
    offset from the position of an entity rather than a coordinate in world space.

  * bool `die_on_contact`: if true, the particles' lifetimes are adjusted so
    that they will die as they cross the attractor threshold. this behavior
    is the default but is undesirable for some kinds of animations; set it to
    false to allow particles to live out their natural lives.

* vec3 range `radius`: if set, particles will be arranged in a sphere around
  `pos`. A constant can be used to create a spherical shell of particles, a
  vector to create an ovoid shell, and a range to create a volume; e.g.
  `{min = 0.5, max = 1, bias = 1}` will allow particles to appear between 0.5
  and 1 nodes away from `pos` but will cluster them towards the center of the
  sphere. Usually if `radius` is used, `pos` should be a single point, but it
  can still be a range if you really know what you're doing (e.g. to create a
  "roundcube" emitter volume).

### Textures

In versions before v5.6.0, particle/particlespawner textures could only be
specified as a single texture string. After v5.6.0, textures can now be
specified as a table as well. This table contains options that allow simple
animations to be applied to the texture.

```lua
texture = {
    name = "mymod_particle_texture.png",
    -- the texture specification string

    alpha = 1.0,
    -- controls how visible the particle is; at 1.0 the particle is fully
    -- visible, at 0, it is completely invisible.

    alpha_tween = {1, 0},
    -- can be used instead of `alpha` to animate the alpha value over the
    -- particle's lifetime. these tween tables work identically to the tween
    -- tables used in particlespawner properties, except that time references
    -- are understood with respect to the particle's lifetime, not the
    -- spawner's. {1,0} fades the particle out over its lifetime.

    scale = 1,
    scale = {x = 1, y = 1},
    -- scales the texture onscreen

    scale_tween = {
        {x = 1, y = 1},
        {x = 0, y = 1},
    },
    -- animates the scale over the particle's lifetime. works like the
    -- alpha_tween table, but can accept two-dimensional vectors as well as
    -- integer values. the example value would cause the particle to shrink
    -- in one dimension over the course of its life until it disappears

    blend = "alpha",
    -- (default) blends transparent pixels with those they are drawn atop
    -- according to the alpha channel of the source texture. useful for
    -- e.g. material objects like rocks, dirt, smoke, or node chunks
    -- note: there will be rendering bugs when particles interact with
    -- translucent nodes. particles are also not transparency-sorted
    -- relative to each other.
    blend = "clip",
    -- pixels are either fully opaque or fully transparent,
    -- depending on whether alpha is greater than or less than 50%
    -- (just like `use_texture_alpha = "clip"` for nodes).
    -- you should prefer this if you don't need semi-transparency, as it's faster.
    blend = "add",
    -- adds the value of pixels to those underneath them, modulo the sources
    -- alpha channel. useful for e.g. bright light effects like sparks or fire
    blend = "screen",
    -- like "add" but less bright. useful for subtler light effects. note that
    -- this is NOT formally equivalent to the "screen" effect used in image
    -- editors and compositors, as it does not respect the alpha channel of
    -- of the image being blended
    blend = "sub",
    -- the inverse of "add"; the value of the source pixel is subtracted from
    -- the pixel underneath it. a white pixel will turn whatever is underneath
    -- it black; a black pixel will be "transparent". useful for creating
    -- darkening effects

    animation = {Tile Animation definition},
    -- overrides the particlespawner's global animation property for a single
    -- specific texture
}
```

For particlespawners, it is also possible to set the `texpool` property instead
of a single texture definition. A `texpool` consists of a list of possible
particle textures. Every time a particle is spawned, the engine will pick a
texture at random from the `texpool` and assign it as that particle's texture.
You can also specify a `texture` in addition to a `texpool`; the `texture`
value will be ignored on newer clients but will be sent to older (pre-v5.6.0)
clients that do not implement texpools.

```lua
texpool = {
    "mymod_particle_texture.png";
    { name = "mymod_spark.png", alpha_tween = {1, 0} },
    {
      name = "mymod_dust.png",
      alpha = 0.3,
      scale = 1.5,
      animation = {
            type = "vertical_frames",
            aspect_w = 16, aspect_h = 16,

            length = 3,
            -- the animation lasts for 3s and then repeats
            length = -3,
            -- repeat the animation three times over the particle's lifetime
            -- (post-v5.6.0 clients only)
      },
    },
}
```

#### List of animatable texture properties

While animated particlespawner values vary over the course of the particlespawner's
lifetime, animated texture properties vary over the lifespans of the individual
particles spawned with that texture. So a particle with the texture property

```lua
alpha_tween = {
    0.0, 1.0,
    style = "pulse",
    reps = 4,
}
```

would be invisible at its spawning, pulse visible four times throughout its
lifespan, and then vanish again before expiring.

* float `alpha` (0.0 - 1.0): controls the visibility of the texture
* vec2 `scale`: controls the size of the displayed billboard onscreen. Its units
  are multiples of the parent particle's assigned size (see the `size` property above)

`HTTPRequest` definition
------------------------

Used by `HTTPApiTable.fetch` and `HTTPApiTable.fetch_async`.

```lua
{
    url = "http://example.org",

    timeout = 10,
    -- Timeout for request to be completed in seconds. Default depends on engine settings.

    method = "GET", "POST", "PUT" or "DELETE"
    -- The http method to use. Defaults to "GET".

    data = "Raw request data string" OR {field1 = "data1", field2 = "data2"},
    -- Data for the POST, PUT or DELETE request.
    -- Accepts both a string and a table. If a table is specified, encodes
    -- table as x-www-form-urlencoded key-value pairs.

    user_agent = "ExampleUserAgent",
    -- Optional, if specified replaces the default Luanti user agent with
    -- given string

    extra_headers = { "Accept-Language: en-us", "Accept-Charset: utf-8" },
    -- Optional, if specified adds additional headers to the HTTP request.
    -- You must make sure that the header strings follow HTTP specification
    -- ("Key: Value").

    multipart = boolean
    -- Optional, if true performs a multipart HTTP request.
    -- Default is false.
    -- Post only, data must be array

    post_data = "Raw POST request data string" OR {field1 = "data1", field2 = "data2"},
    -- Deprecated, use `data` instead. Forces `method = "POST"`.
}
```

`HTTPRequestResult` definition
------------------------------

Passed to `HTTPApiTable.fetch` callback. Returned by
`HTTPApiTable.fetch_async_get`.

```lua
{
    completed = true,
    -- If true, the request has finished (either succeeded, failed or timed
    -- out)

    succeeded = true,
    -- If true, the request was successful

    timeout = false,
    -- If true, the request timed out

    code = 200,
    -- HTTP status code

    data = "response"
}
```

Authentication handler definition
---------------------------------

Used by `core.register_authentication_handler`.

```lua
{
    get_auth = function(name),
    -- Get authentication data for existing player `name` (`nil` if player
    -- doesn't exist).
    -- Returns following structure:
    -- `{password=<string>, privileges=<table>, last_login=<number or nil>}`

    create_auth = function(name, password),
    -- Create new auth data for player `name`.
    -- Note that `password` is not plain-text but an arbitrary
    -- representation decided by the engine.

    delete_auth = function(name),
    -- Delete auth data of player `name`.
    -- Returns boolean indicating success (false if player is nonexistent).

    set_password = function(name, password),
    -- Set password of player `name` to `password`.
    -- Auth data should be created if not present.

    set_privileges = function(name, privileges),
    -- Set privileges of player `name`.
    -- `privileges` is in table form: keys are privilege names, values are `true`;
    -- auth data should be created if not present.

    reload = function(),
    -- Reload authentication data from the storage location.
    -- Returns boolean indicating success.

    record_login = function(name),
    -- Called when player joins, used for keeping track of last_login

    iterate = function(),
    -- Returns an iterator (use with `for` loops) for all player names
    -- currently in the auth database
}
```

Bit Library
-----------

Functions: bit.tobit, bit.tohex, bit.bnot, bit.band, bit.bor, bit.bxor, bit.lshift, bit.rshift, bit.arshift, bit.rol, bit.ror, bit.bswap

See http://bitop.luajit.org/ for advanced information.

Tracy Profiler
--------------

Luanti can be built with support for the Tracy profiler, which can also be
useful for profiling mods and is exposed to Lua as the global `tracy`.

See doc/developing/misc.md for details.

Note: This is a development feature and not covered by compatibility promises.

Error Handling
--------------

When an error occurs that is not caught, Luanti calls the function
`core.error_handler` with the error object as its first argument. The second
argument is the stack level where the error occurred. The return value is the
error string that should be shown. By default this is a backtrace from
`debug.traceback`. If the error object is not a string, it is first converted
with `tostring` before being displayed. This means that you can use tables as
error objects so long as you give them `__tostring` metamethods.

You can override `core.error_handler`. You should call the previous handler
with the correct stack level in your implementation.
