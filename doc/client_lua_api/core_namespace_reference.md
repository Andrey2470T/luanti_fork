'core' namespace reference
==========================

### Utilities

* `core.get_current_modname()`: returns the currently loading mod's name, when we are loading a mod
* `core.get_modpath(modname)`: returns virtual path of given mod including
   the trailing separator. This is useful to load additional Lua files
   contained in your mod:
   e.g. `dofile(core.get_modpath(core.get_current_modname()) .. "stuff.lua")`
* `core.get_language()`: returns two strings
   * the current gettext locale
   * the current language code (the same as used for client-side translations)
* `core.get_version()`: returns a table containing components of the
   engine version.  Components:
    * `project`: Name of the project, eg, "Luanti"
    * `string`: Simple version, eg, "1.2.3-dev"
    * `hash`: Full git version (only set if available), eg, "1.2.3-dev-01234567-dirty"
  Use this for informational purposes only. The information in the returned
  table does not represent the capabilities of the engine, nor is it
  reliable or verifiable. Compatible forks will have a different name and
  version entirely. To check for the presence of engine features, test
  whether the functions exported by the wanted features exist. For example:
  `if core.check_for_falling then ... end`.
* `core.sha1(data, [raw])`: returns the sha1 hash of data
    * `data`: string of data to hash
    * `raw`: return raw bytes instead of hex digits, default: false
* `core.colorspec_to_colorstring(colorspec)`: Converts a ColorSpec to a
  ColorString. If the ColorSpec is invalid, returns `nil`.
    * `colorspec`: The ColorSpec to convert
* `core.get_csm_restrictions()`: returns a table of `Flags` indicating the
   restrictions applied to the current mod.
   * If a flag in this table is set to true, the feature is RESTRICTED.
   * Possible flags: `load_client_mods`, `chat_messages`, `read_itemdefs`,
                   `read_nodedefs`, `lookup_nodes`, `read_playerinfo`
