Texture modifiers
=================

There are various texture modifiers that can be used
to let the client generate textures on-the-fly.
The modifiers are applied directly in sRGB colorspace,
i.e. without gamma-correction.

### Notes

 * `TEXMOD_UPSCALE`: The texture with the lower resolution will be automatically
   upscaled to the higher resolution texture.

### Texture overlaying

Textures can be overlaid by putting a `^` between them.

Warning: If the lower and upper pixels are both semi-transparent, this operation
does *not* do alpha blending, and it is *not* associative. Otherwise it does
alpha blending in srgb color space.

Example:

    default_dirt.png^default_grass_side.png

`default_grass_side.png` is overlaid over `default_dirt.png`.

*See notes: `TEXMOD_UPSCALE`*


### Texture grouping

Textures can be grouped together by enclosing them in `(` and `)`.

Example: `cobble.png^(thing1.png^thing2.png)`

A texture for `thing1.png^thing2.png` is created and the resulting
texture is overlaid on top of `cobble.png`.

### Escaping

Modifiers that accept texture names (e.g. `[combine`) accept escaping to allow
passing complex texture names as arguments. Escaping is done with backslash and
is required for `^`, `:` and `\`.

Example: `cobble.png^[lowpart:50:color.png\^[mask\:trans.png`
Or as a Lua string: `"cobble.png^[lowpart:50:color.png\\^[mask\\:trans.png"`

The lower 50 percent of `color.png^[mask:trans.png` are overlaid
on top of `cobble.png`.

### Advanced texture modifiers

#### Crack

* `[crack:<n>:<p>`
* `[cracko:<n>:<p>`
* `[crack:<t>:<n>:<p>`
* `[cracko:<t>:<n>:<p>`

Parameters:

* `<t>`: tile count (in each direction)
* `<n>`: animation frame count
* `<p>`: current animation frame

Draw a step of the crack animation on the texture.
`crack` draws it normally, while `cracko` lays it over, keeping transparent
pixels intact.

Example:

    default_cobble.png^[crack:10:1

#### `[combine:<w>x<h>:<x1>,<y1>=<file1>:<x2>,<y2>=<file2>:...`

* `<w>`: width
* `<h>`: height
* `<x>`: x position, negative numbers allowed
* `<y>`: y position, negative numbers allowed
* `<file>`: texture to combine

Creates a texture of size `<w>` times `<h>` and blits the listed files to their
specified coordinates.

Example:

    [combine:16x32:0,0=default_cobble.png:0,16=default_wood.png

#### `[resize:<w>x<h>`

Resizes the texture to the given dimensions.

Example:

    default_sandstone.png^[resize:16x16

#### `[opacity:<r>`

Makes the base image transparent according to the given ratio.

`r` must be between 0 (transparent) and 255 (opaque).

Example:

    default_sandstone.png^[opacity:127

#### `[invert:<mode>`

Inverts the given channels of the base image.
Mode may contain the characters "r", "g", "b", "a".
Only the channels that are mentioned in the mode string will be inverted.

Example:

    default_apple.png^[invert:rgb

#### `[brighten`

Brightens the texture.

Example:

    tnt_tnt_side.png^[brighten

#### `[noalpha`

Makes the texture completely opaque.

Example:

    default_leaves.png^[noalpha

#### `[makealpha:<r>,<g>,<b>`

Convert one color to transparency.

Example:

    default_cobble.png^[makealpha:128,128,128

#### `[transform<t>`

* `<t>`: transformation(s) to apply

Rotates and/or flips the image.

`<t>` can be a number (between 0 and 7) or a transform name.
Rotations are counter-clockwise.

    0  I      identity
    1  R90    rotate by 90 degrees
    2  R180   rotate by 180 degrees
    3  R270   rotate by 270 degrees
    4  FX     flip X
    5  FXR90  flip X then rotate by 90 degrees
    6  FY     flip Y
    7  FYR90  flip Y then rotate by 90 degrees

Example:

    default_stone.png^[transformFXR90

#### `[inventorycube{<top>{<left>{<right>`

Escaping does not apply here and `^` is replaced by `&` in texture names
instead.

Create an inventory cube texture using the side textures.

Example:

    [inventorycube{grass.png{dirt.png&grass_side.png{dirt.png&grass_side.png

Creates an inventorycube with `grass.png`, `dirt.png^grass_side.png` and
`dirt.png^grass_side.png` textures

#### `[fill:<w>x<h>:<x>,<y>:<color>`

* `<w>`: width
* `<h>`: height
* `<x>`: x position
* `<y>`: y position
* `<color>`: a `ColorString`.

Creates a texture of the given size and color, optionally with an `<x>,<y>`
position. An alpha value may be specified in the `Colorstring`.

The optional `<x>,<y>` position is only used if the `[fill` is being overlaid
onto another texture with '^'.

When `[fill` is overlaid onto another texture it will not upscale or change
the resolution of the texture, the base texture will determine the output
resolution.

Examples:

    [fill:16x16:#20F02080
    texture.png^[fill:8x8:4,4:red

#### `[lowpart:<percent>:<file>`

Blit the lower `<percent>`% part of `<file>` on the texture.

Example:

    base.png^[lowpart:25:overlay.png

#### `[verticalframe:<t>:<n>`

* `<t>`: animation frame count
* `<n>`: current animation frame

Crops the texture to a frame of a vertical animation.

Example:

    default_torch_animated.png^[verticalframe:16:8

#### `[mask:<file>`

Apply a mask to the base image.

The mask is applied using binary AND.

*See notes: `TEXMOD_UPSCALE`*

#### `[sheet:<w>x<h>:<x>,<y>`

Retrieves a tile at position x, y (in tiles, 0-indexed)
from the base image, which it assumes to be a tilesheet
with dimensions w, h (in tiles).

#### `[colorize:<color>:<ratio>`

Colorize the textures with the given color.
`<color>` is specified as a `ColorString`.
`<ratio>` is an int ranging from 0 to 255 or the word "`alpha`". If
it is an int, then it specifies how far to interpolate between the
colors where 0 is only the texture color and 255 is only `<color>`. If
omitted, the alpha of `<color>` will be used as the ratio.  If it is
the word "`alpha`", then each texture pixel will contain the RGB of
`<color>` and the alpha of `<color>` multiplied by the alpha of the
texture pixel.

#### `[colorizehsl:<hue>:<saturation>:<lightness>`

Colorize the texture to the given hue. The texture will be converted into a
greyscale image as seen through a colored glass, like "Colorize" in GIMP.
Saturation and lightness can optionally be adjusted.

`<hue>` should be from -180 to +180. The hue at 0° on an HSL color wheel is
red, 60° is yellow, 120° is green, and 180° is cyan, while -60° is magenta
and -120° is blue.

`<saturation>` and `<lightness>` are optional adjustments.

`<lightness>` is from -100 to +100, with a default of 0

`<saturation>` is from 0 to 100, with a default of 50

#### `[multiply:<color>`

Multiplies texture colors with the given color.
`<color>` is specified as a `ColorString`.
Result is more like what you'd expect if you put a color on top of another
color, meaning white surfaces get a lot of your new color while black parts
don't change very much.

A Multiply blend can be applied between two textures by using the overlay
modifier with a brightness adjustment:

    textureA.png^[contrast:0:-64^[overlay:textureB.png

#### `[screen:<color>`

Apply a Screen blend with the given color. A Screen blend is the inverse of
a Multiply blend, lightening images instead of darkening them.

`<color>` is specified as a `ColorString`.

A Screen blend can be applied between two textures by using the overlay
modifier with a brightness adjustment:

    textureA.png^[contrast:0:64^[overlay:textureB.png

#### `[hsl:<hue>:<saturation>:<lightness>`

Adjust the hue, saturation, and lightness of the texture. Like
"Hue-Saturation" in GIMP, but with 0 as the mid-point.

`<hue>` should be from -180 to +180

`<saturation>` and `<lightness>` are optional, and both percentages.

`<lightness>` is from -100 to +100.

`<saturation>` goes down to -100 (fully desaturated) but may go above 100,
allowing for even muted colors to become highly saturated.

#### `[contrast:<contrast>:<brightness>`

Adjust the brightness and contrast of the texture. Conceptually like
GIMP's "Brightness-Contrast" feature but allows brightness to be wound
all the way up to white or down to black.

`<contrast>` is a value from -127 to +127.

`<brightness>` is an optional value, from -127 to +127.

If only a boost in contrast is required, an alternative technique is to
hardlight blend the texture with itself, this increases contrast in the same
way as an S-shaped color-curve, which avoids dark colors clipping to black
and light colors clipping to white:

    texture.png^[hardlight:texture.png

#### `[overlay:<file>`

Applies an Overlay blend with the two textures, like the Overlay layer mode
in GIMP. Overlay is the same as Hard light but with the role of the two
textures swapped, see the `[hardlight` modifier description for more detail
about these blend modes.

*See notes: `TEXMOD_UPSCALE`*

#### `[hardlight:<file>`

Applies a Hard light blend with the two textures, like the Hard light layer
mode in GIMP.

Hard light combines Multiply and Screen blend modes. Light parts of the
`<file>` texture will lighten (screen) the base texture, and dark parts of the
`<file>` texture will darken (multiply) the base texture. This can be useful
for applying embossing or chiselled effects to textures. A Hard light with the
same texture acts like applying an S-shaped color-curve, and can be used to
increase contrast without clipping.

Hard light is the same as Overlay but with the roles of the two textures
swapped, i.e. `A.png^[hardlight:B.png` is the same as `B.png^[overlay:A.png`

*See notes: `TEXMOD_UPSCALE`*

#### `[png:<base64>`

Embed a base64 encoded PNG image in the texture string.
You can produce a valid string for this by calling
`core.encode_base64(core.encode_png(tex))`,
where `tex` is pixel data. Refer to the documentation of these
functions for details.
You can use this to send disposable images such as captchas
to individual clients, or render things that would be too
expensive to compose with `[combine:`.

IMPORTANT: Avoid sending large images this way.
This is not a replacement for asset files, do not use it to do anything
that you could instead achieve by just using a file.
In particular consider `core.dynamic_add_media` and test whether
using other texture modifiers could result in a shorter string than
embedding a whole image, this may vary by use case.

*See notes: `TEXMOD_UPSCALE`*
