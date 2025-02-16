Particle definition
-------------------

Used by `core.add_particle`.

```lua
{
    pos = {x=0, y=0, z=0},
    velocity = {x=0, y=0, z=0},
    acceleration = {x=0, y=0, z=0},
    -- Spawn particle at pos with velocity and acceleration

    expirationtime = 1,
    -- Disappears after expirationtime seconds

    size = 1,
    -- Scales the visual size of the particle texture.
    -- If `node` is set, size can be set to 0 to spawn a randomly-sized
    -- particle (just like actual node dig particles).

    collisiondetection = false,
    -- If true collides with `walkable` nodes and, depending on the
    -- `object_collision` field, objects too.

    collision_removal = false,
    -- If true particle is removed when it collides.
    -- Requires collisiondetection = true to have any effect.

    object_collision = false,
    -- If true particle collides with objects that are defined as
    -- `physical = true,` and `collide_with_objects = true,`.
    -- Requires collisiondetection = true to have any effect.

    vertical = false,
    -- If true faces player using y axis only

    texture = "image.png",
    -- The texture of the particle
    -- v5.6.0 and later: also supports the table format described in the
    -- following section, but due to a bug this did not take effect
    -- (beyond the texture name).
    -- v5.9.0 and later: fixes the bug.
    -- Note: "texture.animation" is ignored here. Use "animation" below instead.

    playername = "singleplayer",
    -- Optional, if specified spawns particle only on the player's client

    animation = {Tile Animation definition},
    -- Optional, specifies how to animate the particle texture

    glow = 0
    -- Optional, specify particle self-luminescence in darkness.
    -- Values 0-14.

    node = {name = "ignore", param2 = 0},
    -- Optional, if specified the particle will have the same appearance as
    -- node dig particles for the given node.
    -- `texture` and `animation` will be ignored if this is set.

    node_tile = 0,
    -- Optional, only valid in combination with `node`
    -- If set to a valid number 1-6, specifies the tile from which the
    -- particle texture is picked.
    -- Otherwise, the default behavior is used. (currently: any random tile)

    drag = {x=0, y=0, z=0},
    -- v5.6.0 and later: Optional drag value, consult the following section
    -- Note: Only a vector is supported here. Alternative forms like a single
    -- number are not supported.

    jitter = {min = ..., max = ..., bias = 0},
    -- v5.6.0 and later: Optional jitter range, consult the following section

    bounce = {min = ..., max = ..., bias = 0},
    -- v5.6.0 and later: Optional bounce range, consult the following section
}
```


`ParticleSpawner` definition
----------------------------

Used by `core.add_particlespawner`.

Before v5.6.0, particlespawners used a different syntax and had a more limited set
of features. Definition fields that are the same in both legacy and modern versions
are shown in the next listing, and the fields that are used by legacy versions are
shown separated by a comment; the modern fields are too complex to compactly
describe in this manner and are documented after the listing.

The older syntax can be used in combination with the newer syntax (e.g. having
`minpos`, `maxpos`, and `pos` all set) to support older servers. On newer servers,
the new syntax will override the older syntax; on older servers, the newer syntax
will be ignored.

