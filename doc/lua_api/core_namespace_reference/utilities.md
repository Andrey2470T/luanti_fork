Utilities
=========

* `core.get_current_modname()`: returns the currently loading mod's name,
  when loading a mod.
* `core.get_modpath(modname)`: returns the directory path for a mod,
  e.g. `"/home/user/.minetest/usermods/modname"`.
    * Returns nil if the mod is not enabled or does not exist (not installed).
    * Works regardless of whether the mod has been loaded yet.
    * Useful for loading additional `.lua` modules or static data from a mod,
  or checking if a mod is enabled.
* `core.get_modnames()`: returns a list of enabled mods, sorted alphabetically.
    * Does not include disabled mods, even if they are installed.
* `core.get_game_info()`: returns a table containing information about the
  current game. Note that other meta information (e.g. version/release number)
  can be manually read from `game.conf` in the game's root directory.

  ```lua
  {
      id = string,
      title = string,
      author = string,
      -- The root directory of the game
      path = string,
  }
  ```

* `core.get_worldpath()`: returns e.g. `"/home/user/.minetest/world"`
    * Useful for storing custom data
* `core.get_mod_data_path()`: returns e.g. `"/home/user/.minetest/mod_data/mymod"`
    * Useful for storing custom data *independently of worlds*.
    * Must be called during mod load time.
    * Can read or write to this directory at any time.
    * It's possible that multiple Luanti instances are running at the same
      time, which may lead to corruption if you are not careful.
* `core.is_singleplayer()`
* `core.features`: Table containing API feature flags

  ```lua
  {
      glasslike_framed = true,  -- 0.4.7
      nodebox_as_selectionbox = true,  -- 0.4.7
      get_all_craft_recipes_works = true,  -- 0.4.7
      -- The transparency channel of textures can optionally be used on
      -- nodes (0.4.7)
      use_texture_alpha = true,
      -- Tree and grass ABMs are no longer done from C++ (0.4.8)
      no_legacy_abms = true,
      -- Texture grouping is possible using parentheses (0.4.11)
      texture_names_parens = true,
      -- Unique Area ID for AreaStore:insert_area (0.4.14)
      area_store_custom_ids = true,
      -- add_entity supports passing initial staticdata to on_activate
      -- (0.4.16)
      add_entity_with_staticdata = true,
      -- Chat messages are no longer predicted (0.4.16)
      no_chat_message_prediction = true,
      -- The transparency channel of textures can optionally be used on
      -- objects (ie: players and lua entities) (5.0.0)
      object_use_texture_alpha = true,
      -- Object selectionbox is settable independently from collisionbox
      -- (5.0.0)
      object_independent_selectionbox = true,
      -- Specifies whether binary data can be uploaded or downloaded using
      -- the HTTP API (5.1.0)
      httpfetch_binary_data = true,
      -- Whether formspec_version[<version>] may be used (5.1.0)
      formspec_version_element = true,
      -- Whether AreaStore's IDs are kept on save/load (5.1.0)
      area_store_persistent_ids = true,
      -- Whether core.find_path is functional (5.2.0)
      pathfinder_works = true,
      -- Whether Collision info is available to an objects' on_step (5.3.0)
      object_step_has_moveresult = true,
      -- Whether get_velocity() and add_velocity() can be used on players (5.4.0)
      direct_velocity_on_players = true,
      -- nodedef's use_texture_alpha accepts new string modes (5.4.0)
      use_texture_alpha_string_modes = true,
      -- degrotate param2 rotates in units of 1.5° instead of 2°
      -- thus changing the range of values from 0-179 to 0-240 (5.5.0)
      degrotate_240_steps = true,
      -- ABM supports min_y and max_y fields in definition (5.5.0)
      abm_min_max_y = true,
      -- dynamic_add_media supports passing a table with options (5.5.0)
      dynamic_add_media_table = true,
      -- particlespawners support texpools and animation of properties,
      -- particle textures support smooth fade and scale animations, and
      -- sprite-sheet particle animations can by synced to the lifetime
      -- of individual particles (5.6.0)
      particlespawner_tweenable = true,
      -- allows get_sky to return a table instead of separate values (5.6.0)
      get_sky_as_table = true,
      -- VoxelManip:get_light_data accepts an optional buffer argument (5.7.0)
      get_light_data_buffer = true,
      -- When using a mod storage backend that is not "files" or "dummy",
      -- the amount of data in mod storage is not constrained by
      -- the amount of RAM available. (5.7.0)
      mod_storage_on_disk = true,
      -- "zstd" method for compress/decompress (5.7.0)
      compress_zstd = true,
      -- Sound parameter tables support start_time (5.8.0)
      sound_params_start_time = true,
      -- New fields for set_physics_override: speed_climb, speed_crouch,
      -- liquid_fluidity, liquid_fluidity_smooth, liquid_sink,
      -- acceleration_default, acceleration_air (5.8.0)
      physics_overrides_v2 = true,
      -- In HUD definitions the field `type` is used and `hud_elem_type` is deprecated (5.9.0)
      hud_def_type_field = true,
      -- PseudoRandom and PcgRandom state is restorable
      -- PseudoRandom has get_state method
      -- PcgRandom has get_state and set_state methods (5.9.0)
      random_state_restore = true,
      -- core.after guarantees that coexisting jobs are executed primarily
      -- in order of expiry and secondarily in order of registration (5.9.0)
      after_order_expiry_registration = true,
      -- wallmounted nodes mounted at floor or ceiling may additionally
      -- be rotated by 90° with special param2 values (5.9.0)
      wallmounted_rotate = true,
      -- Availability of the `pointabilities` property in the item definition (5.9.0)
      item_specific_pointabilities = true,
      -- Nodes `pointable` property can be `"blocking"` (5.9.0)
      blocking_pointability_type = true,
      -- dynamic_add_media can be called at startup when leaving callback as `nil` (5.9.0)
      dynamic_add_media_startup = true,
      -- dynamic_add_media supports `filename` and `filedata` parameters (5.9.0)
      dynamic_add_media_filepath = true,
       -- L-system decoration type (5.9.0)
      lsystem_decoration_type = true,
      -- Overridable pointing range using the itemstack meta key `"range"` (5.9.0)
      item_meta_range = true,
      -- Allow passing an optional "actor" ObjectRef to the following functions:
      -- core.place_node, core.dig_node, core.punch_node (5.9.0)
      node_interaction_actor = true,
      -- "new_pos" field in entity moveresult (5.9.0)
      moveresult_new_pos = true,
      -- Allow removing definition fields in `core.override_item` (5.9.0)
      override_item_remove_fields = true,
      -- The predefined hotbar is a Lua HUD element of type `hotbar` (5.10.0)
      hotbar_hud_element = true,
      -- Bulk LBM support (5.10.0)
      bulk_lbms = true,
      -- ABM supports field without_neighbors (5.10.0)
      abm_without_neighbors = true,
      -- biomes have a weight parameter (5.11.0)
      biome_weights = true,
      -- Particles can specify a "clip" blend mode (5.11.0)
      particle_blend_clip = true,
  }
  ```

