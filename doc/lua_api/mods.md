Mods
====

Mod load path
-------------

Paths are relative to the directories listed in the [Paths] section above.

* `games/<gameid>/mods/`
* `mods/`
* `worlds/<worldname>/worldmods/`

World-specific games
--------------------

It is possible to include a game in a world; in this case, no mods or
games are loaded or checked from anywhere else.

This is useful for e.g. adventure worlds and happens if the `<worldname>/game/`
directory exists.

Mods should then be placed in `<worldname>/game/mods/`.

Modpacks
--------

Mods can be put in a subdirectory, if the parent directory, which otherwise
should be a mod, contains a file named `modpack.conf`.
The file is a key-value store of modpack details.

* `name`: The modpack name. Allows Luanti to determine the modpack name even
          if the folder is wrongly named.
* `title`: A human-readable title to address the modpack. See [Translating content meta](#translating-content-meta).
* `description`: Description of mod to be shown in the Mods tab of the main
                 menu. See [Translating content meta](#translating-content-meta).
* `author`: The author's ContentDB username.
* `release`: Ignore this: Should only ever be set by ContentDB, as it is an
             internal ID used to track versions.
* `textdomain`: Textdomain used to translate title and description. Defaults to modpack name.
  See [Translating content meta](#translating-content-meta).

Note: to support 0.4.x, please also create an empty modpack.txt file.

Mod directory structure
-----------------------

    mods
    ├── modname
    │   ├── mod.conf
    │   ├── screenshot.png
    │   ├── settingtypes.txt
    │   ├── init.lua
    │   ├── models
    │   ├── textures
    │   │   ├── modname_stuff.png
    │   │   ├── modname_something_else.png
    │   │   ├── subfolder_foo
    │   │   │   ├── modname_more_stuff.png
    │   │   │   └── another_subfolder
    │   │   └── bar_subfolder
    │   ├── sounds
    │   ├── media
    │   ├── locale
    │   └── <custom data>
    └── another

### modname

The location of this directory can be fetched by using
`core.get_modpath(modname)`.

### mod.conf

A `Settings` file that provides meta information about the mod.

* `name`: The mod name. Allows Luanti to determine the mod name even if the
          folder is wrongly named.
* `title`: A human-readable title to address the mod. See [Translating content meta](#translating-content-meta).
* `description`: Description of mod to be shown in the Mods tab of the main
                 menu. See [Translating content meta](#translating-content-meta).
* `depends`: A comma separated list of dependencies. These are mods that must be
             loaded before this mod.
* `optional_depends`: A comma separated list of optional dependencies.
                      Like a dependency, but no error if the mod doesn't exist.
* `author`: The author's ContentDB username.
* `release`: Ignore this: Should only ever be set by ContentDB, as it is an
             internal ID used to track versions.
* `textdomain`: Textdomain used to translate title and description. Defaults to modname.
  See [Translating content meta](#translating-content-meta).

### `screenshot.png`

A screenshot shown in the mod manager within the main menu. It should
have an aspect ratio of 3:2 and a minimum size of 300×200 pixels.

### `depends.txt`

**Deprecated:** you should use mod.conf instead.

This file is used if there are no dependencies in mod.conf.

List of mods that have to be loaded before loading this mod.

A single line contains a single modname.

Optional dependencies can be defined by appending a question mark
to a single modname. This means that if the specified mod
is missing, it does not prevent this mod from being loaded.

### `description.txt`

**Deprecated:** you should use mod.conf instead.

This file is used if there is no description in mod.conf.

A file containing a description to be shown in the Mods tab of the main menu.

### `settingtypes.txt`

The format is documented in `builtin/settingtypes.txt`.
It is parsed by the main menu settings dialogue to list mod-specific
settings in the "Mods" category.

`core.settings` can be used to read custom or engine settings.
See [`Settings`].

### `init.lua`

The main Lua script. Running this script should register everything it
wants to register. Subsequent execution depends on Luanti calling the
registered callbacks.

### `textures`, `sounds`, `media`, `models`, `locale`

Media files (textures, sounds, whatever) that will be transferred to the
client and will be available for use by the mod and translation files for
the clients (see [Translations]). Accepted characters for names are:

    a-zA-Z0-9_.-

Accepted formats are:

    images: .png, .jpg, .tga
    sounds: .ogg vorbis
    models: .x, .b3d, .obj, (since version 5.10:) .gltf, .glb

Other formats won't be sent to the client (e.g. you can store .blend files
in a folder for convenience, without the risk that such files are transferred)

It is suggested to use the folders for the purpose they are thought for,
eg. put textures into `textures`, translation files into `locale`,
models for entities or meshnodes into `models` et cetera.

These folders and subfolders can contain subfolders.
Subfolders with names starting with `_` or `.` are ignored.
If a subfolder contains a media file with the same name as a media file
in one of its parents, the parent's file is used.

Although it is discouraged, a mod can overwrite a media file of any mod that it
depends on by supplying a file with an equal name.

Only a subset of model file format features is supported:

Simple textured meshes (with multiple textures), optionally with normals.
The .x, .b3d and .gltf formats additionally support (a single) animation.

#### glTF

The glTF model file format for now only serves as a
more modern alternative to the other static model file formats;
it unlocks no special rendering features.

Binary glTF (`.glb`) files are supported and recommended over `.gltf` files
due to their space savings.

Bone weights should be normalized, e.g. using ["normalize all" in Blender](https://docs.blender.org/manual/en/4.2/grease_pencil/modes/weight_paint/weights_menu.html#normalize-all).

You can use the [Khronos glTF validator](https://github.com/KhronosGroup/glTF-Validator)
to check whether a model is a valid glTF file.

Many glTF features are not supported *yet*, including:

* Animations
  * Only a single animation is supported, use frame ranges within this animation.
  * `CUBICSPLINE` interpolation is not supported.
* Cameras
* Materials
  * Only base color textures are supported
  * Backface culling is overridden
  * Double-sided materials don't work
* Alternative means of supplying data
  * Embedded images. You can use `gltfutil.py` from the
    [modding tools](https://github.com/minetest/modtools) to strip or extract embedded images.
  * References to files via URIs

Textures are supplied solely via the same means as for the other model file formats:
The `textures` object property, the `tiles` node definition field and
the list of textures used in the `model[]` formspec element.

The order in which textures are to be supplied
is that in which they appear in the `textures` array in the glTF file.

Do not rely on glTF features not being supported; they may be supported in the future.
The backwards compatibility guarantee does not extend to ignoring unsupported features.

For example, if your model used an emissive material,
you should expect that a future version of Luanti may respect this,
and thus cause your model to render differently there.

Naming conventions
------------------

Registered names should generally be in this format:

    modname:<whatever>

`<whatever>` can have these characters:

    a-zA-Z0-9_

This is to prevent conflicting names from corrupting maps and is
enforced by the mod loader.

Registered names can be overridden by prefixing the name with `:`. This can
be used for overriding the registrations of some other mod.

The `:` prefix can also be used for maintaining backwards compatibility.

### Example

In the mod `experimental`, there is the ideal item/node/entity name `tnt`.
So the name should be `experimental:tnt`.

Any mod can redefine `experimental:tnt` by using the name

    :experimental:tnt

when registering it. For this to work correctly, that mod must have
`experimental` as a dependency.
