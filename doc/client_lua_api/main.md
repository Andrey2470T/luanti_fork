Luanti Lua Client Modding API Reference
==============================================

**WARNING**: if you're looking for the `minetest` namespace (e.g. `minetest.something`),
it's now called `core` due to the renaming of Luanti (formerly Minetest).
`minetest` will keep existing as an alias, so that old code won't break.

* More information at <http://www.luanti.org/>
* Developer Wiki: <https://dev.luanti.org/>

Introduction
------------

Content and functionality can be added to Luanti by using Lua
scripting in run-time loaded mods.

A mod is a self-contained bunch of scripts, textures and other related
things that is loaded by and interfaces with Luanti.

Starting from 1.3.0 client-side mods can be transferred from the server to clients,
this feature is called `SSCSM` (see `client_lua_api/sscsm.md` for detail).

If you see a deficiency in the API, feel free to attempt to add the
functionality in the engine and API. You can send such improvements as
source code patches on GitHub.

Programming in Lua
------------------
If you have any difficulty in understanding this, please read
[Programming in Lua](http://www.lua.org/pil/).

Startup
-------
Mods are loaded during client startup from the mod load paths by running
the `init.lua` scripts in a shared environment.

Client-side mod is located in `$path_user/clientmods/<modname>`. In order
to enable that it is necessary to add `load_mod_<modname> = true` in
`$path_user/clientmods/mods.conf`.

Note: Depending on the remote server's settings, client-side mods might not
be loaded or have limited functionality. See setting `csm_restriction_flags` for reference.

Paths
-----
* `RUN_IN_PLACE=1` (Windows release, local build)
    * `$path_user`: `<build directory>`
    * `$path_share`: `<build directory>`
* `RUN_IN_PLACE=0`: (Linux release)
    * `$path_share`:
        * Linux: `/usr/share/minetest`
        * Windows: `<install directory>/minetest-0.4.x`
    * `$path_user`:
        * Linux: `$HOME/.minetest`
        * Windows: `C:/users/<user>/AppData/minetest` (maybe)

Sections
--------
- [Class reference](class_reference.md)
- ['core' namespace reference](core_namespace_reference.md)
- [Definition tables](definitions.md)
- [Flag Specifier Format](flag_specifier_format.md)
- [Formspec](formspec.md)
- [Math Objects](math_objects.md)
- [Mods](mods.md)
- [Representations Of Simple Things](representations_of_simple_things.md)
- [Sounds](sounds.md)
- [Spatial Vectors](spatial_vectors.md)
- [SSCSM](sscsm.md)
- [Graphics](graphics.md)