* `core.has_feature(arg)`: returns `boolean, missing_features`
    * `arg`: string or table in format `{foo=true, bar=true}`
    * `missing_features`: `{foo=true, bar=true}`
* `core.get_player_information(player_name)`: Table containing information
  about a player. Example return value:

  ```lua
  {
      address = "127.0.0.1",     -- IP address of client
      ip_version = 4,            -- IPv4 / IPv6
      connection_uptime = 200,   -- seconds since client connected
      protocol_version = 32,     -- protocol version used by client
      formspec_version = 2,      -- supported formspec version
      lang_code = "fr",          -- Language code used for translation

      -- the following keys can be missing if no stats have been collected yet
      min_rtt = 0.01,            -- minimum round trip time
      max_rtt = 0.2,             -- maximum round trip time
      avg_rtt = 0.02,            -- average round trip time
      min_jitter = 0.01,         -- minimum packet time jitter
      max_jitter = 0.5,          -- maximum packet time jitter
      avg_jitter = 0.03,         -- average packet time jitter
      -- the following information is available in a debug build only!!!
      -- DO NOT USE IN MODS
      --ser_vers = 26,             -- serialization version used by client
      --major = 0,                 -- major version number
      --minor = 4,                 -- minor version number
      --patch = 10,                -- patch version number
      --vers_string = "0.4.9-git", -- full version string
      --state = "Active"           -- current client state
  }
  ```

* `core.get_player_window_information(player_name)`:

  ```lua
  -- Will only be present if the client sent this information (requires v5.7+)
  --
  -- Note that none of these things are constant, they are likely to change during a client
  -- connection as the player resizes the window and moves it between monitors
  --
  -- real_gui_scaling and real_hud_scaling can be used instead of DPI.
  -- OSes don't necessarily give the physical DPI, as they may allow user configuration.
  -- real_*_scaling is just OS DPI / 96 but with another level of user configuration.
  {
      -- Current size of the in-game render target (pixels).
      --
      -- This is usually the window size, but may be smaller in certain situations,
      -- such as side-by-side mode.
      size = {
          x = 1308,
          y = 577,
      },

      -- Estimated maximum formspec size before Luanti will start shrinking the
      -- formspec to fit. For a fullscreen formspec, use this formspec size and
      -- `padding[0,0]`. `bgcolor[;true]` is also recommended.
      max_formspec_size = {
          x = 20,
          y = 11.25
      },

      -- GUI Scaling multiplier
      -- Equal to the setting `gui_scaling` multiplied by `dpi / 96`
      real_gui_scaling = 1,

      -- HUD Scaling multiplier
      -- Equal to the setting `hud_scaling` multiplied by `dpi / 96`
      real_hud_scaling = 1,

      -- Whether the touchscreen controls are enabled.
      -- Usually (but not always) `true` on Android.
      -- Requires at least version 5.9.0 on the client. For older clients, it
      -- is always set to `false`.
      touch_controls = false,
  }
  ```

