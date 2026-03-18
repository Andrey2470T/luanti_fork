HUD
===

HUD element types
-----------------

The `position` field is used for all element types.
To account for differing resolutions, the position coordinates are the
percentage of the screen, ranging in value from `0` to `1`.

The `name` field is not yet used, but should contain a description of what the
HUD element represents.

The `direction` field is the direction in which something is drawn.
`0` draws from left to right, `1` draws from right to left, `2` draws from
top to bottom, and `3` draws from bottom to top.

The `alignment` field specifies how the item will be aligned. It is a table
where `x` and `y` range from `-1` to `1`, with `0` being central. `-1` is
moved to the left/up, and `1` is to the right/down. Fractional values can be
used.

The `offset` field specifies a pixel offset from the position. Contrary to
position, the offset is not scaled to screen size. This allows for some
precisely positioned items in the HUD.

**Note**: `offset` _will_ adapt to screen DPI as well as user defined scaling
factor!

The `z_index` field specifies the order of HUD elements from back to front.
Lower z-index elements are displayed behind higher z-index elements. Elements
with same z-index are displayed in an arbitrary order. Default 0.
Supports negative values. By convention, the following values are recommended:

*  -400: Graphical effects, such as vignette
*  -300: Name tags, waypoints
*  -200: Wieldhand
*  -100: Things that block the player's view, e.g. masks
*     0: Default. For standard in-game HUD elements like crosshair, hotbar,
         minimap, builtin statbars, etc.
*   100: Temporary text messages or notification icons
*  1000: Full-screen effects such as full-black screen or credits.
         This includes effects that cover the entire screen

If your HUD element doesn't fit into any category, pick a number
between the suggested values

Below are the specific uses for fields in each type; fields not listed for that
type are ignored.

### `image`

Displays an image on the HUD.

* `scale`: The scale of the image, with `{x = 1, y = 1}` being the original texture size.
  The `x` and `y` fields apply to the respective axes.
  Positive values scale the source image.
  Negative values represent percentages relative to screen dimensions.
  Example: `{x = -20, y = 3}` means the image will be drawn 20% of screen width wide,
  and 3 times as high as the source image is.
* `text`: The name of the texture that is displayed.
* `alignment`: The alignment of the image.
* `offset`: offset in pixels from position.

### `text`

Displays text on the HUD.

* `scale`: Defines the bounding rectangle of the text.
  A value such as `{x=100, y=100}` should work.
* `text`: The text to be displayed in the HUD element.
  Supports `core.translate` (always)
  and `core.colorize` (since protocol version 44)
* `number`: An integer containing the RGB value of the color used to draw the
  text. Specify `0xFFFFFF` for white text, `0xFF0000` for red, and so on.
* `alignment`: The alignment of the text.
* `offset`: offset in pixels from position.
* `size`: size of the text.
  The player-set font size is multiplied by size.x (y value isn't used).
* `style`: determines font style
  Bitfield with 1 = bold, 2 = italic, 4 = monospace

### `statbar`

Displays a horizontal bar made up of half-images with an optional background.

* `text`: The name of the texture to use.
* `text2`: Optional texture name to enable a background / "off state"
  texture (useful to visualize the maximal value). Both textures
  must have the same size.
* `number`: The number of half-textures that are displayed.
  If odd, will end with a vertically center-split texture.
* `item`: Same as `number` but for the "off state" texture
* `direction`: To which direction the images will extend to
* `offset`: offset in pixels from position.
* `size`: If used, will force full-image size to this value (override texture
  pack image size)

### `inventory`

* `text`: The name of the inventory list to be displayed.
* `number`: Number of items in the inventory to be displayed.
* `item`: Position of item that is selected.
* `direction`: Direction the list will be displayed in
* `offset`: offset in pixels from position.
* `alignment`: The alignment of the inventory. Aligned at the top left corner if not specified.

### `hotbar`

* `direction`: Direction the list will be displayed in
* `offset`: offset in pixels from position.
* `alignment`: The alignment of the inventory.

### `waypoint`

Displays distance to selected world position.

* `name`: The name of the waypoint.
* `text`: Distance suffix. Can be blank.
* `precision`: Waypoint precision, integer >= 0. Defaults to 10.
  If set to 0, distance is not shown. Shown value is `floor(distance*precision)/precision`.
  When the precision is an integer multiple of 10, there will be `log_10(precision)` digits after the decimal point.
  `precision = 1000`, for example, will show 3 decimal places (eg: `0.999`).
  `precision = 2` will show multiples of `0.5`; precision = 5 will show multiples of `0.2` and so on:
  `precision = n` will show multiples of `1/n`
* `number:` An integer containing the RGB value of the color used to draw the
  text.
* `world_pos`: World position of the waypoint.
* `offset`: offset in pixels from position.
* `alignment`: The alignment of the waypoint.

### `image_waypoint`

Same as `image`, but does not accept a `position`; the position is instead determined by `world_pos`, the world position of the waypoint.

* `scale`: The scale of the image, with `{x = 1, y = 1}` being the original texture size.
  The `x` and `y` fields apply to the respective axes.
  Positive values scale the source image.
  Negative values represent percentages relative to screen dimensions.
  Example: `{x = -20, y = 3}` means the image will be drawn 20% of screen width wide,
  and 3 times as high as the source image is.
* `text`: The name of the texture that is displayed.
* `alignment`: The alignment of the image.
* `world_pos`: World position of the waypoint.
* `offset`: offset in pixels from position.

### `compass`

Displays an image oriented or translated according to current heading direction.

* `size`: The size of this element. Negative values represent percentage
  of the screen; e.g. `x=-100` means 100% (width).
* `scale`: Scale of the translated image (used only for dir = 2 or dir = 3).
* `text`: The name of the texture to use.
* `alignment`: The alignment of the image.
* `offset`: Offset in pixels from position.
* `direction`: How the image is rotated/translated:
  * 0 - Rotate as heading direction
  * 1 - Rotate in reverse direction
  * 2 - Translate as landscape direction
  * 3 - Translate in reverse direction

If translation is chosen, texture is repeated horizontally to fill the whole element.

### `minimap`

Displays a minimap on the HUD.

* `size`: Size of the minimap to display. Minimap should be a square to avoid
  distortion.
  * Negative values represent percentages of the screen. If either `x` or `y`
    is specified as a percentage, the resulting pixel size will be used for
    both `x` and `y`. Example: On a 1920x1080 screen, `{x = 0, y = -25}` will
    result in a 270x270 minimap.
  * Negative values are supported starting with protocol version 45.
* `alignment`: The alignment of the minimap.
* `offset`: offset in pixels from position.
