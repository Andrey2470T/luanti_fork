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