* `core.mkdir(path)`: returns success.
    * Creates a directory specified by `path`, creating parent directories
      if they don't exist.
* `core.rmdir(path, recursive)`: returns success.
    * Removes a directory specified by `path`.
    * If `recursive` is set to `true`, the directory is recursively removed.
      Otherwise, the directory will only be removed if it is empty.
    * Returns true on success, false on failure.
* `core.cpdir(source, destination)`: returns success.
    * Copies a directory specified by `path` to `destination`
    * Any files in `destination` will be overwritten if they already exist.
    * Returns true on success, false on failure.
* `core.mvdir(source, destination)`: returns success.
    * Moves a directory specified by `path` to `destination`.
    * If the `destination` is a non-empty directory, then the move will fail.
    * Returns true on success, false on failure.
* `core.get_dir_list(path, [is_dir])`: returns list of entry names
    * is_dir is one of:
        * nil: return all entries,
        * true: return only subdirectory names, or
        * false: return only file names.
* `core.safe_file_write(path, content)`: returns boolean indicating success
    * Replaces contents of file at path with new contents in a safe (atomic)
      way. Use this instead of below code when writing e.g. database files:
      `local f = io.open(path, "wb"); f:write(content); f:close()`
* `core.get_version()`: returns a table containing components of the
   engine version.  Components:
    * `project`: Name of the project, eg, "Luanti"
    * `string`: Simple version, eg, "1.2.3-dev"
    * `proto_min`: The minimum supported protocol version
    * `proto_max`: The maximum supported protocol version
    * `hash`: Full git version (only set if available),
      eg, "1.2.3-dev-01234567-dirty".
    * `is_dev`: Boolean value indicating whether it's a development build
  Use this for informational purposes only. The information in the returned
  table does not represent the capabilities of the engine, nor is it
  reliable or verifiable. Compatible forks will have a different name and
  version entirely. To check for the presence of engine features, test
  whether the functions exported by the wanted features exist. For example:
  `if core.check_for_falling then ... end`.
* `core.sha1(data, [raw])`: returns the sha1 hash of data
    * `data`: string of data to hash
    * `raw`: return raw bytes instead of hex digits, default: false
* `core.sha256(data, [raw])`: returns the sha256 hash of data
    * `data`: string of data to hash
    * `raw`: return raw bytes instead of hex digits, default: false
* `core.colorspec_to_colorstring(colorspec)`: Converts a ColorSpec to a
  ColorString. If the ColorSpec is invalid, returns `nil`.
    * `colorspec`: The ColorSpec to convert
* `core.colorspec_to_bytes(colorspec)`: Converts a ColorSpec to a raw
  string of four bytes in an RGBA layout, returned as a string.
  * `colorspec`: The ColorSpec to convert
* `core.colorspec_to_table(colorspec)`: Converts a ColorSpec into RGBA table
  form. If the ColorSpec is invalid, returns `nil`. You can use this to parse
  ColorStrings.
    * `colorspec`: The ColorSpec to convert
* `core.time_to_day_night_ratio(time_of_day)`: Returns a "day-night ratio" value
  (as accepted by `ObjectRef:override_day_night_ratio`) that is equivalent to
  the given "time of day" value (as returned by `core.get_timeofday`).
* `core.encode_png(width, height, data, [compression])`: Encode a PNG
  image and return it in string form.
    * `width`: Width of the image
    * `height`: Height of the image
    * `data`: Image data, one of:
        * array table of ColorSpec, length must be width*height
        * string with raw RGBA pixels, length must be width*height*4
    * `compression`: Optional zlib compression level, number in range 0 to 9.
  The data is one-dimensional, starting in the upper left corner of the image
  and laid out in scanlines going from left to right, then top to bottom.
  You can use `colorspec_to_bytes` to generate raw RGBA values.
  Palettes are not supported at the moment.
  You may use this to procedurally generate textures during server init.
* `core.urlencode(str)`: Encodes reserved URI characters by a
  percent sign followed by two hex digits. See
  [RFC 3986, section 2.3](https://datatracker.ietf.org/doc/html/rfc3986#section-2.3).
