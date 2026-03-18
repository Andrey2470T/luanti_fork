Global objects
--------------

* `core.env`: `EnvRef` of the server environment and world.
    * Any function in the `core` namespace can be called using the syntax
      `core.env:somefunction(somearguments)`
      instead of `core.somefunction(somearguments)`
    * Deprecated, but support is not to be dropped soon
* `minetest`: alias for the `core` namespace
    * Deprecated, but support is not to be dropped soon

Global tables
-------------

### Registered definition tables

* `core.registered_items`
    * Map of registered items, indexed by name
* `core.registered_nodes`
    * Map of registered node definitions, indexed by name
* `core.registered_craftitems`
    * Map of registered craft item definitions, indexed by name
* `core.registered_tools`
    * Map of registered tool definitions, indexed by name
* `core.registered_entities`
    * Map of registered entity prototypes, indexed by name
    * Values in this table may be modified directly.
      Note: changes to initial properties will only affect entities spawned afterwards,
      as they are only read when spawning.
* `core.object_refs`
    * Map of object references, indexed by active object id
* `core.luaentities`
    * Map of Lua entities, indexed by active object id
* `core.registered_abms`
    * List of ABM definitions
* `core.registered_lbms`
    * List of LBM definitions
* `core.registered_aliases`
    * Map of registered aliases, indexed by name
* `core.registered_ores`
    * Map of registered ore definitions, indexed by the `name` field.
    * If `name` is nil, the key is the object handle returned by
      `core.register_ore`.
* `core.registered_biomes`
    * Map of registered biome definitions, indexed by the `name` field.
    * If `name` is nil, the key is the object handle returned by
      `core.register_biome`.
* `core.registered_decorations`
    * Map of registered decoration definitions, indexed by the `name` field.
    * If `name` is nil, the key is the object handle returned by
      `core.register_decoration`.
* `core.registered_chatcommands`
    * Map of registered chat command definitions, indexed by name
* `core.registered_privileges`
    * Map of registered privilege definitions, indexed by name
    * Registered privileges can be modified directly in this table.

### Registered callback tables

All callbacks registered with [Global callback registration functions] are added
to corresponding `core.registered_*` tables.

For historical reasons, the use of an -s suffix in these names is inconsistent.

* `core.registered_on_chat_messages`
* `core.registered_on_chatcommands`
* `core.registered_globalsteps`
* `core.registered_on_punchnodes`
* `core.registered_on_placenodes`
* `core.registered_on_dignodes`
* `core.registered_on_generateds`
* `core.registered_on_newplayers`
* `core.registered_on_dieplayers`
* `core.registered_on_respawnplayers`
* `core.registered_on_prejoinplayers`
* `core.registered_on_joinplayers`
* `core.registered_on_leaveplayers`
* `core.registered_on_player_receive_fields`
* `core.registered_on_cheats`
* `core.registered_on_crafts`
* `core.registered_craft_predicts`
* `core.registered_on_item_eats`
* `core.registered_on_item_pickups`
* `core.registered_on_punchplayers`
* `core.registered_on_authplayers`
* `core.registered_on_player_inventory_actions`
* `core.registered_allow_player_inventory_actions`
* `core.registered_on_rightclickplayers`
* `core.registered_on_mods_loaded`
* `core.registered_on_shutdown`
* `core.registered_on_protection_violation`
* `core.registered_on_priv_grant`
* `core.registered_on_priv_revoke`
* `core.registered_can_bypass_userlimit`
* `core.registered_on_modchannel_message`
* `core.registered_on_liquid_transformed`
* `core.registered_on_mapblocks_changed`
