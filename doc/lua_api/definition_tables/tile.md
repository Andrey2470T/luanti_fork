Tile definition
---------------

* `"image.png"`
* `{name="image.png", animation={Tile Animation definition}}`
* `{name="image.png", backface_culling=bool, align_style="node"/"world"/"user", scale=int}`
    * backface culling enabled by default for most nodes
    * align style determines whether the texture will be rotated with the node
      or kept aligned with its surroundings. "user" means that client
      setting will be used, similar to `glasslike_framed_optional`.
      Note: supported by solid nodes and nodeboxes only.
    * scale is used to make texture span several (exactly `scale`) nodes,
      instead of just one, in each direction. Works for world-aligned
      textures only.
      Note that as the effect is applied on per-mapblock basis, `16` should
      be equally divisible by `scale` or you may get wrong results.
* `{name="image.png", color=ColorSpec}`
    * the texture's color will be multiplied with this color.
    * the tile's color overrides the owning node's color in all cases.
* deprecated, yet still supported field names:
    * `image` (name)

Tile animation definition
-------------------------

```lua
{
    type = "vertical_frames",

    aspect_w = 16,
    -- Width of a frame in pixels

    aspect_h = 16,
    -- Height of a frame in pixels

    length = 3.0,
    -- Full loop length
}

{
    type = "sheet_2d",

    frames_w = 5,
    -- Width in number of frames

    frames_h = 3,
    -- Height in number of frames

    frame_length = 0.5,
    -- Length of a single frame
}
```