```lua
{
    -------------------
    -- Common fields --
    -------------------
    -- (same name and meaning in both new and legacy syntax)

    amount = 1,
    -- Number of particles spawned over the time period `time`.

    time = 1,
    -- Lifespan of spawner in seconds.
    -- If time is 0 spawner has infinite lifespan and spawns the `amount` on
    -- a per-second basis.

    collisiondetection = false,
    -- If true collide with `walkable` nodes and, depending on the
    -- `object_collision` field, objects too.

    collision_removal = false,
    -- If true particles are removed when they collide.
    -- Requires collisiondetection = true to have any effect.

    object_collision = false,
    -- If true particles collide with objects that are defined as
    -- `physical = true,` and `collide_with_objects = true,`.
    -- Requires collisiondetection = true to have any effect.

    attached = ObjectRef,
    -- If defined, particle positions, velocities and accelerations are
    -- relative to this object's position and yaw

    vertical = false,
    -- If true face player using y axis only

    texture = "image.png",
    -- The texture of the particle
    -- v5.6.0 and later: also supports the table format described in the
    -- following section.

    playername = "singleplayer",
    -- Optional, if specified spawns particles only on the player's client

    animation = {Tile Animation definition},
    -- Optional, specifies how to animate the particles' texture
    -- v5.6.0 and later: set length to -1 to synchronize the length
    -- of the animation with the expiration time of individual particles.
    -- (-2 causes the animation to be played twice, and so on)

    glow = 0,
    -- Optional, specify particle self-luminescence in darkness.
    -- Values 0-14.

    node = {name = "ignore", param2 = 0},
    -- Optional, if specified the particles will have the same appearance as
    -- node dig particles for the given node.
    -- `texture` and `animation` will be ignored if this is set.

    node_tile = 0,
    -- Optional, only valid in combination with `node`
    -- If set to a valid number 1-6, specifies the tile from which the
    -- particle texture is picked.
    -- Otherwise, the default behavior is used. (currently: any random tile)

    -------------------
    -- Legacy fields --
    -------------------

    minpos = {x=0, y=0, z=0},
    maxpos = {x=0, y=0, z=0},
    minvel = {x=0, y=0, z=0},
    maxvel = {x=0, y=0, z=0},
    minacc = {x=0, y=0, z=0},
    maxacc = {x=0, y=0, z=0},
    minexptime = 1,
    maxexptime = 1,
    minsize = 1,
    maxsize = 1,
    -- The particles' properties are random values between the min and max
    -- values.
    -- applies to: pos, velocity, acceleration, expirationtime, size
    -- If `node` is set, min and maxsize can be set to 0 to spawn
    -- randomly-sized particles (just like actual node dig particles).
}
```

### Modern definition fields

After v5.6.0, spawner properties can be defined in several different ways depending
on the level of control you need. `pos` for instance can be set as a single vector,
in which case all particles will appear at that exact point throughout the lifetime
of the spawner. Alternately, it can be specified as a min-max pair, specifying a
cubic range the particles can appear randomly within. Finally, some properties can
be animated by suffixing their key with `_tween` (e.g. `pos_tween`) and supplying
a tween table.

The following definitions are all equivalent, listed in order of precedence from
lowest (the legacy syntax) to highest (tween tables). If multiple forms of a
property definition are present, the highest-precedence form will be selected
and all lower-precedence fields will be ignored, allowing for graceful
degradation in older clients).

```lua
{
  -- old syntax
  maxpos = {x = 0, y = 0, z = 0},
  minpos = {x = 0, y = 0, z = 0},

  -- absolute value
  pos = 0,
  -- all components of every particle's position vector will be set to this
  -- value

  -- vec3
  pos = vector.new(0,0,0),
  -- all particles will appear at this exact position throughout the lifetime
  -- of the particlespawner

  -- vec3 range
  pos = {
        -- the particle will appear at a position that is picked at random from
        -- within a cubic range

        min = vector.new(0,0,0),
        -- `min` is the minimum value this property will be set to in particles
        -- spawned by the generator

        max = vector.new(0,0,0),
        -- `max` is the minimum value this property will be set to in particles
        -- spawned by the generator

        bias = 0,
        -- when `bias` is 0, all random values are exactly as likely as any
        -- other. when it is positive, the higher it is, the more likely values
        -- will appear towards the minimum end of the allowed spectrum. when
        -- it is negative, the lower it is, the more likely values will appear
        -- towards the maximum end of the allowed spectrum. the curve is
        -- exponential and there is no particular maximum or minimum value
    },

    -- tween table
    pos_tween = {...},
    -- a tween table should consist of a list of frames in the same form as the
    -- untweened pos property above, which the engine will interpolate between,
    -- and optionally a number of properties that control how the interpolation
    -- takes place. currently **only two frames**, the first and the last, are
    -- used, but extra frames are accepted for the sake of forward compatibility.
    -- any of the above definition styles can be used here as well in any combination
    -- supported by the property type

    pos_tween = {
        style = "fwd",
        -- linear animation from first to last frame (default)
        style = "rev",
        -- linear animation from last to first frame
        style = "pulse",
        -- linear animation from first to last then back to first again
        style = "flicker",
        -- like "pulse", but slightly randomized to add a bit of stutter

        reps = 1,
        -- number of times the animation is played over the particle's lifespan

        start = 0.0,
        -- point in the spawner's lifespan at which the animation begins. 0 is
        -- the very beginning, 1 is the very end

        -- frames can be defined in a number of different ways, depending on the
        -- underlying type of the property. for now, all but the first and last
        -- frame are ignored

        -- frames

            -- floats
            0, 0,

            -- vec3s
            vector.new(0,0,0),
            vector.new(0,0,0),

            -- vec3 ranges
            { min = vector.new(0,0,0), max = vector.new(0,0,0), bias = 0 },
            { min = vector.new(0,0,0), max = vector.new(0,0,0), bias = 0 },

            -- mixed
            0, { min = vector.new(0,0,0), max = vector.new(0,0,0), bias = 0 },
    },
}
```

