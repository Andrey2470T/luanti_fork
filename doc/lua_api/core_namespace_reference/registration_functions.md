Registration functions
======================

Call these functions only at load time!

### Environment

* `core.register_node(name, node definition)`
* `core.register_craftitem(name, item definition)`
* `core.register_tool(name, item definition)`
* `core.override_item(name, redefinition, del_fields)`
    * `redefinition` is a table of fields `[name] = new_value`,
      overwriting fields of or adding fields to the existing definition.
    * `del_fields` is a list of field names to be set
      to `nil` ("deleted from") the original definition.
    * Overrides fields of an item registered with register_node/tool/craftitem.
    * Note: Item must already be defined, (opt)depend on the mod defining it.
    * Example: `core.override_item("default:mese",
      {light_source=core.LIGHT_MAX}, {"sounds"})`:
      Overwrites the `light_source` field,
      removes the sounds from the definition of the mese block.
* `core.unregister_item(name)`
    * Unregisters the item from the engine, and deletes the entry with key
      `name` from `core.registered_items` and from the associated item table
      according to its nature: `core.registered_nodes`, etc.
* `core.register_entity(name, entity definition)`
* `core.register_abm(abm definition)`
* `core.register_lbm(lbm definition)`
* `core.register_alias(alias, original_name)`
    * Also use this to set the 'mapgen aliases' needed in a game for the core
      mapgens. See [Mapgen aliases] section above.
* `core.register_alias_force(alias, original_name)`
* `core.register_ore(ore definition)`
    * Returns an integer object handle uniquely identifying the registered
      ore on success.
    * The order of ore registrations determines the order of ore generation.
* `core.register_biome(biome definition)`
    * Returns an integer object handle uniquely identifying the registered
      biome on success. To get the biome ID, use `core.get_biome_id`.
* `core.unregister_biome(name)`
    * Unregisters the biome from the engine, and deletes the entry with key
      `name` from `core.registered_biomes`.
    * Warning: This alters the biome to biome ID correspondences, so any
      decorations or ores using the 'biomes' field must afterwards be cleared
      and re-registered.
* `core.register_decoration(decoration definition)`
    * Returns an integer object handle uniquely identifying the registered
      decoration on success. To get the decoration ID, use
      `core.get_decoration_id`.
    * The order of decoration registrations determines the order of decoration
      generation.
* `core.register_schematic(schematic definition)`
    * Returns an integer object handle uniquely identifying the registered
      schematic on success.
    * If the schematic is loaded from a file, the `name` field is set to the
      filename.
    * If the function is called when loading the mod, and `name` is a relative
      path, then the current mod path will be prepended to the schematic
      filename.
* `core.clear_registered_biomes()`
    * Clears all biomes currently registered.
    * Warning: Clearing and re-registering biomes alters the biome to biome ID
      correspondences, so any decorations or ores using the 'biomes' field must
      afterwards be cleared and re-registered.
* `core.clear_registered_decorations()`
    * Clears all decorations currently registered.
* `core.clear_registered_ores()`
    * Clears all ores currently registered.
* `core.clear_registered_schematics()`
    * Clears all schematics currently registered.

### Gameplay

* `core.register_craft(recipe)`
    * Check recipe table syntax for different types below.
* `core.clear_craft(recipe)`
    * Will erase existing craft based either on output item or on input recipe.
    * Specify either output or input only. If you specify both, input will be
      ignored. For input use the same recipe table syntax as for
      `core.register_craft(recipe)`. For output specify only the item,
      without a quantity.
    * Returns false if no erase candidate could be found, otherwise returns true.
    * **Warning**! The type field ("shaped", "cooking" or any other) will be
      ignored if the recipe contains output. Erasing is then done independently
      from the crafting method.
* `core.register_chatcommand(cmd, chatcommand definition)`
* `core.override_chatcommand(name, redefinition)`
    * Overrides fields of a chatcommand registered with `register_chatcommand`.
* `core.unregister_chatcommand(name)`
    * Unregisters a chatcommands registered with `register_chatcommand`.
* `core.register_privilege(name, definition)`
    * `definition` can be a description or a definition table (see [Privilege
      definition]).
    * If it is a description, the priv will be granted to singleplayer and admin
      by default.
    * To allow players with `basic_privs` to grant, see the `basic_privs`
      minetest.conf setting.