* `core.urlencode(str)`: Encodes non-unreserved URI characters by a
  percent sign followed by two hex digits. See
  [RFC 3986, section 2.3](https://datatracker.ietf.org/doc/html/rfc3986#section-2.3).

### Logging
* `core.debug(...)`
    * Equivalent to `core.log(table.concat({...}, "\t"))`
* `core.log([level,] text)`
    * `level` is one of `"none"`, `"error"`, `"warning"`, `"action"`,
      `"info"`, or `"verbose"`.  Default is `"none"`.

### Global callback registration functions
Call these functions only at load time!

* `core.register_globalstep(function(dtime))`
    * Called every client environment step
    * `dtime` is the time since last execution in seconds.
* `core.register_on_mods_loaded(function())`
    * Called just after mods have finished loading.
* `core.register_on_shutdown(function())`
    * Called before client shutdown
    * **Warning**: If the client terminates abnormally (i.e. crashes), the registered
      callbacks **will likely not be run**. Data should be saved at
      semi-frequent intervals as well as on server shutdown.
* `core.register_on_receiving_chat_message(function(message))`
    * Called always when a client receive a message
    * Return `true` to mark the message as handled, which means that it will not be shown to chat
* `core.register_on_sending_chat_message(function(message))`
    * Called always when a client sends a message from chat
    * Return `true` to mark the message as handled, which means that it will not be sent to server
* `core.register_chatcommand(cmd, chatcommand definition)`
    * Adds definition to core.registered_chatcommands
* `core.unregister_chatcommand(name)`
    * Unregisters a chatcommands registered with register_chatcommand.
* `core.register_on_chatcommand(function(command, params))`
    * Called always when a chatcommand is triggered, before `core.registered_chatcommands`
      is checked to see if the command exists, but after the input is parsed.
    * Return `true` to mark the command as handled, which means that the default
      handlers will be prevented.
* `core.register_on_hp_modification(function(hp))`
    * Called when server modified player's HP
* `core.register_on_damage_taken(function(hp))`
    * Called when the local player take damages
* `core.register_on_formspec_input(function(formname, fields))`
    * Called when a button is pressed in the local player's inventory form
    * Newest functions are called first
    * If function returns `true`, remaining functions are not called
* `core.register_on_dignode(function(pos, node))`
    * Called when the local player digs a node
    * Newest functions are called first
    * If any function returns true, the node isn't dug
* `core.register_on_punchnode(function(pos, node))`
    * Called when the local player punches a node
    * Newest functions are called first
    * If any function returns true, the punch is ignored
* `core.register_on_placenode(function(pointed_thing, node))`
    * Called when a node has been placed
* `core.register_on_item_use(function(item, pointed_thing))`
    * Called when the local player uses an item.
    * Newest functions are called first.
    * If any function returns true, the item use is not sent to server.
* `core.register_on_modchannel_message(function(channel_name, sender, message))`
    * Called when an incoming mod channel message is received
    * You must have joined some channels before, and server must acknowledge the
      join request.
    * If message comes from a server mod, `sender` field is an empty string.
* `core.register_on_modchannel_signal(function(channel_name, signal))`
    * Called when a valid incoming mod channel signal is received
    * Signal id permit to react to server mod channel events
    * Possible values are:
      0: join_ok
      1: join_failed
      2: leave_ok
      3: leave_failed
      4: event_on_not_joined_channel
      5: state_changed
* `core.register_on_inventory_open(function(inventory))`
    * Called when the local player open inventory
    * Newest functions are called first
    * If any function returns true, inventory doesn't open
### Sounds
* `core.sound_play(spec, parameters)`: returns a handle
    * `spec` is a `SimpleSoundSpec`
    * `parameters` is a sound parameter table
* `handle:stop()` or `core.sound_stop(handle)`
    * `handle` is a handle returned by `core.sound_play`
* `handle:fade(step, gain)` or `core.sound_fade(handle, step, gain)`
    * `handle` is a handle returned by `core.sound_play`
    * `step` determines how fast a sound will fade.
      Negative step will lower the sound volume, positive step will increase
      the sound volume.
    * `gain` the target gain for the fade.

### Timing
* `core.after(time, func, ...)`
    * Call the function `func` after `time` seconds, may be fractional
    * Optional: Variable number of arguments that are passed to `func`
    * Jobs set for earlier times are executed earlier. If multiple jobs expire
      at exactly the same time, then they expire in the order in which they were
      registered. This basically just applies to jobs registered on the same
      step with the exact same delay.
* `core.get_us_time()`
    * Returns time with microsecond precision. May not return wall time.
* `core.get_timeofday()`
    * Returns the time of day: `0` for midnight, `0.5` for midday

### Map
* `core.get_node_or_nil(pos)`
    * Returns the node at the given position as table in the format
      `{name="node_name", param1=0, param2=0}`, returns `nil`
      for unloaded areas or flavor limited areas.
* `core.get_node_light(pos, timeofday)`
    * Gets the light value at the given position. Note that the light value
      "inside" the node at the given position is returned, so you usually want
      to get the light value of a neighbor.
    * `pos`: The position where to measure the light.
    * `timeofday`: `nil` for current time, `0` for night, `0.5` for day
    * Returns a number between `0` and `15` or `nil`
* `core.find_node_near(pos, radius, nodenames, [search_center])`: returns pos or `nil`
    * `radius`: using a maximum metric
    * `nodenames`: e.g. `{"ignore", "group:tree"}` or `"default:dirt"`
    * `search_center` is an optional boolean (default: `false`)
      If true `pos` is also checked for the nodes
* `core.find_nodes_in_area(pos1, pos2, nodenames, [grouped])`
    * `pos1` and `pos2` are the min and max positions of the area to search.
    * `nodenames`: e.g. `{"ignore", "group:tree"}` or `"default:dirt"`
    * If `grouped` is true the return value is a table indexed by node name
      which contains lists of positions.
    * If `grouped` is false or absent the return values are as follows:
      first value: Table with all node positions
      second value: Table with the count of each node with the node name
      as index
    * Area volume is limited to 4,096,000 nodes
* `core.find_nodes_in_area_under_air(pos1, pos2, nodenames)`: returns a
  list of positions.
    * `nodenames`: e.g. `{"ignore", "group:tree"}` or `"default:dirt"`
    * Return value: Table with all node positions with a node air above
    * Area volume is limited to 4,096,000 nodes
* `core.line_of_sight(pos1, pos2)`: returns `boolean, pos`
    * Checks if there is anything other than air between pos1 and pos2.
    * Returns false if something is blocking the sight.
    * Returns the position of the blocking node when `false`
    * `pos1`: First position
    * `pos2`: Second position
* `core.raycast(pos1, pos2, objects, liquids)`: returns `Raycast`
    * Creates a `Raycast` object.
    * `pos1`: start of the ray
    * `pos2`: end of the ray
    * `objects`: if false, only nodes will be returned. Default is `true`.
    * `liquids`: if false, liquid nodes won't be returned. Default is `false`.

* `core.find_nodes_with_meta(pos1, pos2)`
    * Get a table of positions of nodes that have metadata within a region
      {pos1, pos2}.
* `core.get_meta(pos)`
    * Get a `NodeMetaRef` at that position
* `core.get_node_level(pos)`
    * get level of leveled node (water, snow)
* `core.get_node_max_level(pos)`
    * get max available level for leveled node

### Player
* `core.send_chat_message(message)`
    * Act as if `message` was typed by the player into the terminal.
* `core.run_server_chatcommand(cmd, param)`
    * Alias for `core.send_chat_message("/" .. cmd .. " " .. param)`
* `core.clear_out_chat_queue()`
    * Clears the out chat queue
* `core.localplayer`
    * Reference to the LocalPlayer object. See [`LocalPlayer`](#localplayer) class reference for methods.

### Privileges
* `core.get_privilege_list()`
    * Returns a list of privileges the current player has in the format `{priv1=true,...}`
* `core.string_to_privs(str)`: returns `{priv1=true,...}`
* `core.privs_to_string(privs)`: returns `"priv1,priv2,..."`
    * Convert between two privilege representations

### Client Environment
* `core.get_player_names()`
    * Returns list of player names on server (nil if CSM_RF_READ_PLAYERINFO is enabled by server)
* `core.disconnect()`
    * Disconnect from the server and exit to main menu.
    * Returns `false` if the client is already disconnecting otherwise returns `true`.
* `core.get_server_info()`
    * Returns [server info](#server-info).

### Storage API
* `core.get_mod_storage()`:
    * returns reference to mod private `StorageRef`
    * must be called during mod load time

### Mod channels
![Mod channels communication scheme](docs/mod channels.png)

* `core.mod_channel_join(channel_name)`
    * Client joins channel `channel_name`, and creates it, if necessary. You
      should listen from incoming messages with `core.register_on_modchannel_message`
      call to receive incoming messages. Warning, this function is asynchronous.

### Particles
* `core.add_particle(particle definition)`

* `core.add_particlespawner(particlespawner definition)`
    * Add a `ParticleSpawner`, an object that spawns an amount of particles over `time` seconds
    * Returns an `id`, and -1 if adding didn't succeed

* `core.delete_particlespawner(id)`
    * Delete `ParticleSpawner` with `id` (return value from `core.add_particlespawner`)

### Misc.
* `core.parse_json(string[, nullvalue])`: returns something
    * Convert a string containing JSON data into the Lua equivalent
    * `nullvalue`: returned in place of the JSON null; defaults to `nil`
    * On success returns a table, a string, a number, a boolean or `nullvalue`
    * On failure outputs an error message and returns `nil`
    * Example: `parse_json("[10, {\"a\":false}]")`, returns `{10, {a = false}}`
* `core.write_json(data[, styled])`: returns a string or `nil` and an error message
    * Convert a Lua table into a JSON string
    * styled: Outputs in a human-readable format if this is set, defaults to false
    * Unserializable things like functions and userdata are saved as null.
    * **Warning**: JSON is more strict than the Lua table format.
        1. You can only use strings and positive integers of at least one as keys.
        2. You cannot mix string and integer keys.
           This is due to the fact that JSON has two distinct array and object values.
    * Example: `write_json({10, {a = false}})`, returns `"[10, {\"a\": false}]"`
* `core.serialize(table)`: returns a string
    * Convert a table containing tables, strings, numbers, booleans and `nil`s
      into string form readable by `core.deserialize`
    * Example: `serialize({foo='bar'})`, returns `'return { ["foo"] = "bar" }'`
* `core.deserialize(string)`: returns a table
    * Convert a string returned by `core.deserialize` into a table
    * `string` is loaded in an empty sandbox environment.
    * Will load functions, but they cannot access the global environment.
    * Example: `deserialize('return { ["foo"] = "bar" }')`, returns `{foo='bar'}`
    * Example: `deserialize('print("foo")')`, returns `nil` (function call fails)
        * `error:[string "print("foo")"]:1: attempt to call global 'print' (a nil value)`
* `core.compress(data, method, ...)`: returns `compressed_data`
    * Compress a string of data.
    * `method` is a string identifying the compression method to be used.
    * Supported compression methods:
        * Deflate (zlib): `"deflate"`
        * Zstandard: `"zstd"`
    * `...` indicates method-specific arguments. Currently defined arguments
      are:
        * Deflate: `level` - Compression level, `0`-`9` or `nil`.
        * Zstandard: `level` - Compression level. Integer or `nil`. Default `3`.
        Note any supported Zstandard compression level could be used here,
        but these are subject to change between Zstandard versions.
* `core.decompress(compressed_data, method, ...)`: returns data
    * Decompress a string of data using the algorithm specified by `method`.
    * See documentation on `core.compress()` for supported compression
      methods.
    * `...` indicates method-specific arguments. Currently, no methods use this
* `core.rgba(red, green, blue[, alpha])`: returns a string
    * Each argument is an 8 Bit unsigned integer
    * Returns the ColorString from rgb or rgba values
    * Example: `core.rgba(10, 20, 30, 40)`, returns `"#0A141E28"`
* `core.encode_base64(string)`: returns string encoded in base64
    * Encodes a string in base64.
* `core.decode_base64(string)`: returns string or nil on failure
    * Padding characters are only supported starting at version 5.4.0, where
      5.5.0 and newer perform proper checks.
    * Decodes a string encoded in base64.
* `core.gettext(string)` : returns string
    * look up the translation of a string in the gettext message catalog
* `fgettext_ne(string, ...)`
    * call core.gettext(string), replace "$1"..."$9" with the given
      extra arguments and return the result
* `fgettext(string, ...)` : returns string
    * same as fgettext_ne(), but calls core.formspec_escape before returning result
* `core.pointed_thing_to_face_pos(placer, pointed_thing)`: returns a position
    * returns the exact position on the surface of a pointed node
* `core.global_exists(name)`
    * Checks if a global variable has been set, without triggering a warning.

### UI
* `core.ui.minimap`
    * Reference to the minimap object. See [`Minimap`](#minimap) class reference for methods.
    * If client disabled minimap (using enable_minimap setting) this reference will be nil.
* `core.camera`
    * Reference to the camera object. See [`Camera`](#camera) class reference for methods.
* `core.show_formspec(formname, formspec)` : returns true on success
    * Shows a formspec to the player
* `core.display_chat_message(message)` returns true on success
    * Shows a chat message to the current player.

Setting-related
---------------

* `core.settings`: Settings object containing all of the settings from the
  main config file (`minetest.conf`). Check `server_lua_api/core_namespace_reference/settings.md` for class reference.
* `core.setting_get_pos(name)`: Loads a setting from the main settings and
  parses it as a position (in the format `(1,2,3)`). Returns a position or nil.