All of the properties that can be defined in this way are listed in the next
section, along with the datatypes they accept.

#### List of particlespawner properties

All properties in this list of type "vec3 range", "float range" or "vec3" can
be animated with `*_tween` tables. For example, `jitter` can be tweened by
setting a `jitter_tween` table instead of (or in addition to) a `jitter`
table/value.

In this section, a float range is a table defined as so: { min = A, max = B }
A and B are your supplemented values. For a vec3 range this means they are vectors.
Types used are defined in the previous section.

* vec3 range `pos`: the position at which particles can appear

* vec3 range `vel`: the initial velocity of the particle

* vec3 range `acc`: the direction and speed with which the particle
  accelerates

* float range `size`: scales the visual size of the particle texture.
  if `node` is set, this can be set to 0 to spawn randomly-sized particles
  (just like actual node dig particles).

* vec3 range `jitter`: offsets the velocity of each particle by a random
  amount within the specified range each frame. used to create Brownian motion.

* vec3 range `drag`: the amount by which absolute particle velocity along
  each axis is decreased per second.  a value of 1.0 means that the particle
  will be slowed to a stop over the space of a second; a value of -1.0 means
  that the particle speed will be doubled every second. to avoid interfering
  with gravity provided by `acc`, a drag vector like `vector.new(1,0,1)` can
  be used instead of a uniform value.

* float range `bounce`: how bouncy the particles are when `collisiondetection`
  is turned on. values less than or equal to `0` turn off particle bounce;
  `1` makes the particles bounce without losing any velocity, and `2` makes
  them double their velocity with every bounce.  `bounce` is not bounded but
  values much larger than `1.0` probably aren't very useful.

* float range `exptime`: the number of seconds after which the particle
  disappears.

* table `attract`: sets the birth orientation of particles relative to various
  shapes defined in world coordinate space. this is an alternative means of
  setting the velocity which allows particles to emerge from or enter into
  some entity or node on the map, rather than simply being assigned random
  velocity values within a range. the velocity calculated by this method will
  be **added** to that specified by `vel` if `vel` is also set, so in most
  cases **`vel` should be set to 0**. `attract` has the fields:

  * string `kind`: selects the kind of shape towards which the particles will
    be oriented. it must have one of the following values:

    * `"none"`: no attractor is set and the `attractor` table is ignored
    * `"point"`: the particles are attracted to a specific point in space.
      use this also if you want a sphere-like effect, in combination with
      the `radius` property.
    * `"line"`: the particles are attracted to an (infinite) line passing
      through the points `origin` and `angle`. use this for e.g. beacon
      effects, energy beam effects, etc.
    * `"plane"`: the particles are attracted to an (infinite) plane on whose
      surface `origin` designates a point in world coordinate space. use this
      for e.g. particles entering or emerging from a portal.

  * float range `strength`: the speed with which particles will move towards
    `attractor`. If negative, the particles will instead move away from that
    point.

  * vec3 `origin`: the origin point of the shape towards which particles will
    initially be oriented. functions as an offset if `origin_attached` is also
    set.

  * vec3 `direction`: sets the direction in which the attractor shape faces. for
    lines, this sets the angle of the line; e.g. a vector of (0,1,0) will
    create a vertical line that passes through `origin`. for planes, `direction`
    is the surface normal of an infinite plane on whose surface `origin` is
    a point. functions as an offset if `direction_attached` is also set.

  * ObjectRef `origin_attached`: allows the origin to be specified as an offset
    from the position of an entity rather than a coordinate in world space.

  * ObjectRef `direction_attached`: allows the direction to be specified as an
    offset from the position of an entity rather than a coordinate in world space.

  * bool `die_on_contact`: if true, the particles' lifetimes are adjusted so
    that they will die as they cross the attractor threshold. this behavior
    is the default but is undesirable for some kinds of animations; set it to
    false to allow particles to live out their natural lives.