* `core.register_authentication_handler(authentication handler definition)`
    * Registers an auth handler that overrides the builtin one.
    * This function can be called by a single mod once only.

Global callback registration functions
--------------------------------------

Call these functions only at load time!

* `core.register_globalstep(function(dtime))`
    * Called every server step, usually interval of 0.1s.
    * `dtime` is the time since last execution in seconds.
* `core.register_on_mods_loaded(function())`
    * Called after mods have finished loading and before the media is cached or the
      aliases handled.
* `core.register_on_shutdown(function())`
    * Called before server shutdown
    * Players that were kicked by the shutdown procedure are still fully accessible
     in `core.get_connected_players()`.
    * **Warning**: If the server terminates abnormally (i.e. crashes), the
      registered callbacks **will likely not be run**. Data should be saved at
      semi-frequent intervals as well as on server shutdown.
* `core.register_on_placenode(function(pos, newnode, placer, oldnode, itemstack, pointed_thing))`
    * Called when a node has been placed
    * If return `true` no item is taken from `itemstack`
    * `placer` may be any valid ObjectRef or nil.
    * **Not recommended**; use `on_construct` or `after_place_node` in node
      definition whenever possible.
* `core.register_on_dignode(function(pos, oldnode, digger))`
    * Called when a node has been dug.
    * **Not recommended**; Use `on_destruct` or `after_dig_node` in node
      definition whenever possible.
* `core.register_on_punchnode(function(pos, node, puncher, pointed_thing))`
    * Called when a node is punched
* `core.register_on_generated(function(minp, maxp, blockseed))`
    * Called after generating a piece of world between `minp` and `maxp`.
    * **Avoid using this** whenever possible. As with other callbacks this blocks
      the main thread and introduces noticeable latency.
      Consider [Mapgen environment] for an alternative.
* `core.register_on_newplayer(function(ObjectRef))`
    * Called when a new player enters the world for the first time
* `core.register_on_punchplayer(function(player, hitter, time_from_last_punch, tool_capabilities, dir, damage))`
    * Called when a player is punched
    * Note: This callback is invoked even if the punched player is dead.
    * `player`: ObjectRef - Player that was punched
    * `hitter`: ObjectRef - Player that hit. Can be nil.
    * `time_from_last_punch`: Meant for disallowing spamming of clicks
      (can be nil).
    * `tool_capabilities`: Capability table of used item (can be nil)
    * `dir`: Unit vector of direction of punch. Always defined. Points from
      the puncher to the punched.
    * `damage`: Number that represents the damage calculated by the engine
    * should return `true` to prevent the default damage mechanism
* `core.register_on_rightclickplayer(function(player, clicker))`
    * Called when the 'place/use' key was used while pointing a player
      (not necessarily an actual rightclick)
    * `player`: ObjectRef - Player that is acted upon
    * `clicker`: ObjectRef - Object that acted upon `player`, may or may not be a player
* `core.register_on_player_hpchange(function(player, hp_change, reason), modifier)`
    * Called when the player gets damaged or healed
    * When `hp == 0`, damage doesn't trigger this callback.
    * When `hp == hp_max`, healing does still trigger this callback.
    * `player`: ObjectRef of the player
    * `hp_change`: the amount of change. Negative when it is damage.
      * Historically, the new HP value was clamped to [0, 65535] before
        calculating the HP change. This clamping has been removed as of
        version 5.10.0
    * `reason`: a PlayerHPChangeReason table.
        * The `type` field will have one of the following values:
            * `set_hp`: A mod or the engine called `set_hp` without
                        giving a type - use this for custom damage types.
            * `punch`: Was punched. `reason.object` will hold the puncher, or nil if none.
            * `fall`
            * `node_damage`: `damage_per_second` from a neighboring node.
                             `reason.node` will hold the node name or nil.
                             `reason.node_pos` will hold the position of the node
            * `drown`
            * `respawn`
        * Any of the above types may have additional fields from mods.
        * `reason.from` will be `mod` or `engine`.
    * `modifier`: when true, the function should return the actual `hp_change`.
       Note: modifiers only get a temporary `hp_change` that can be modified by later modifiers.
       Modifiers can return true as a second argument to stop the execution of further functions.
       Non-modifiers receive the final HP change calculated by the modifiers.
