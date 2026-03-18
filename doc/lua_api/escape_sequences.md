Escape sequences
================

Most text can contain escape sequences, that can for example color the text.
There are a few exceptions: tab headers, dropdowns and vertical labels can't.
The following functions provide escape sequences:

* `core.get_color_escape_sequence(color)`:
    * `color` is a ColorString
    * The escape sequence sets the text color to `color`
* `core.colorize(color, message)`:
    * Equivalent to:
      `core.get_color_escape_sequence(color) ..
      message ..
      core.get_color_escape_sequence("#ffffff")`
* `core.get_background_escape_sequence(color)`
    * `color` is a ColorString
    * The escape sequence sets the background of the whole text element to
      `color`. Only defined for item descriptions and tooltips.
* `core.strip_foreground_colors(str)`
    * Removes foreground colors added by `get_color_escape_sequence`.
* `core.strip_background_colors(str)`
    * Removes background colors added by `get_background_escape_sequence`.
* `core.strip_colors(str)`
    * Removes all color escape sequences.