* vec3 range `radius`: if set, particles will be arranged in a sphere around
  `pos`. A constant can be used to create a spherical shell of particles, a
  vector to create an ovoid shell, and a range to create a volume; e.g.
  `{min = 0.5, max = 1, bias = 1}` will allow particles to appear between 0.5
  and 1 nodes away from `pos` but will cluster them towards the center of the
  sphere. Usually if `radius` is used, `pos` should be a single point, but it
  can still be a range if you really know what you're doing (e.g. to create a
  "roundcube" emitter volume).

### Textures

In versions before v5.6.0, particle/particlespawner textures could only be
specified as a single texture string. After v5.6.0, textures can now be
specified as a table as well. This table contains options that allow simple
animations to be applied to the texture.

```lua
texture = {
    name = "mymod_particle_texture.png",
    -- the texture specification string

    alpha = 1.0,
    -- controls how visible the particle is; at 1.0 the particle is fully
    -- visible, at 0, it is completely invisible.

    alpha_tween = {1, 0},
    -- can be used instead of `alpha` to animate the alpha value over the
    -- particle's lifetime. these tween tables work identically to the tween
    -- tables used in particlespawner properties, except that time references
    -- are understood with respect to the particle's lifetime, not the
    -- spawner's. {1,0} fades the particle out over its lifetime.

    scale = 1,
    scale = {x = 1, y = 1},
    -- scales the texture onscreen

    scale_tween = {
        {x = 1, y = 1},
        {x = 0, y = 1},
    },
    -- animates the scale over the particle's lifetime. works like the
    -- alpha_tween table, but can accept two-dimensional vectors as well as
    -- integer values. the example value would cause the particle to shrink
    -- in one dimension over the course of its life until it disappears

    blend = "alpha",
    -- (default) blends transparent pixels with those they are drawn atop
    -- according to the alpha channel of the source texture. useful for
    -- e.g. material objects like rocks, dirt, smoke, or node chunks
    -- note: there will be rendering bugs when particles interact with
    -- translucent nodes. particles are also not transparency-sorted
    -- relative to each other.
    blend = "clip",
    -- pixels are either fully opaque or fully transparent,
    -- depending on whether alpha is greater than or less than 50%
    -- (just like `use_texture_alpha = "clip"` for nodes).
    -- you should prefer this if you don't need semi-transparency, as it's faster.
    blend = "add",
    -- adds the value of pixels to those underneath them, modulo the sources
    -- alpha channel. useful for e.g. bright light effects like sparks or fire
    blend = "screen",
    -- like "add" but less bright. useful for subtler light effects. note that
    -- this is NOT formally equivalent to the "screen" effect used in image
    -- editors and compositors, as it does not respect the alpha channel of
    -- of the image being blended
    blend = "sub",
    -- the inverse of "add"; the value of the source pixel is subtracted from
    -- the pixel underneath it. a white pixel will turn whatever is underneath
    -- it black; a black pixel will be "transparent". useful for creating
    -- darkening effects

    animation = {Tile Animation definition},
    -- overrides the particlespawner's global animation property for a single
    -- specific texture
}
```

For particlespawners, it is also possible to set the `texpool` property instead
of a single texture definition. A `texpool` consists of a list of possible
particle textures. Every time a particle is spawned, the engine will pick a
texture at random from the `texpool` and assign it as that particle's texture.
You can also specify a `texture` in addition to a `texpool`; the `texture`
value will be ignored on newer clients but will be sent to older (pre-v5.6.0)
clients that do not implement texpools.

```lua
texpool = {
    "mymod_particle_texture.png";
    { name = "mymod_spark.png", alpha_tween = {1, 0} },
    {
      name = "mymod_dust.png",
      alpha = 0.3,
      scale = 1.5,
      animation = {
            type = "vertical_frames",
            aspect_w = 16, aspect_h = 16,

            length = 3,
            -- the animation lasts for 3s and then repeats
            length = -3,
            -- repeat the animation three times over the particle's lifetime
            -- (post-v5.6.0 clients only)
      },
    },
}
```

#### List of animatable texture properties

While animated particlespawner values vary over the course of the particlespawner's
lifetime, animated texture properties vary over the lifespans of the individual
particles spawned with that texture. So a particle with the texture property

```lua
alpha_tween = {
    0.0, 1.0,
    style = "pulse",
    reps = 4,
}
```

would be invisible at its spawning, pulse visible four times throughout its
lifespan, and then vanish again before expiring.

* float `alpha` (0.0 - 1.0): controls the visibility of the texture
* vec2 `scale`: controls the size of the displayed billboard onscreen. Its units
  are multiples of the parent particle's assigned size (see the `size` property above)
