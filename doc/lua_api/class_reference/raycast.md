`Raycast`
---------

A raycast on the map. It works with selection boxes.
Can be used as an iterator in a for loop as:

```lua
local ray = Raycast(...)
for pointed_thing in ray do
    ...
end
```

The map is loaded as the ray advances. If the map is modified after the
`Raycast` is created, the changes may or may not have an effect on the object.

It can be created via `Raycast(pos1, pos2, objects, liquids)` or
`core.raycast(pos1, pos2, objects, liquids)` where:

* `pos1`: start of the ray
* `pos2`: end of the ray
* `objects`: if false, only nodes will be returned. Default is true.
* `liquids`: if false, liquid nodes (`liquidtype ~= "none"`) won't be
             returned. Default is false.

### Limitations

Raycasts don't always work properly for attached objects as the server has no knowledge of models & bones.

**Rotated selectionboxes paired with `automatic_rotate` are not reliable** either since the server
can't reliably know the total rotation of the objects on different clients (which may differ on a per-client basis).
The server calculates the total rotation incurred through `automatic_rotate` as a "best guess"
assuming the object was active & rotating on the client all the time since its creation.
This may be significantly out of sync with what clients see.
Additionally, network latency and delayed property sending may create a mismatch of client- & server rotations.

In singleplayer mode, raycasts on objects with rotated selectionboxes & automatic rotate will usually only be slightly off;
toggling automatic rotation may however cause errors to add up.

In multiplayer mode, the error may be arbitrarily large.

### Methods

* `next()`: returns a `pointed_thing` with exact pointing location
    * Returns the next thing pointed by the ray or nil.
