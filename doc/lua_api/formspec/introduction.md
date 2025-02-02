Formspec
========

Formspec defines a menu. This supports inventories and some of the
typical widgets like buttons, checkboxes, text input fields, etc.
It is a string, with a somewhat strange format.

A formspec is made out of formspec elements, which includes widgets
like buttons but also can be used to set stuff like background color.

Many formspec elements have a `name`, which is a unique identifier which
is used when the server receives user input. You must not use the name
"quit" for formspec elements.

Spaces and newlines can be inserted between the blocks, as is used in the
examples.

Position and size units are inventory slots unless the new coordinate system
is enabled. `X` and `Y` position the formspec element relative to the top left
of the menu or container. `W` and `H` are its width and height values.

If the new system is enabled, all elements have unified coordinates for all
elements with no padding or spacing in between. This is highly recommended
for new forms. See `real_coordinates[<bool>]` and `Migrating to Real
Coordinates`.

Inventories with a `player:<name>` inventory location are only sent to the
player named `<name>`.

When displaying text which can contain formspec code, e.g. text set by a player,
use `core.formspec_escape`.
For colored text you can use `core.colorize`.

Since formspec version 3, elements drawn in the order they are defined. All
background elements are drawn before all other elements.

**WARNING**: do _not_ use an element name starting with `key_`; those names are
reserved to pass key press events to formspec!

**WARNING**: names and values of elements cannot contain binary data such as ASCII
control characters. For values, escape sequences used by the engine are an exception to this.

**WARNING**: Luanti allows you to add elements to every single formspec instance
using `player:set_formspec_prepend()`, which may be the reason backgrounds are
appearing when you don't expect them to, or why things are styled differently
to normal. See [`no_prepend[]`] and [Styling Formspecs].

Examples
--------

### Chest

    size[8,9]
    list[context;main;0,0;8,4;]
    list[current_player;main;0,5;8,4;]

### Furnace

    size[8,9]
    list[context;fuel;2,3;1,1;]
    list[context;src;2,1;1,1;]
    list[context;dst;5,1;2,2;]
    list[current_player;main;0,5;8,4;]

### Minecraft-like player inventory

    size[8,7.5]
    image[1,0.6;1,2;player.png]
    list[current_player;main;0,3.5;8,4;]
    list[current_player;craft;3,0;3,3;]
    list[current_player;craftpreview;7,1;1,1;]

Version History
---------------

* Formspec version 1 (pre-5.1.0):
  * (too much)
* Formspec version 2 (5.1.0):
  * Forced real coordinates
  * background9[]: 9-slice scaling parameters
* Formspec version 3 (5.2.0):
  * Formspec elements are drawn in the order of definition
  * bgcolor[]: use 3 parameters (bgcolor, formspec (now an enum), fbgcolor)
  * box[] and image[] elements enable clipping by default
  * new element: scroll_container[]
* Formspec version 4 (5.4.0):
  * Allow dropdown indexing events
* Formspec version 5 (5.5.0):
  * Added padding[] element
* Formspec version 6 (5.6.0):
  * Add nine-slice images, animated_image, and fgimg_middle
* Formspec version 7 (5.8.0):
  * style[]: Add focused state for buttons
  * Add field_enter_after_edit[] (experimental)
* Formspec version 8 (5.10.0)
  * scroll_container[]: content padding parameter

Migrating to Real Coordinates
-----------------------------

In the old system, positions included padding and spacing. Padding is a gap between
the formspec window edges and content, and spacing is the gaps between items. For
example, two `1x1` elements at `0,0` and `1,1` would have a spacing of `5/4` between them,
and a padding of `3/8` from the formspec edge. It may be easiest to recreate old layouts
in the new coordinate system from scratch.

To recreate an old layout with padding, you'll need to pass the positions and sizes
through the following formula to re-introduce padding:

```
pos = (oldpos + 1)*spacing + padding
where
    padding = 3/8
    spacing = 5/4
```

You'll need to change the `size[]` tag like this:

```
size = (oldsize-1)*spacing + padding*2 + 1
```

A few elements had random offsets in the old system. Here is a table which shows these
offsets when migrating:

| Element |  Position  |  Size   | Notes
|---------|------------|---------|-------
| box     | +0.3, +0.1 | 0, -0.4 |
| button  |            |         | Buttons now support height, so set h = 2 * 15/13 * 0.35, and reposition if h ~= 15/13 * 0.35 before
| list    |            |         | Spacing is now 0.25 for both directions, meaning lists will be taller in height
| label   | 0, +0.3    |         | The first line of text is now positioned centered exactly at the position specified
