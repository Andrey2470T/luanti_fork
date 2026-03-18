Textures
========

Mods should generally prefix their textures with `modname_`, e.g. given
the mod name `foomod`, a texture could be called:

    foomod_foothing.png

Textures are referred to by their complete name, or alternatively by
stripping out the file extension:

* e.g. `foomod_foothing.png`
* e.g. `foomod_foothing`

Supported texture formats are PNG (`.png`), JPEG (`.jpg`) and Targa (`.tga`).

Luanti generally uses nearest-neighbor upscaling for textures to preserve the crisp
look of pixel art (low-res textures).
Users can optionally enable bilinear and/or trilinear filtering. However, to avoid
everything becoming blurry, textures smaller than 192px will either not be filtered,
or will be upscaled to that minimum resolution first without filtering.

This is subject to change to move more control to the Lua API, but you can rely on
low-res textures not suddenly becoming filtered.

