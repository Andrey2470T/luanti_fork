Games
=====

Games are looked up from:

* `$path_share/games/<gameid>/`
* `$path_user/games/<gameid>/`

Where `<gameid>` is unique to each game.

The game directory can contain the following files:

* `game.conf`, with the following keys:
    * `title`: Required, a human-readable title to address the game, e.g. `title = Minetest Game`.
    * `name`: (Deprecated) same as title.
    * `description`: Short description to be shown in the content tab.
      See [Translating content meta](#translating-content-meta).
    * `first_mod`: Use this to specify the mod that must be loaded before any other mod.
    * `last_mod`: Use this to specify the mod that must be loaded after all other mods
    * `allowed_mapgens = <comma-separated mapgens>`
      e.g. `allowed_mapgens = v5,v6,flat`
      Mapgens not in this list are removed from the list of mapgens for the
      game.
      If not specified, all mapgens are allowed.
    * `disallowed_mapgens = <comma-separated mapgens>`
      e.g. `disallowed_mapgens = v5,v6,flat`
      These mapgens are removed from the list of mapgens for the game.
      When both `allowed_mapgens` and `disallowed_mapgens` are
      specified, `allowed_mapgens` is applied before
      `disallowed_mapgens`.
    * `disallowed_mapgen_settings= <comma-separated mapgen settings>`
      e.g. `disallowed_mapgen_settings = mgv5_spflags`
      These mapgen settings are hidden for this game in the world creation
      dialog and game start menu. Add `seed` to hide the seed input field.
    * `disabled_settings = <comma-separated settings>`
      e.g. `disabled_settings = enable_damage, creative_mode`
      These settings are hidden for this game in the "Start game" tab
      and will be initialized as `false` when the game is started.
      Prepend a setting name with an exclamation mark to initialize it to `true`
      (this does not work for `enable_server`).
      Only these settings are supported:
          `enable_damage`, `creative_mode`, `enable_server`.
    * `map_persistent`: Specifies whether newly created worlds should use
      a persistent map backend. Defaults to `true` (= "sqlite3")
    * `author`: The author's ContentDB username.
    * `release`: Ignore this: Should only ever be set by ContentDB, as it is
                 an internal ID used to track versions.
    * `textdomain`: Textdomain used to translate description. Defaults to game id.
      See [Translating content meta](#translating-content-meta).
* `minetest.conf`:
  Used to set default settings when running this game.
* `settingtypes.txt`:
  In the same format as the one in builtin.
  This settingtypes.txt will be parsed by the menu and the settings will be
  displayed in the "Games" category in the advanced settings tab.
* If the game contains a folder called `textures` the server will load it as a
  texturepack, overriding mod textures.
  Any server texturepack will override mod textures and the game texturepack.

Menu images
-----------

Games can provide custom main menu images. They are put inside a `menu`
directory inside the game directory.

The images are named `$identifier.png`, where `$identifier` is one of
`overlay`, `background`, `footer`, `header`.
If you want to specify multiple images for one identifier, add additional
images named like `$identifier.$n.png`, with an ascending number $n starting
with 1, and a random image will be chosen from the provided ones.

Menu music
-----------

Games can provide custom main menu music. They are put inside a `menu`
directory inside the game directory.

The music files are named `theme.ogg`.
If you want to specify multiple music files for one game, add additional
images named like `theme.$n.ogg`, with an ascending number $n starting
with 1 (max 10), and a random music file will be chosen from the provided ones.
