Graphics (**experimental**)
---------------------------

Starting from 1.3.0 version it has become possible to access the graphics backend
with allowing cutomizing the materials, shaders and controlling render state.
The CSM API provides the new name space for this called `gfx`.
It is worth to note this part of API is still experimental, may be unstable and highly WIP.

For now one can register new materials with some properties set for various
kinds of stuff (nodes, objects, particles and etc) using `gfx.register_material`.

Shaders are sent over network from `shaders` server mod subfolder
supporting `vsh`, `fsh` and `glsl` shader formats and cached on clients like all builtin ones.


GFX API functions
-----------------

* `gfx.register_material(name, materialdef)`: registers a new material with unique `name` name
    and properties from `materialdef` (see `Material Definition`).
* `gfx.set_lighting(lightdef)`: analogous to the PlayerRef's eponymous method
    (see `server_lua_api/class_reference/object.md`).
    Except the server lighting params, it reads also the following ones:
    * `skycolors` is a table adjusting the sky light colors at the four timeofday points (or intervals):
      * `night` is a color at [0.825;0.18]  (default: `{r=10, g=10, b=31}`).
      * `sunrise` is a color at 0.23        (default: `{r=205, g=90, b=51}`).
      * `day` is a color at [0.28:0.725]    (default: `{r=255, g=251, b=243}`).
      * `sunset` is a color at 0.775        (default: `{r=218, g=77, b=38}`).
      Colors at the night and day stay constant, at the sunrise and sunset they are being interpolated
    * `ambient_color` is a color of ambient light (default: `{r=13, g=13, b=13}`).
      This part of the light is added with the mixed shader light color.
      Allows illuminating badly lit places like caves, nether, underground at the night.

      All color parameters are ColorSpecs, alpha channel is ignored.
* `gfx.get_lighting()`: analogous to the PlayerRef's eponymous method
    (see `server_lua_api/class_reference/object.md`).

The following set/get methods are analogous to the PlayerRef's eponymous methods
(see `server_lua_api/class_reference/object.md`):
* `gfx.set_sky(sky_parameters)`
* `gfx.get_sky(as_table)`
* `gfx.set_sun(sun_parameters)`
* `gfx.get_sun()`
* `gfx.set_moon(moon_parameters)`
* `gfx.get_moon()`
* `gfx.set_stars(star_parameters)`
* `gfx.get_stars()`
* `gfx.set_clouds(cloud_parameters)`
* `gfx.get_clouds()`
* `gfx.override_day_night_ratio(ratio or nil)`
* `gfx.get_day_night_ratio()`

Texture buffer API:
* `gfx.create_texture_buffer(name, texturedefs)`: create TextureBuffer for the pipeline.
    Necessary for setting shaders inputs (samplers) and outputs (render targets) (see `Texture Definition`).
* `gfx.add_buffer_texture(name, index, texturedef)`: create and add to the `name` buffer a new texture at `index` position.
* `gfx.get_texture_params(name, index)`: returns `Texture Definition` for the texture at `index` position in the `name` buffer.
* `gfx.get_texture_count(name)`: returns the textures count in the `name` buffer
* `gfx.override_draw3d_outputs(outputdefs)`: assign textures to the target outputs for the `Draw3D` step (see `Texture Output Definition`).

Material Definition
-------------------

Used by `gfx.register_material`.

```lua
{
    {  -- [0] layer

        -- Defines the OpenGL blending state
        blend = {
            enable = true/false,
            -- Supported blend modes: "alpha", "add", "subtract", "revsubtract", "multiply", "screen"
            mode = <Blend modes>
        },
        -- Defines the OpenGL depth test state
        depth = {
            enable = true/false,
            -- Supported compare functions: "lessequal", "equal", "less", "notequal", "greaterequal",
            "greater", "always", "never"
            func = = <Compare functions>
        },
        -- Defines the OpenGL stencil test state
        stencil = {
            enable = true/false,
            func = = <Compare functions>,
        },
        -- Defines the OpenGL culling state
        cull = {
            enable = true/false,
            -- Cull modes: "back", "front", "front_and_back"
            mode = <Cull mode>
        },
        shader = <Shader subdefinition>,
        samplers = <Samplers subdefinition>
    },
    ... -- [1] layer, [2] layer and etc
}
```

