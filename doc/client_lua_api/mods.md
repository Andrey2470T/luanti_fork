Mods
====

Mod load path
-------------
Generic:

* `$path_share/clientmods/`
* `$path_user/clientmods/` (User-installed mods)

In a run-in-place version (e.g. the distributed windows version):

* `minetest/clientmods/` (User-installed mods)

On an installed version on Linux:

* `/usr/share/minetest/clientmods/`
* `$HOME/.minetest/clientmods/` (User-installed mods)

Modpack support
----------------

Mods can be put in a subdirectory, if the parent directory, which otherwise
should be a mod, contains a file named `modpack.conf`.
The file is a key-value store of modpack details.

* `name`: The modpack name.
* `description`: Description of mod to be shown in the Mods tab of the main
                 menu.

Mod directory structure
------------------------

    clientmods
    ├── modname
    │   ├── mod.conf
    │   ├── init.lua
    └── another

### modname

The location of this directory.

### mod.conf

An (optional) settings file that provides meta information about the mod.

* `name`: The mod name. Allows Luanti to determine the mod name even if the
          folder is wrongly named.
* `description`: Description of mod to be shown in the Mods tab of the main
                 menu.
* `depends`: A comma separated list of dependencies. These are mods that must be
             loaded before this mod.
* `optional_depends`: A comma separated list of optional dependencies.
                      Like a dependency, but no error if the mod doesn't exist.

### `init.lua`

The main Lua script. Running this script should register everything it
wants to register. Subsequent execution depends on Luanti calling the
registered callbacks.

**NOTE**: Client mods currently can't provide textures, sounds, or models by
themselves. Any media referenced in function calls must already be loaded
(provided by mods that exist on the server).
