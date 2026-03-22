Sounds
======

**NOTE: Connecting sounds to objects is not implemented.**

Only Ogg Vorbis files are supported.

For positional playing of sounds, only single-channel (mono) files are
supported. Otherwise OpenAL will play them non-positionally.

Mods should generally prefix their sounds with `modname_`, e.g. given
the mod name "`foomod`", a sound could be called:

    foomod_foosound.ogg

Sounds are referred to by their name with a dot, a single digit and the
file extension stripped out. When a sound is played, the actual sound file
is chosen randomly from the matching sounds.

When playing the sound `foomod_foosound`, the sound is chosen randomly
from the available ones of the following files:

* `foomod_foosound.ogg`
* `foomod_foosound.0.ogg`
* `foomod_foosound.1.ogg`
* (...)
* `foomod_foosound.9.ogg`

Examples of sound parameter tables:

```lua
-- Play locationless
{
    gain = 1.0, -- default
}
-- Play locationless, looped
{
    gain = 1.0, -- default
    loop = true,
}
-- Play in a location
{
    pos = {x = 1, y = 2, z = 3},
    gain = 1.0, -- default
}
```

Looped sounds must be played locationless.

### SimpleSoundSpec
* e.g. `""`
* e.g. `"default_place_node"`
* e.g. `{}`
* e.g. `{name = "default_place_node"}`
* e.g. `{name = "default_place_node", gain = 1.0}`