* `core.register_on_dieplayer(function(ObjectRef, reason))`
    * Called when a player dies
    * `reason`: a PlayerHPChangeReason table, see register_on_player_hpchange
    * For customizing the death screen, see `core.show_death_screen`.
* `core.register_on_respawnplayer(function(ObjectRef))`
    * Called when player is to be respawned
    * Called _before_ repositioning of player occurs
    * return true in func to disable regular player placement
* `core.register_on_prejoinplayer(function(name, ip))`
    * Called when a client connects to the server, prior to authentication
    * If it returns a string, the client is disconnected with that string as
      reason.
* `core.register_on_joinplayer(function(ObjectRef, last_login))`
    * Called when a player joins the game
    * `last_login`: The timestamp of the previous login, or nil if player is new
* `core.register_on_leaveplayer(function(ObjectRef, timed_out))`
    * Called when a player leaves the game
    * Does not get executed for connected players on shutdown.
    * `timed_out`: True for timeout, false for other reasons.
* `core.register_on_authplayer(function(name, ip, is_success))`
    * Called when a client attempts to log into an account.
    * `name`: The name of the account being authenticated.
    * `ip`: The IP address of the client
    * `is_success`: Whether the client was successfully authenticated
    * For newly registered accounts, `is_success` will always be true
* `core.register_on_auth_fail(function(name, ip))`
    * Deprecated: use `core.register_on_authplayer(name, ip, is_success)` instead.
* `core.register_on_cheat(function(ObjectRef, cheat))`
    * Called when a player cheats
    * `cheat`: `{type=<cheat_type>}`, where `<cheat_type>` is one of:
        * `moved_too_fast`
        * `interacted_too_far`
        * `interacted_with_self`
        * `interacted_while_dead`
        * `finished_unknown_dig`
        * `dug_unbreakable`
        * `dug_too_fast`
* `core.register_on_chat_message(function(name, message))`
    * Called always when a player says something
    * Return `true` to mark the message as handled, which means that it will
      not be sent to other players.
* `core.register_on_chatcommand(function(name, command, params))`
    * Called always when a chatcommand is triggered, before `core.registered_chatcommands`
      is checked to see if the command exists, but after the input is parsed.
    * Return `true` to mark the command as handled, which means that the default
      handlers will be prevented.
* `core.register_on_player_receive_fields(function(player, formname, fields))`
    * Called when the server received input from `player`.
      Specifically, this is called on any of the
      following events:
          * a button was pressed,
          * Enter was pressed while the focus was on a text field
          * a checkbox was toggled,
          * something was selected in a dropdown list,
          * a different tab was selected,
          * selection was changed in a textlist or table,
          * an entry was double-clicked in a textlist or table,
          * a scrollbar was moved, or
          * the form was actively closed by the player.
    * `formname` is the name passed to `core.show_formspec`.
      Special case: The empty string refers to the player inventory
      (the formspec set by the `set_inventory_formspec` player method).
    * Fields are sent for formspec elements which define a field. `fields`
      is a table containing each formspecs element value (as string), with
      the `name` parameter as index for each. The value depends on the
      formspec element type:
        * `animated_image`: Returns the index of the current frame.
        * `button` and variants: If pressed, contains the user-facing button
          text as value. If not pressed, is `nil`
        * `field`, `textarea` and variants: Text in the field
        * `dropdown`: Either the index or value, depending on the `index event`
          dropdown argument.
        * `tabheader`: Tab index, starting with `"1"` (only if tab changed)
        * `checkbox`: `"true"` if checked, `"false"` if unchecked
        * `textlist`: See `core.explode_textlist_event`
        * `table`: See `core.explode_table_event`
        * `scrollbar`: See `core.explode_scrollbar_event`
        * Special case: `["quit"]="true"` is sent when the user actively
          closed the form by mouse click, keypress or through a button_exit[]
          element.
        * Special case: `["key_enter"]="true"` is sent when the user pressed
          the Enter key and the focus was either nowhere (causing the formspec
          to be closed) or on a button. If the focus was on a text field,
          additionally, the index `key_enter_field` contains the name of the
          text field. See also: `field_close_on_enter`.
    * Newest functions are called first
    * If function returns `true`, remaining functions are not called
