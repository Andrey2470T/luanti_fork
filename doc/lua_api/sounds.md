Sounds
======

Only Ogg Vorbis files are supported.

For positional playing of sounds, only single-channel (mono) files are
supported. Otherwise OpenAL will play them non-positionally.

Mods should generally prefix their sound files with `modname_`, e.g. given
the mod name "`foomod`", a sound could be called:

    foomod_foosound.ogg

Sound group
-----------

A sound group is the set of all sound files, whose filenames are of the following
format:
`<sound-group name>[.<single digit>].ogg`
When a sound-group is played, one the files in the group is chosen at random.
Sound files can only be referred to by their sound-group name.

Example: When playing the sound `foomod_foosound`, the sound is chosen randomly
from the available ones of the following files:

* `foomod_foosound.ogg`
* `foomod_foosound.0.ogg`
* `foomod_foosound.1.ogg`
* (...)
* `foomod_foosound.9.ogg`

`SimpleSoundSpec`
-----------------

Specifies a sound name, gain (=volume), pitch and fade.
This is either a string or a table.

In string form, you just specify the sound name or
the empty string for no sound.

Table form has the following fields:

* `name`:
  Sound-group name.
  If == `""`, no sound is played.
* `gain`:
  Volume (`1.0` = 100%), must be non-negative.
  At the end, OpenAL clamps sound gain to a maximum of `1.0`. By setting gain for
  a positional sound higher than `1.0`, one can increase the radius inside which
  maximal gain is reached.
  Furthermore, gain of positional sounds doesn't increase inside a 1 node radius.
  The gain given here describes the gain at a distance of 3 nodes.
* `pitch`:
  Applies a pitch-shift to the sound.
  Each factor of `2.0` results in a pitch-shift of +12 semitones.
  Must be positive.
* `fade`:
  If > `0.0`, the sound is faded in, with this value in gain per second, until
  `gain` is reached.

`gain`, `pitch` and `fade` are optional and default to `1.0`, `1.0` and `0.0`.

Examples:

* `""`: No sound
* `{}`: No sound
* `"default_place_node"`: Play e.g. `default_place_node.ogg`
* `{name = "default_place_node"}`: Same as above
* `{name = "default_place_node", gain = 0.5}`: 50% volume
* `{name = "default_place_node", gain = 0.9, pitch = 1.1}`: 90% volume, 110% pitch

Sound parameter table
---------------------

Table used to specify how a sound is played:

```lua
{
    gain = 1.0,
    -- Scales the gain specified in `SimpleSoundSpec`.

    pitch = 1.0,
    -- Overwrites the pitch specified in `SimpleSoundSpec`.

    fade = 0.0,
    -- Overwrites the fade specified in `SimpleSoundSpec`.

    start_time = 0.0,
    -- Start with a time-offset into the sound.
    -- The behavior is as if the sound was already playing for this many seconds.
    -- Negative values are relative to the sound's length, so the sound reaches
    -- its end in `-start_time` seconds.
    -- It is unspecified what happens if `loop` is false and `start_time` is
    -- smaller than minus the sound's length.
    -- Available since feature `sound_params_start_time`.

    loop = false,
    -- If true, sound is played in a loop.

    pos = {x = 1, y = 2, z = 3},
    -- Play sound at a position.
    -- Can't be used together with `object`.

    object = <an ObjectRef>,
    -- Attach the sound to an object.
    -- Can't be used together with `pos`.

    to_player = name,
    -- Only play for this player.
    -- Can't be used together with `exclude_player`.

    exclude_player = name,
    -- Don't play sound for this player.
    -- Can't be used together with `to_player`.

    max_hear_distance = 32,
    -- Only play for players that are at most this far away when the sound
    -- starts playing.
    -- Needs `pos` or `object` to be set.
    -- `32` is the default.
}
```

Examples:

```lua
-- Play locationless on all clients
{
    gain = 1.0,   -- default
    fade = 0.0,   -- default
    pitch = 1.0,  -- default
}
-- Play locationless to one player
{
    to_player = name,
    gain = 1.0,   -- default
    fade = 0.0,   -- default
    pitch = 1.0,  -- default
}
-- Play locationless to one player, looped
{
    to_player = name,
    gain = 1.0,  -- default
    loop = true,
}
-- Play at a location, start the sound at offset 5 seconds
{
    pos = {x = 1, y = 2, z = 3},
    gain = 1.0,  -- default
    max_hear_distance = 32,  -- default
    start_time = 5.0,
}
-- Play connected to an object, looped
{
    object = <an ObjectRef>,
    gain = 1.0,  -- default
    max_hear_distance = 32,  -- default
    loop = true,
}
-- Play at a location, heard by anyone *but* the given player
{
    pos = {x = 32, y = 0, z = 100},
    max_hear_distance = 40,
    exclude_player = name,
}
```

Special sound-groups
--------------------

These sound-groups are played back by the engine if provided.

 * `player_damage`: Played when the local player takes damage (gain = 0.5)
 * `player_falling_damage`: Played when the local player takes
   damage by falling (gain = 0.5)
 * `player_jump`: Played when the local player jumps
 * `default_dig_<groupname>`: Default node digging sound (gain = 0.5)
   (see node sound definition for details)
