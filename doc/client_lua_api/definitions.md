Definitions
===========

* `core.get_node_def(nodename)`
    * Returns [node definition](#node-definition) table of `nodename`
* `core.get_item_def(itemstring)`
    * Returns item definition table of `itemstring`

#### Node Definition

```lua
{
    has_on_construct = bool,        -- Whether the node has the on_construct callback defined
    has_on_destruct = bool,         -- Whether the node has the on_destruct callback defined
    has_after_destruct = bool,      -- Whether the node has the after_destruct callback defined
    name = string,                  -- The name of the node e.g. "air", "default:dirt"
    groups = table,                 -- The groups of the node
    paramtype = string,             -- Paramtype of the node
    paramtype2 = string,            -- ParamType2 of the node
    drawtype = string,              -- Drawtype of the node
    mesh = <string>,                -- Mesh name if existent
    minimap_color = <Color>,        -- Color of node on minimap *May not exist*
    visual_scale = number,          -- Visual scale of node
    alpha = number,                 -- Alpha of the node. Only used for liquids
    color = <Color>,                -- Color of node *May not exist*
    palette_name = <string>,        -- Filename of palette *May not exist*
    palette = <{                    -- List of colors
        Color,
        Color
    }>,
    waving = number,                -- 0 of not waving, 1 if waving
    connect_sides = number,         -- Used for connected nodes
    connects_to = {                 -- List of nodes to connect to
        "node1",
        "node2"
    },
    post_effect_color = Color,      -- Color overlaid on the screen when the player is in the node
    leveled = number,               -- Max level for node
    sunlight_propogates = bool,     -- Whether light passes through the block
    light_source = number,          -- Light emitted by the block
    is_ground_content = bool,       -- Whether caves should cut through the node
    walkable = bool,                -- Whether the player collides with the node
    pointable = bool,               -- Whether the player can select the node
    diggable = bool,                -- Whether the player can dig the node
    climbable = bool,               -- Whether the player can climb up the node
    buildable_to = bool,            -- Whether the player can replace the node by placing a node on it
    rightclickable = bool,          -- Whether the player can place nodes pointing at this node
    damage_per_second = number,     -- HP of damage per second when the player is in the node
    liquid_type = <string>,         -- A string containing "none", "flowing", or "source" *May not exist*
    liquid_alternative_flowing = <string>, -- Alternative node for liquid *May not exist*
    liquid_alternative_source = <string>, -- Alternative node for liquid *May not exist*
    liquid_viscosity = <number>,    -- How slow the liquid flows *May not exist*
    liquid_renewable = <boolean>,   -- Whether the liquid makes an infinite source *May not exist*
    liquid_range = <number>,        -- How far the liquid flows *May not exist*
    drowning = bool,                -- Whether the player will drown in the node
    floodable = bool,               -- Whether nodes will be replaced by liquids (flooded)
    node_box = table,               -- Nodebox to draw the node with
    collision_box = table,          -- Nodebox to set the collision area
    selection_box = table,          -- Nodebox to set the area selected by the player
    sounds = {                      -- Table of sounds that the block makes
        sound_footstep = SimpleSoundSpec,
        sound_dig = SimpleSoundSpec,
        sound_dug = SimpleSoundSpec
    },
    legacy_facedir_simple = bool,   -- Whether to use old facedir
    legacy_wallmounted = bool       -- Whether to use old wallmounted
    move_resistance = <number>,     -- How slow players can move through the node *May not exist*
}
```

#### Item Definition

```lua
{
    name = string,                  -- Name of the item e.g. "default:stone"
    description = string,           -- Description of the item e.g. "Stone"
    type = string,                  -- Item type: "none", "node", "craftitem", "tool"
    inventory_image = string,       -- Image in the inventory
    wield_image = string,           -- Image in wieldmesh
    palette_image = string,         -- Image for palette
    color = Color,                  -- Color for item
    wield_scale = Vector,           -- Wieldmesh scale
    stack_max = number,             -- Number of items stackable together
    usable = bool,                  -- Has on_use callback defined
    liquids_pointable = bool,       -- Whether you can point at liquids with the item
    tool_capabilities = <table>,    -- If the item is a tool, tool capabilities of the item
    groups = table,                 -- Groups of the item
    sound_place = SimpleSoundSpec,  -- Sound played when placed
    sound_place_failed = SimpleSoundSpec, -- Sound played when placement failed
    node_placement_prediction = string -- Node placed in client until server catches up
}
```
-----------------

### Chat command definition (`register_chatcommand`)

```lua
{
    params = "<name> <privilege>", -- Short parameter description
    description = "Remove privilege from player", -- Full description
    func = function(param),        -- Called when command is run.
                                   -- Returns boolean success and text output.
}
```

### Server info

```lua
{
    address = "luanti.example.org",   -- The domain name/IP address of a remote server or "" for a local server.
    ip = "203.0.113.156",             -- The IP address of the server.
    port = 30000,                     -- The port the client is connected to.
    protocol_version = 30             -- Will not be accurate at start up as the client might not be connected to the server yet, in that case it will be 0.
}
```

### HUD Definition (`hud_add`, `hud_get`)

Refer to `server_lua_api/definition_tables/hud.md`.

Escape sequences
----------------
Most text can contain escape sequences that can for example color the text.
There are a few exceptions: tab headers, dropdowns and vertical labels can't.
The following functions provide escape sequences:
* `core.get_color_escape_sequence(color)`:
    * `color` is a [ColorString](#colorstring)
    * The escape sequence sets the text color to `color`
* `core.colorize(color, message)`:
    * Equivalent to:
      `core.get_color_escape_sequence(color) ..
       message ..
       core.get_color_escape_sequence("#ffffff")`
* `core.get_background_escape_sequence(color)`
    * `color` is a [ColorString](#colorstring)
    * The escape sequence sets the background of the whole text element to
      `color`. Only defined for item descriptions and tooltips.
* `core.strip_foreground_colors(str)`
    * Removes foreground colors added by `get_color_escape_sequence`.
* `core.strip_background_colors(str)`
    * Removes background colors added by `get_background_escape_sequence`.
* `core.strip_colors(str)`
    * Removes all color escape sequences.

`ColorString`
-------------

Refer to `server_lua_api/colors.md`.

`Color`
-------------
`{a = alpha, r = red, g = green, b = blue}` defines an ARGB8 color.

### Particle definition (`add_particle`)

As documented in `server_lua_api/definition_tables/particle.md`, except for obvious reasons, the `playername` field is not supported.

### `ParticleSpawner` definition (`add_particlespawner`)

As documented in `server_lua_api/definition_tables/particle.md`, except for obvious reasons, the `playername` field is not supported.