* `core.register_on_craft(function(itemstack, player, old_craft_grid, craft_inv))`
    * Called when `player` crafts something
    * `itemstack` is the output
    * `old_craft_grid` contains the recipe (Note: the one in the inventory is
      cleared).
    * `craft_inv` is the inventory with the crafting grid
    * Return either an `ItemStack`, to replace the output, or `nil`, to not
      modify it.
* `core.register_craft_predict(function(itemstack, player, old_craft_grid, craft_inv))`
    * The same as before, except that it is called before the player crafts, to
      make craft prediction, and it should not change anything.
* `core.register_allow_player_inventory_action(function(player, action, inventory, inventory_info))`
    * Determines how much of a stack may be taken, put or moved to a
      player inventory.
    * Function arguments: see `core.register_on_player_inventory_action`
    * Return a numeric value to limit the amount of items to be taken, put or
      moved. A value of `-1` for `take` will make the source stack infinite.
* `core.register_on_player_inventory_action(function(player, action, inventory, inventory_info))`
    * Called after an item take, put or move event from/to/in a player inventory
    * These inventory actions are recognized:
        * move: Item was moved within the player inventory
        * put: Item was put into player inventory from another inventory
        * take: Item was taken from player inventory and put into another inventory
    * `player` (type `ObjectRef`) is the player who modified the inventory
      `inventory` (type `InvRef`).
    * List of possible `action` (string) values and their
      `inventory_info` (table) contents:
        * `move`: `{from_list=string, to_list=string, from_index=number, to_index=number, count=number}`
        * `put`:  `{listname=string, index=number, stack=ItemStack}`
        * `take`: Same as `put`
    * Does not accept or handle any return value.
* `core.register_on_protection_violation(function(pos, name))`
    * Called by `builtin` and mods when a player violates protection at a
      position (eg, digs a node or punches a protected entity).
    * The registered functions can be called using
      `core.record_protection_violation`.
    * The provided function should check that the position is protected by the
      mod calling this function before it prints a message, if it does, to
      allow for multiple protection mods.
* `core.register_on_item_eat(function(hp_change, replace_with_item, itemstack, user, pointed_thing))`
    * Called when an item is eaten, by `core.item_eat`
    * Return `itemstack` to cancel the default item eat response (i.e.: hp increase).
* `core.register_on_item_pickup(function(itemstack, picker, pointed_thing, time_from_last_punch,  ...))`
    * Called by `core.item_pickup` before an item is picked up.
    * Function is added to `core.registered_on_item_pickups`.
    * Oldest functions are called first.
    * Parameters are the same as in the `on_pickup` callback.
    * Return an itemstack to cancel the default item pick-up response (i.e.: adding
      the item into inventory).
* `core.register_on_priv_grant(function(name, granter, priv))`
    * Called when `granter` grants the priv `priv` to `name`.
    * Note that the callback will be called twice if it's done by a player,
      once with granter being the player name, and again with granter being nil.
* `core.register_on_priv_revoke(function(name, revoker, priv))`
    * Called when `revoker` revokes the priv `priv` from `name`.
    * Note that the callback will be called twice if it's done by a player,
      once with revoker being the player name, and again with revoker being nil.
* `core.register_can_bypass_userlimit(function(name, ip))`
    * Called when `name` user connects with `ip`.
    * Return `true` to by pass the player limit
* `core.register_on_modchannel_message(function(channel_name, sender, message))`
    * Called when an incoming mod channel message is received
    * You should have joined some channels to receive events.
    * If message comes from a server mod, `sender` field is an empty string.
* `core.register_on_liquid_transformed(function(pos_list, node_list))`
    * Called after liquid nodes (`liquidtype ~= "none"`) are modified by the
      engine's liquid transformation process.
    * `pos_list` is an array of all modified positions.
    * `node_list` is an array of the old node that was previously at the position
      with the corresponding index in pos_list.
* `core.register_on_mapblocks_changed(function(modified_blocks, modified_block_count))`
    * Called soon after any nodes or node metadata have been modified. No
      modifications will be missed, but there may be false positives.
    * Will never be called more than once per server step.
    * `modified_blocks` is the set of modified mapblock position hashes. These
      are in the same format as those produced by `core.hash_node_position`,
      and can be converted to positions with `core.get_position_from_hash`.
      The set is a table where the keys are hashes and the values are `true`.
    * `modified_block_count` is the number of entries in the set.
    * Note: callbacks must be registered at mod load time.
