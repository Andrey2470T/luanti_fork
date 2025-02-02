Luanti Lua Modding API Reference
================================

**WARNING**: if you're looking for the `minetest` namespace (e.g. `minetest.something`),
it's now called `core` due to the renaming of Luanti (formerly Minetest).
`minetest` will keep existing as an alias, so that old code won't break.

* More information at <http://www.minetest.net/>
* Developer Wiki: <http://dev.minetest.net/>
* (Unofficial) Minetest Modding Book by rubenwardy: <https://rubenwardy.com/minetest_modding_book/>
* Modding tools: <https://github.com/minetest/modtools>

Introduction
------------

Content and functionality can be added to Luanti using Lua scripting
in run-time loaded mods.

A mod is a self-contained bunch of scripts, textures and other related
things, which is loaded by and interfaces with Luanti.

Mods are contained and ran solely on the server side. Definitions and media
files are automatically transferred to the client.

If you see a deficiency in the API, feel free to attempt to add the
functionality in the engine and API, and to document it here.

Programming in Lua
------------------

If you have any difficulty in understanding this, please read
[Programming in Lua](http://www.lua.org/pil/).

Startup
-------

Mods are loaded during server startup from the mod load paths by running
the `init.lua` scripts in a shared environment.

Paths
-----

Luanti keeps and looks for files mostly in two paths. `path_share` or `path_user`.

`path_share` contains possibly read-only content for the engine (incl. games and mods).
`path_user` contains mods or games installed by the user but also the users
worlds or settings.

With a local build (`RUN_IN_PLACE=1`) `path_share` and `path_user` both point to
the build directory. For system-wide builds on Linux the share path is usually at
`/usr/share/minetest` while the user path resides in `.minetest` in the home directory.
Paths on other operating systems will differ.

Sections
--------
- [Games](games.md)
- [Mods](mods.md)
- [Aliases](aliases.md)
- Textures
  - [Introduction](textures/introduction.md)
  - [Texture Modifiers](textures/texture_modifiers.md)
  - [Hardware Coloring](textures/hardware_coloring.md)
  - [Soft Texture Overlay](textures/soft_texture_overlay.md)
- [Sounds](sounds.md)
- [Registered Definitions](registered_definitions.md)
- [Nodes](nodes.md)
- [Map Terminology And Coordinates](map_terminology_and_coordinates.md)
- [Hud](hud.md)
- [Representations Of Simple Things](representations_of_simple_things.md)
- [Flag Specifier Format](flag_specifier_format.md)
- [Items](items.md)
- [Groups](groups.md)
- [Tool Capabilities](tool_capabilities.md)
- [Entity Damage Mechanism](entity_damage_mechanism.md)
- [Metadata](metadata.md)
- Formspec
  - [Introduction](formspec/introduction.md)
  - [Elements](formspec/elements.md)
  - [Styling](formspec/styling.md)
  - [Markup Language](formspec/markup_language.md)
- [Inventory](inventory.md)
- [Colors](colors.md)
- [Escape Sequences](escape_sequences.md)
- [Spatial Vectors](spatial_vectors.md)
- [Helper Functions](helper_functions.md)
- [Translations](translations.md)
- [Perlin Noise](perlin_noise.md)
- [Ores](ores.md)
- [Decoration Types](decoration_types.md)
- [Schematics](schematics.md)
- [Lua Voxel Manipulator](lua_voxel_manipulator.md)
- [Mapgen Objects](mapgen_objects.md)
- [Registered Entities](registered_entities.md)
- [L-System Trees](l-system_trees.md)
- [Privileges](privileges.md)
- 'core' namespace reference
  - [Utilities](core_namespace_reference/utilities.md)
  - [Logging](core_namespace_reference/logging.md)
  - [Registration Functions](core_namespace_reference/registration_functions.md)
  - [Settings-related](core_namespace_reference/settings.md)
  - [Authentication](core_namespace_reference/authentication.md)
  - [Chat](core_namespace_reference/chat.md)
  - [Environment Access](core_namespace_reference/environment.md)
  - [Mod Channels](core_namespace_reference/mod_channels.md)
  - [Inventory](core_namespace_reference/inventory.md)
  - [Formspec](core_namespace_reference/formspec.md)
  - [Item Handling](core_namespace_reference/item_handling.md)
  - [Rollback](core_namespace_reference/rollback.md)
  - [Sounds](core_namespace_reference/sounds.md)
  - [Timing](core_namespace_reference/timing.md)
  - [Async environment](core_namespace_reference/async_environment.md)
  - [Mapgen environment](core_namespace_reference/mapgen_environment.md)
  - [Server](core_namespace_reference/server.md)
  - [IPC](core_namespace_reference/ipc.md)
  - [Bans](core_namespace_reference/bans.md)
  - [Particles](core_namespace_reference/particles.md)
  - [Schematics](core_namespace_reference/schematics.md)
  - [HTTP Requests](core_namespace_reference/http_requests.md)
  - [Storage API](core_namespace_reference/storage_api.md)
  - [Misc](core_namespace_reference/misc.md)
  - [Globals](core_namespace_reference/globals.md)
- [Class reference](class_reference.md)
- [Definition tables](definition_tables.md)
