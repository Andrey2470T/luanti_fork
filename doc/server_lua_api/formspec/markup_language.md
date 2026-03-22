Markup Language
===============

Markup language used in `hypertext[]` elements uses tags that look like HTML tags.
The markup language is currently unstable and subject to change. Use with caution.
Some tags can enclose text, they open with `<tagname>` and close with `</tagname>`.
Tags can have attributes, in that case, attributes are in the opening tag in
form of a key/value separated with equal signs.
Attribute values should be quoted using either " or '.

If you want to insert a literal greater-than, less-than, or a backslash into the text,
you must escape it by preceding it with a backslash. In a quoted attribute value, you
can insert a literal quote mark by preceding it with a backslash.

These are the technically basic tags but see below for usual tags. Base tags are:

`<style color=... font=... size=...>...</style>`

Changes the style of the text.

* `color`: Text color. Given color is a `colorspec`.
* `size`: Text size.
* `font`: Text font (`mono` or `normal`).

`<global background=... margin=... valign=... color=... hovercolor=... size=... font=... halign=... >`

Sets global style.

Global only styles:

* `background`: Text background, a `colorspec` or `none`.
* `margin`: Page margins in pixel.
* `valign`: Text vertical alignment (`top`, `middle`, `bottom`).

Inheriting styles (affects child elements):

* `color`: Default text color. Given color is a `colorspec`.
* `hovercolor`: Color of <action> tags when mouse is over.
* `size`: Default text size.
* `font`: Default text font (`mono` or `normal`).
* `halign`: Default text horizontal alignment (`left`, `right`, `center`, `justify`).

This tag needs to be placed only once as it changes the global settings of the
text. Anyway, if several tags are placed, each changed will be made in the order
tags appear.

`<tag name=... color=... hovercolor=... font=... size=...>`

Defines or redefines tag style. This can be used to define new tags.

* `name`: Name of the tag to define or change.
* `color`: Text color. Given color is a `colorspec`.
* `hovercolor`: Text color when element hovered (only for `action` tags). Given color is a `colorspec`.
* `size`: Text size.
* `font`: Text font (`mono` or `normal`).

Following tags are the usual tags for text layout. They are defined by default.
Other tags can be added using `<tag ...>` tag.

`<normal>...</normal>`: Normal size text

`<big>...</big>`: Big text

`<bigger>...</bigger>`: Bigger text

`<center>...</center>`: Centered text

`<left>...</left>`: Left-aligned text

`<right>...</right>`: Right-aligned text

`<justify>...</justify>`: Justified text

`<mono>...</mono>`: Monospaced font

`<b>...</b>`, `<i>...</i>`, `<u>...</u>`: Bold, italic, underline styles.

`<action name=...>...</action>`

Make that text a clickable text triggering an action.

* `name`: Name of the action (mandatory).
* `url`: URL to open when the action is triggered (optional).

When clicked, the formspec is send to the server. The value of the text field
sent to `on_player_receive_fields` will be "action:" concatenated to the action
name.

`<img name=... float=... width=... height=...>`

Draws an image which is present in the client media cache.

* `name`: Name of the texture (mandatory).
* `float`: If present, makes the image floating (`left` or `right`).
* `width`: Force image width instead of taking texture width.
* `height`: Force image height instead of taking texture height.

If only width or height given, texture aspect is kept.

`<item name=... float=... width=... height=... rotate=...>`

Draws an item image.

* `name`: Item string of the item to draw (mandatory).
* `float`: If present, makes the image floating (`left` or `right`).
* `width`: Item image width.
* `height`: Item image height.
* `rotate`: Rotate item image if set to `yes` or `X,Y,Z`. X, Y and Z being
rotation speeds in percent of standard speed (-1000 to 1000). Works only if
`inventory_items_animations` is set to true.
* `angle`: Angle in which the item image is shown. Value has `X,Y,Z` form.
X, Y and Z being angles around each three axes. Works only if
`inventory_items_animations` is set to true.
