Styling Formspecs
=================

Formspec elements can be themed using the style elements:

    style[<name 1>,<name 2>,...;<prop1>;<prop2>;...]
    style[<name 1>:<state>,<name 2>:<state>,...;<prop1>;<prop2>;...]
    style_type[<type 1>,<type 2>,...;<prop1>;<prop2>;...]
    style_type[<type 1>:<state>,<type 2>:<state>,...;<prop1>;<prop2>;...]

Where a prop is:

    property_name=property_value

For example:

    style_type[button;bgcolor=#006699]
    style[world_delete;bgcolor=red;textcolor=yellow]
    button[4,3.95;2.6,1;world_delete;Delete]

A name/type can optionally be a comma separated list of names/types, like so:

    world_delete,world_create,world_configure
    button,image_button

A `*` type can be used to select every element in the formspec.

Any name/type in the list can also be accompanied by a `+`-separated list of states, like so:

    world_delete:hovered+pressed
    button:pressed

States allow you to apply styles in response to changes in the element, instead of applying at all times.

Setting a property to nothing will reset it to the default value. For example:

    style_type[button;bgimg=button.png;bgimg_pressed=button_pressed.png;border=false]
    style[btn_exit;bgimg=;bgimg_pressed=;border=;bgcolor=red]


### Supported Element Types

Some types may inherit styles from parent types.

* animated_image, inherits from image
* box
* button
* button_exit, inherits from button
* checkbox
* dropdown
* field
* image
* image_button
* item_image_button
* label
* list
* model
* pwdfield, inherits from field
* scrollbar
* tabheader
* table
* textarea
* textlist
* vertlabel, inherits from label


### Valid Properties

* animated_image
    * noclip - boolean, set to true to allow the element to exceed formspec bounds.
* box
    * noclip - boolean, set to true to allow the element to exceed formspec bounds.
        * Defaults to false in formspec_version version 3 or higher
    * **Note**: `colors`, `bordercolors`, and `borderwidths` accept multiple input types:
        * Single value (e.g. `#FF0`): All corners/borders.
        * Two values (e.g. `red,#FFAAFF`): top-left and bottom-right,top-right and bottom-left/
          top and bottom,left and right.
        * Four values (e.g. `blue,#A0F,green,#FFFA`): top-left/top and rotates clockwise.
        * These work similarly to CSS borders.
    * colors - `ColorString`. Sets the color(s) of the box corners. Default `black`.
    * bordercolors - `ColorString`. Sets the color(s) of the borders. Default `black`.
    * borderwidths - Integer. Sets the width(s) of the borders in pixels. If the width is
      negative, the border will extend inside the box, whereas positive extends outside
      the box. A width of zero results in no border; this is default.
* button, button_exit, image_button, item_image_button
    * alpha - boolean, whether to draw alpha in bgimg. Default true.
    * bgcolor - color, sets button tint.
    * bgcolor_hovered - color when hovered. Defaults to a lighter bgcolor when not provided.
        * This is deprecated, use states instead.
    * bgcolor_pressed - color when pressed. Defaults to a darker bgcolor when not provided.
        * This is deprecated, use states instead.
    * bgimg - standard background image. Defaults to none.
    * bgimg_hovered - background image when hovered. Defaults to bgimg when not provided.
        * This is deprecated, use states instead.
    * bgimg_middle - Makes the bgimg textures render in 9-sliced mode and defines the middle rect.
                     See background9[] documentation for more details. This property also pads the
                     button's content when set.
    * bgimg_pressed - background image when pressed. Defaults to bgimg when not provided.
        * This is deprecated, use states instead.
    * font - Sets font type. This is a comma separated list of options. Valid options:
      * Main font type options. These cannot be combined with each other:
        * `normal`: Default font
        * `mono`: Monospaced font
      * Font modification options. If used without a main font type, `normal` is used:
        * `bold`: Makes font bold.
        * `italic`: Makes font italic.
      Default `normal`.
    * font_size - Sets font size. Default is user-set. Can have multiple values:
      * `<number>`: Sets absolute font size to `number`.
      * `+<number>`/`-<number>`: Offsets default font size by `number` points.
      * `*<number>`: Multiplies default font size by `number`, similar to CSS `em`.
    * border - boolean, draw border. Set to false to hide the bevelled button pane. Default true.
    * content_offset - 2d vector, shifts the position of the button's content without resizing it.
    * noclip - boolean, set to true to allow the element to exceed formspec bounds.
    * padding - rect, adds space between the edges of the button and the content. This value is
                relative to bgimg_middle.
    * sound - a sound to be played when triggered.
    * textcolor - color, default white.
* checkbox
    * noclip - boolean, set to true to allow the element to exceed formspec bounds.
    * sound - a sound to be played when triggered.
* dropdown
    * noclip - boolean, set to true to allow the element to exceed formspec bounds.
    * sound - a sound to be played when the entry is changed.
* field, pwdfield, textarea
    * border - set to false to hide the textbox background and border. Default true.
    * font - Sets font type. See button `font` property for more information.
    * font_size - Sets font size. See button `font_size` property for more information.
    * noclip - boolean, set to true to allow the element to exceed formspec bounds.
    * textcolor - color. Default white.
* model
    * bgcolor - color, sets background color.
    * noclip - boolean, set to true to allow the element to exceed formspec bounds.
        * Default to false in formspec_version version 3 or higher
* image
    * noclip - boolean, set to true to allow the element to exceed formspec bounds.
        * Default to false in formspec_version version 3 or higher
* item_image
    * noclip - boolean, set to true to allow the element to exceed formspec bounds. Default to false.
* label, vertlabel
    * font - Sets font type. See button `font` property for more information.
    * font_size - Sets font size. See button `font_size` property for more information.
    * noclip - boolean, set to true to allow the element to exceed formspec bounds.
* list
    * noclip - boolean, set to true to allow the element to exceed formspec bounds.
    * size - 2d vector, sets the size of inventory slots in coordinates.
    * spacing - 2d vector, sets the space between inventory slots in coordinates.
* image_button (additional properties)
    * fgimg - standard image. Defaults to none.
    * fgimg_hovered - image when hovered. Defaults to fgimg when not provided.
        * This is deprecated, use states instead.
    * fgimg_pressed - image when pressed. Defaults to fgimg when not provided.
        * This is deprecated, use states instead.
    * fgimg_middle - Makes the fgimg textures render in 9-sliced mode and defines the middle rect.
                     See background9[] documentation for more details.
    * NOTE: The parameters of any given image_button will take precedence over fgimg/fgimg_pressed
    * sound - a sound to be played when triggered.
* scrollbar
    * noclip - boolean, set to true to allow the element to exceed formspec bounds.
* tabheader
    * noclip - boolean, set to true to allow the element to exceed formspec bounds.
    * sound - a sound to be played when a different tab is selected.
    * textcolor - color. Default white.
* table, textlist
    * font - Sets font type. See button `font` property for more information.
    * font_size - Sets font size. See button `font_size` property for more information.
    * noclip - boolean, set to true to allow the element to exceed formspec bounds.

### Valid States

* *all elements*
    * default - Equivalent to providing no states
* button, button_exit, image_button, item_image_button
    * focused - Active when button has focus
    * hovered - Active when the mouse is hovering over the element
    * pressed - Active when the button is pressed
