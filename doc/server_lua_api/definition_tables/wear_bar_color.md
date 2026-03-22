Wear Bar Color
--------------

'Wear Bar' is a property of items that defines the coloring
of the bar that appears under damaged tools.
If it is absent, the default behavior of green-yellow-red is
used.

### Wear bar colors definition

#### Syntax

```lua
{
    -- 'constant' or 'linear'
    -- (nil defaults to 'constant')
    blend = "linear",
    color_stops = {
        [0.0] = "#ff0000",
        [0.5] = "slateblue",
        [1.0] = {r=0, g=255, b=0, a=150},
    }
}
```

#### Blend mode `blend`

* `linear`: blends smoothly between each defined color point.
* `constant`: each color starts at its defined point, and continues up to the next point

#### Color stops `color_stops`

Specified as `ColorSpec` color values assigned to `float` durability keys.

"Durability" is defined as `1 - (wear / 65535)`.

#### Shortcut usage

Wear bar color can also be specified as a single `ColorSpec` instead of a table.