Shader subdefinition
--------------------

Defines properties for shader generation.

```lua
{
    -- Names of shader files
    vertex = "block_vertex.glsl"/"block.vsh",
    geometry = "block_geometry.glsl"/"block.gsh",
    fragment = "block_fragment.glsl"/"block.fsh",

    -- Preprocessor connstants (name, value) which will be declared via #define
    constants = {
        {"ENABLE_NODE_SPECULAR", "1"},
        {"USE_DISCARD", "0"},
        ...
    },

    -- Names of include shader files for vertex and fragment shader
    -- Their code will be appended in the beginning of the registered shaders code
    -- It is possible to define the builtin includes like "common", "noise", "fog" (see "/client/shaders/includes")
    vertex_includes = {"matrices"},
    fragment_includes = {"fog", "blending"},

    -- Type of vertices which vertex buffer contains and to which it will be possible to apply the shader
    -- Supported vertex types:
    --      "vertex3d": position, color, normal, texture coordinates
    --      "vertex2tcoords": derivative of "vertex3d", adds the second tex coords
    --      "vertextangents": derivative of "vertex3d", adds tangent and binormal
    --      "vertex3dext": derivative of "vertex3d", adds vec3
    vertex_type = "vertex3d",

    -- Apply the builtin dynamic shadows adding necessary includes
    apply_shadows = true/false,

    -- Callback which updates uniforms using "setter" object, called each frame
    on_set_uniforms = function(setter)
    {
        -- Set uniforms using the uniform setter
        local pos = core.localplayer:get_pos()
        setter.set("cameraPos", "v3f", {x=pos.x, y+pos.y+1.5, z=pos.z})
        setter.set("animationTimer", "f32", core.get_us_time())
    }
}
```

Samplers subdefinition
----------------------

Creates the additional set of textures from `textures` folder, could be used for PBR or anything else.
All these textures are referred in a shader code as `texture<N>`, where `N` is starting from 1.
So, e.g. `texture2D(texture1, uv)` will extract the pixel at `uv` coordinates from `texture1` texture sampler.

```lua
{
    -- "texname" is texture file name
    <texname>,        -- "texture1"
    <texname>,        -- "texture2"
    <texname>,        -- "texture3"
    <texname>,        -- "texture4"
    <texname>         -- "texture5"
}
```

Texture Definition
------------------

Defines parameters of creating OpenGL texture for the pipeline

```lua
{
    -- Type of texture (default is "2d")
    type = "2d"/"cubemap",
    -- Name of texture
    name = <string>,
    -- Resolution of texture in pixels
    size = {x=<number>, y=<number>},
    -- Or scale factor instead
    scale = {x=<factor>, y=<factor>},
    -- Pixel format of texture data (default is "argb8")
    -- Supported pixel formats: "argb8"/"rgb8"/"d16"/"d32"/"d24s8"
    format = <Pixel format>,
    msaa = <number>
}
```

Texture Output Definition
-------------------------

Defines parameters of attaching them to outputs for the pipeline.
The textures must be already created through `gfx.create_texture_buffer` or `gfx.add_buffer_texture`.

```lua
{
    -- Unique output index inside the Draw3D texture buffer output
    index = <number>,
    -- Mapping index of the "index" to some cubemap face (valid for type="cubemap")
    -- Mapping pairs (number - face dir): [0] - "pos_x"; [1] - "pos_y"; [2] - "pos_z"; [3] - "neg_x"; [4] - "neg_y"; [5] - "neg_z"
    face = <number>
}
```
