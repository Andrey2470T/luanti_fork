Flag Specifier Format
=====================

Flags using the standardized flag specifier format can be specified in either
of two ways, by string or table.

The string format is a comma-delimited set of flag names; whitespace and
unrecognized flag fields are ignored. Specifying a flag in the string sets the
flag, and specifying a flag prefixed by the string `"no"` explicitly
clears the flag from whatever the default may be.

In addition to the standard string flag format, the schematic flags field can
also be a table of flag names to boolean values representing whether or not the
flag is set. Additionally, if a field with the flag name prefixed with `"no"`
is present, mapped to a boolean of any value, the specified flag is unset.

E.g. A flag field of value

```lua
{place_center_x = true, place_center_y=false, place_center_z=true}
```

is equivalent to

```lua
{place_center_x = true, noplace_center_y=true, place_center_z=true}
```

which is equivalent to

```lua
"place_center_x, noplace_center_y, place_center_z"
```

or even

```lua
"place_center_x, place_center_z"
```

since, by default, no schematic attributes are set.
